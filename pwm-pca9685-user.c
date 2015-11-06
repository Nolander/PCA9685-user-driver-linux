

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "pwm-pca9685-user.h"

/////////////////////////////////////
////////// NON USER THINGS //////////
/////////////////////////////////////

#define PCA9685_NUMREGS         0xFF
#define PCA9685_WRITE_BIT 0
#define PCA9685_READ_BIT 1
#define PCA9685_PWM_PERIOD_BITS_PRECISION 12
#define MODE1_SLEEP (1<<4)
#define EXTOSC_ENABLED (1<<0)

#define VERIFY(x) if(!x){ \
						return PCA9685_ERR_NO_CONFIG; \
					}

#define LED_N_ON_H(N)   (PCA9685_REG_LEDX_ON_H + (4 * (N)))
#define LED_N_ON_L(N)   (PCA9685_REG_LEDX_ON_L + (4 * (N)))
#define LED_N_OFF_H(N)  (PCA9685_REG_LEDX_OFF_H + (4 * (N)))
#define LED_N_OFF_L(N)  (PCA9685_REG_LEDX_OFF_L + (4 * (N)))

static int __read_reg(uint8_t reg, char* buf, PCA9685_config* config);
static int __write_reg(uint8_t reg, uint8_t val, PCA9685_config* config);
static int __execute_settings(PCA9685_config* config);
static int __calc_prescale(uint32_t period, uint32_t osc, PCA9685_config* config);

///////////////////////////////////////////////

//static char* i2cPath = {'/','d', 'e', 'v', '/', 'i', '2', 'c', '-', 0 , 0 };

//PCA9685_ERR_NOERR
//PCA9685_ERR_NO_CONFIG
//PCA9685_ERR_DUTY_OVERFLOW
//PCA9685_ERR_PRESCALE_OVERFLOW
//PCA9685_ERR_PRESCALE_TOOLOW
//PCA9685_ERR_I2CfOPEN
//PCA9685_ERR_SET_SLAVEADDR
//PCA9685_ERR_WRITING_PRESCALE
//PCA9685_ERR_I2C_WRITE
//PCA9685_ERR_I2C_READ

/*
 *
 * Calculates the prescale based on Equation 1 in the literature.
 * The prescaler does not seem to be very precise, or at least the formula for calculating
 * it is over simplified. For example, when prescale equals 3, at a given update rate, the internal osc
 * is calculated to be 21 MHz (instead of 25 MHz). For prescale values above 80 or so, the internal osc
 * is calculated to be about 28 MHz.
 *
 * TODO: improve prescale calculation when internal oscillator is used.
 *
 */
static int __calc_prescale(uint32_t period_us, uint32_t osc, PCA9685_config* config ){

	uint32_t prescale;
	prescale = ((osc / 1000000)*period_us) >> 12; //dividing by 4096
	prescale--;
	if(prescale >> 8)
		return PCA9685_ERR_PRESCALE_OVERFLOW;

	uint8_t prescaleC = (uint8_t)prescale;

	if(prescaleC < PCA9685_MIN_PRESCALE)
		return PCA9685_ERR_PRESCALE_TOOLOW;

	config->prescale = prescaleC;

	return PCA9685_ERR_NOERR;
}

int PCA9685_config_only(PCA9685_config* config,
		int i2cfile,
		uint8_t dev_address,
		uint8_t mode1_settings,
		uint8_t mode2_settings,
		uint32_t default_pwm_period_us,
		uint32_t osc_freq_Hz)
{
	int err;

	if(!config)
		return PCA9685_ERR_NO_CONFIG;

	//TODO: see if defaulting the dev_address messes with the auto increment protocol

	if (ioctl(config->i2cFile, I2C_SLAVE, dev_address>>1) < 0) {
		perror("i2cSetAddress");
		return PCA9685_ERR_I2CfOPEN;
	}


	config->i2cFile = i2cfile;
	config->dev_i2c_address = dev_address;
	config->mode1_settings = mode1_settings;
	config->mode2_settings = mode2_settings;

	if(err = __calc_prescale(default_pwm_period_us, osc_freq_Hz, config))
			return err;

	config->pwm_period = default_pwm_period_us;
	config->osc_freq = osc_freq_Hz;

	return __execute_settings(config);
}

int PCA9685_config_and_open_i2c(PCA9685_config* config,
		int i2cbus,
		uint8_t dev_address,
		uint8_t mode1_settings,
		uint8_t mode2_settings,
		uint32_t default_pwm_period_us,
		uint32_t osc_freq_Hz)
{
	int err;
	char i2cpath[11] = {0};

	if(!config)
		return PCA9685_ERR_NO_CONFIG;

	config->i2c_bus = i2cbus;
	config->dev_i2c_address = dev_address;

	//i2cpath = config->i2c_bus == 0 ? (char*)"/dev/i2c-0" : (char*)"/dev/i2c-1";
	snprintf(i2cpath, sizeof(i2cpath), "/dev/i2c-%d", i2cbus);

	config->i2cFile = open(i2cpath, O_RDWR);

	if (config->i2cFile < 0) {
		perror("i2cOpen");
		return PCA9685_ERR_I2CfOPEN;
	}
	//see if defaulting the dev_address messes with the auto increment protocol

	if (ioctl(config->i2cFile, I2C_SLAVE, (char)(dev_address>>1)) < 0) {
		perror("i2cSetAddress");
		return PCA9685_ERR_SET_SLAVEADDR;
	}

	config->mode1_settings = mode1_settings;
	config->mode2_settings = mode2_settings;

	if(err = __calc_prescale(default_pwm_period_us, osc_freq_Hz, config))
		return err;

	config->pwm_period = default_pwm_period_us;
	config->osc_freq = osc_freq_Hz;

	return __execute_settings(config);
}

/*
 *
 * This function is only called from one of the config functions above
 */
static int __execute_settings(PCA9685_config* config){

	//prescale can only be set when SLEEP is enabled, so we do it first
	if(config->prescale != PCA9685_PRESCALE_DEFAULT)
		if(__write_reg(PCA9685_REG_PRESCALE, config->prescale, config)){
			perror("unable to set prescale value");
			return PCA9685_ERR_WRITING_PRESCALE;
		}

	//we do mode2 settings first in case mode 1 intends to set auto increment
	if(config->mode2_settings != PCA9685_SETTING_MODE2_DEFAULTS)
		if(__write_reg(PCA9685_REG_MODE2, config->mode2_settings, config)){
			perror("unable to set mode 2");
			return PCA9685_ERR_I2C_WRITE;
		}

	config->mode1_settings |= MODE1_SLEEP;

	if(config->mode1_settings != PCA9685_SETTING_MODE1_DEFAULTS)
		if(__write_reg(PCA9685_REG_MODE1, config->mode1_settings, config)){
			perror("unable to set mode 1");
			return PCA9685_ERR_I2C_WRITE;
		}

	return PCA9685_ERR_NOERR;

}


int PCA9685_close_i2c(PCA9685_config* config){
	VERIFY(config);

	if(config->i2cFile)
		close(config->i2cFile);
	else
		return PCA9685_ERR_NO_FILE;

	return PCA9685_ERR_NOERR;
}

int PCA9685_setAllChannelsToZero(PCA9685_config* config){

	VERIFY(config);


	int i;
	for(i = 0;i< PCA9685_MAXCHAN;++i){
		config->channels[i].dutyTime_us = 0;
	}

	return PCA9685_updateChannelRange(0, PCA9685_MAXCHAN - 1, config);
}

int PCA9685_updateChannels(PCA9685_WORD_t channels,
				PCA9685_config* config)
{
	VERIFY(config);

	if (ioctl(config->i2cFile, I2C_SLAVE, (char)(config->dev_i2c_address>>1)) < 0) {
		perror("i2cSetAddress");
		return PCA9685_ERR_SET_SLAVEADDR;
	}

	char data[5];
	PCA9685_reg offtime;
	int i;
	for(i=0;i<PCA9685_MAXCHAN;++i){

		if((channels & (1<<i)) == 0)
			continue;

		if(config->pwm_period < config->channels[i].dutyTime_us)
			return PCA9685_ERR_DUTY_OVERFLOW;

		offtime.val = (config->channels[i].dutyTime_us << 12) / config->pwm_period;

		if(config->mode1_settings & PCA9685_SETTING_MODE1_AUTOINCR){

			data[0] = LED_N_ON_L(i);
			data[1] = GET_LOW(0);//on time
			data[2] = GET_HIGH(0);
			data[3] = GET_LOW(offtime.val);
			data[4] = GET_HIGH(offtime.val);
			if (write(config->i2cFile, data, 5) != 5) {
				perror("error updating channels");
				return PCA9685_ERR_I2C_WRITE;
			}
		}
		else{
			if (__write_reg(LED_N_ON_L(i), GET_LOW(0), config))
				return PCA9685_ERR_I2C_WRITE;

			if (__write_reg(LED_N_ON_H(i), GET_HIGH(0), config))
				return PCA9685_ERR_I2C_WRITE;

			if (__write_reg(LED_N_OFF_L(i), GET_LOW(offtime.val), config))
				return PCA9685_ERR_I2C_WRITE;

			if (__write_reg(LED_N_OFF_H(i), GET_HIGH(offtime.val), config))
				return PCA9685_ERR_I2C_WRITE;
		}
	}

	return PCA9685_ERR_NOERR;

}

int PCA9685_updateChannelRange(uint8_t channel_start,
		uint8_t channel_end,
		PCA9685_config* config)
{
	VERIFY(config);

	if (ioctl(config->i2cFile, I2C_SLAVE, (char)(config->dev_i2c_address>>1)) < 0) {
		perror("i2cSetAddress");
		return PCA9685_ERR_SET_SLAVEADDR;
	}

	uint8_t data[PCA9685_MAXCHAN*4 + 1]; //dynamically allocating on the stack is bad/impossible, so we dont do that
	int n_bytes = ((channel_end - channel_start + 1) << 2) + 1;
	PCA9685_reg offtime;
	int i;

	if(channel_start > channel_end || channel_end >= PCA9685_MAXCHAN)
		return PCA9685_ERR_BOUNDS;

	if(config->mode1_settings & PCA9685_SETTING_MODE1_AUTOINCR){
		data[0] = LED_N_ON_L(channel_start);

		for(i=channel_start;i<=channel_end;++i){

			if(config->pwm_period < config->channels[i].dutyTime_us)
				return PCA9685_ERR_DUTY_OVERFLOW;

			offtime.val = (config->channels[i].dutyTime_us << 12) / config->pwm_period;

			data[(i<<2) + 1] = 0;
			data[(i<<2) + 2] = 0;
			data[(i<<2) + 3] = GET_LOW(offtime.val);
			data[(i<<2) + 4] = GET_HIGH(offtime.val);
		}
		if(write(config->i2cFile, data, n_bytes) != n_bytes)
			return PCA9685_ERR_I2C_WRITE;

	}

	else{
		for(i=channel_start;i<=channel_end;++i){

			if(config->pwm_period < config->channels[i].dutyTime_us)
				return PCA9685_ERR_DUTY_OVERFLOW;

			offtime.val = (config->channels[i].dutyTime_us << 12) / config->pwm_period;

			if (__write_reg(LED_N_ON_L(i), GET_LOW(0), config))
				return PCA9685_ERR_I2C_WRITE;

			if (__write_reg(LED_N_ON_H(i), GET_HIGH(0), config))
				return PCA9685_ERR_I2C_WRITE;

			if (__write_reg(LED_N_OFF_L(i), GET_LOW(offtime.val), config))
				return PCA9685_ERR_I2C_WRITE;

			if (__write_reg(LED_N_OFF_H(i), GET_HIGH(offtime.val), config))
				return PCA9685_ERR_I2C_WRITE;
		}
	}

	return PCA9685_ERR_NOERR;

}

int PCA9685_updateChannel(uint8_t channel,
		PCA9685_config* config)
{
	VERIFY(config);

	if (ioctl(config->i2cFile, I2C_SLAVE, (char)(config->dev_i2c_address>>1)) < 0) {
		perror("i2cSetAddress");
		return PCA9685_ERR_SET_SLAVEADDR;
	}


	PCA9685_reg offtime;
	uint8_t data[5];

	if(channel >= PCA9685_MAXCHAN)
		return PCA9685_ERR_BOUNDS;

	if(config->pwm_period < config->channels[channel].dutyTime_us)
		return PCA9685_ERR_DUTY_OVERFLOW;

	offtime.val = (config->channels[channel].dutyTime_us << 12) / config->pwm_period;

	if(config->mode1_settings & PCA9685_SETTING_MODE1_AUTOINCR){

		data[0] = LED_N_ON_L(channel);
		data[1] = 0;
		data[2] = 0;
		data[3] = GET_LOW(offtime.val);
		data[4] = GET_HIGH(offtime.val);

		if(write(config->i2cFile, data, 5) != 5)
			return PCA9685_ERR_I2C_WRITE;
	}
	else{

		if (__write_reg(LED_N_ON_L(channel), GET_LOW(0), config))
			return PCA9685_ERR_I2C_WRITE;

		if (__write_reg(LED_N_ON_H(channel), GET_HIGH(0), config))
			return PCA9685_ERR_I2C_WRITE;

		if (__write_reg(LED_N_OFF_L(channel), GET_LOW(offtime.val), config))
			return PCA9685_ERR_I2C_WRITE;

		if (__write_reg(LED_N_OFF_H(channel), GET_HIGH(offtime.val), config))
			return PCA9685_ERR_I2C_WRITE;

	}

	return PCA9685_ERR_NOERR;

}

int PCA9685_writeReg(uint8_t reg,
		uint8_t val,
		PCA9685_config* config,
		uint8_t mask)
{
	VERIFY(config);

	if (ioctl(config->i2cFile, I2C_SLAVE, (char)(config->dev_i2c_address>>1)) < 0) {
		perror("i2cSetAddress");
		return PCA9685_ERR_SET_SLAVEADDR;
	}


	char temp;
	int err;

	if(mask == 0)
		return PCA9685_ERR_TRIVIAL_ACTION;

	if(mask != 0xff){
		if(err = __read_reg(reg, &temp, config)){
			perror("error reading from device");
			return err;
		}
		val = (temp & (~mask)) | val;
	}

	return __write_reg(reg, val, config);
}

int PCA9685_readReg(uint8_t reg,
		char* buf,
		PCA9685_config* config)
{
	VERIFY(config);

	return __read_reg(reg, buf, config);

}

static int __write_reg(uint8_t reg,
		uint8_t val,
		PCA9685_config* config)
{
	uint8_t data[2];

	if (ioctl(config->i2cFile, I2C_SLAVE, (char)(config->dev_i2c_address>>1)) < 0) {
		perror("i2cSetAddress");
		return PCA9685_ERR_SET_SLAVEADDR;
	}

	//data[0] = config->dev_i2c_address | PCA9685_WRITE_BIT;
	data[0] = reg;
	data[1] = val;

	if (write(config->i2cFile, data, 2) != 2) {
		perror("pca9555SetRegister");
		return PCA9685_ERR_I2C_WRITE;
	}

	return PCA9685_ERR_NOERR;
}

static int __read_reg(uint8_t reg,
		char* buf,
		PCA9685_config* config)
{

	char data[2];
	//data[0] = config->dev_i2c_address;
	data[0] = reg;

	if (ioctl(config->i2cFile, I2C_SLAVE, (char)(config->dev_i2c_address>>1)) < 0) {
		perror("i2cSetAddress");
		return PCA9685_ERR_SET_SLAVEADDR;
	}

	if(write(config->i2cFile, data, 1) != 1 ){
		perror("pca9555SetRegisterPair");
		return PCA9685_ERR_I2C_WRITE;
	}

	if(read(config->i2cFile, buf, 1) != 1 ){
		perror("pca9555SetRegisterPair");
		return PCA9685_ERR_I2C_READ;
	}

	return PCA9685_ERR_NOERR;
}

int PCA9685_wake(PCA9685_config* config)
{
	VERIFY(config);

	PCA9685_updateChannelRange(0, PCA9685_MAXCHAN-1, config);

	if(PCA9685_writeReg(PCA9685_REG_MODE1,config->mode1_settings & ~MODE1_SLEEP, config, 0xff))
		return PCA9685_ERR_I2C_WRITE;

	if(config->int_settings ^ EXTOSC_ENABLED)
		usleep(500);

	return PCA9685_ERR_NOERR;
}

int PCA9685_sleep(PCA9685_config* config)

{
	VERIFY(config);

	return PCA9685_writeReg(PCA9685_REG_MODE1,config->mode1_settings | MODE1_SLEEP,config, 0xff);
}
/*

//TODO
int PCA9685_softReset(PCA9685_config* config)
{
	VERIFY(config);
	return PCA9685_writeReg(PCA9685_REG_MODE1,0,0,config);

}

//TODO
int PCA9685_enableAutoIncrement(PCA9685_config* config)
{
	VERIFY(config);
	return PCA9685_writeReg(PCA9685_REG_MODE1,0,0,config);
}

//TODO
int PCA9685_disableAutoIncrement(PCA9685_config* config)
{
	VERIFY(config);

	return PCA9685_writeReg(PCA9685_REG_MODE1,0,0,config);
}

//TODO
int PCA9685_changePWMPeriod(int newPeriod_us, PCA9685_config* config)
{
	VERIFY(config);

	config->pwm_period = newPeriod_us;

	return PCA9685_ERR_NOERR;
}

//TODO

 //This function should be called after the external osc has been changed in hardware

int PCA9685_changeExtOSC(int new_osc_freq_hz, PCA9685_config* config)
{
	VERIFY(config);

	config->osc_freq = new_osc_freq_hz;

	return PCA9685_ERR_NOERR;
}
*/
