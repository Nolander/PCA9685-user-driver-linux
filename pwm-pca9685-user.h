/*
 * pwm-pca9685-user.h
 *
 *  Created on: Jul 31, 2015
 *      Author: Nolan Replogle
 *
 *	Refer to : https://www.adafruit.com/datasheets/PCA9685.pdf
 *
 *      Naming is mostly consistent with the Linux Kernel Driver for this device,
 *      which can be found here: http://lxr.free-electrons.com/source/drivers/pwm/pwm-pca9685.c
 *
 *	TODO: add support for auto increment
 *	TODO: add duty cycle param support
 *	TODO: add allcall LED support
 *	TODO: resolve I2C_SLAVE set so we can have multiple slaves
 *	TODO: add support for STOP vs ACK
 *	TODO: add support for phase
 *
 */
#ifndef PWM_PCA9685_USER_H_
#define PWM_PCA9685_USER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

//////////////////////////////////////////////
///////////////// ERROR LIST /////////////////
//////////////////////////////////////////////

#define PCA9685_ERR_NOERR					 0
#define PCA9685_ERR_NO_CONFIG				-1
#define PCA9685_ERR_DUTY_OVERFLOW			-2
#define PCA9685_ERR_PRESCALE_OVERFLOW		-3
#define PCA9685_ERR_PRESCALE_TOOLOW			-4
#define PCA9685_ERR_I2CfOPEN				-5
#define PCA9685_ERR_SET_SLAVEADDR			-6
#define PCA9685_ERR_WRITING_PRESCALE		-7
#define PCA9685_ERR_I2C_WRITE				-8
#define PCA9685_ERR_I2C_READ				-9
#define PCA9685_ERR_NO_FILE					-10
#define PCA9685_ERR_TRIVIAL_ACTION			-11
#define PCA9685_ERR_BOUNDS					-12

/////////////////////////////////////////////
/////////////// REGISTER LIST ///////////////
/////////////////////////////////////////////

#define PCA9685_REG_MODE1           	0x00
#define PCA9685_REG_MODE2           	0x01
#define PCA9685_REG_SUBADDR1        	0x02
#define PCA9685_REG_SUBADDR2        	0x03
#define PCA9685_REG_SUBADDR3        	0x04
#define PCA9685_REG_ALLCALLADDR     	0x05
#define PCA9685_REG_LEDX_ON_L       	0x06
#define PCA9685_REG_LEDX_ON_H       	0x07
#define PCA9685_REG_LEDX_OFF_L      	0x08
#define PCA9685_REG_LEDX_OFF_H      	0x09

#define PCA9685_REG_ALL_LED_ON_L    	0xFA
#define PCA9685_REG_ALL_LED_ON_H    	0xFB
#define PCA9685_REG_ALL_LED_OFF_L   	0xFC
#define PCA9685_REG_ALL_LED_OFF_H   	0xFD
#define PCA9685_REG_PRESCALE        	0xFE

/////////////////////////////////////////////
////////////// MODE SETTINGS ////////////////
/////////////////////////////////////////////

//settings for mode 1 reg
#define PCA9685_SETTING_MODE1_DEFAULTS 0b00010001
#define PCA9685_SETTING_MODE1_ALLCALL (1<<0)
#define PCA9685_SETTING_MODE1_SUB3 (1<<1)
#define PCA9685_SETTING_MODE1_SUB2 (1<<2)
#define PCA9685_SETTING_MODE1_SUB1 (1<<3)
//#define PCA9685_SETTING_MODE1_SLEEP (1<<4) //dont export this to the user
#define PCA9685_SETTING_MODE1_AUTOINCR (1<<5) //TODO: add support
#define PCA9685_SETTING_MODE1_EXTCLK (1<<6)
#define PCA9685_SETTING_MODE1_RESTART (1<<7)

//settings for mode 2 reg
#define PCA9685_SETTING_MODE2_DEFAULTS 0b00000100
#define PCA9685_SETTING_MODE2_OUTNE_0 (1<<0)
#define PCA9685_SETTING_MODE2_OUTNE_1 (1<<1)
#define PCA9685_SETTING_MODE2_OUTDRV (1<<2)
#define PCA9685_SETTING_MODE2_OCH (1<<3)
#define PCA9685_SETTING_MODE2_INVRT (1<<4)

//prescale
#define PCA9685_PRESCALE_DEFAULT 0b11111110
#define PCA9685_MIN_PRESCALE 3

//////////////////////////////////////////////
///////////// COMPANY SETTINGS ///////////////
//////////////////////////////////////////////

// Adafruit PCA9685
#define PCA9685_DEFAULT_OSC 25000000 //Hz, 25 MHz
#define PCA9685_DEFAULT_OSC_MHz 25
#define PCA9685_DEFAULT_PERIOD_FOR_INTOSC 5000 //200Hz

// Futaba S3 Series
#define FUTABA_MIN_PERIOD_us 800
#define FUTABA_MAX_PERIOD_us 1200
#define PCA9685_FUTABAS3004_PWM_PERIOD 14000 //us

//////////////////////////////////////////////
////////////////// A P I /////////////////////
//////////////////////////////////////////////

#define PCA9685_MAXCHAN         16

typedef uint16_t PCA9685_WORD_t;

//TODO: fix endianness issues here (fixed?)
typedef struct PCA9685_reg{
	union{
		PCA9685_WORD_t val;
		struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
			char hval;
			char lval;
#else // __BYTE_ORDER == __BIG_ENDIAN
			char lval;
			char hval;
#endif
		};
	};
} PCA9685_reg;

//here's a fix
#define GET_LOW(x) (char)x
#define GET_HIGH(x) (char)(x>>8)

typedef struct PCA9685_channel{
	uint32_t dutyTime_us;
	uint32_t dutyPhase_us;
} PCA9685_channel;

typedef struct PCA9685_config{

	PCA9685_channel channels[PCA9685_MAXCHAN];

//private: dont touch these directly
	int i2c_bus;
	int i2cFile;
	uint8_t dev_i2c_address;
	uint32_t osc_freq;//Hz
	uint32_t pwm_period;//us
	char mode1_settings;
	char mode2_settings;
	uint8_t prescale;
	char int_settings;
} PCA9685_config;

#ifdef __cplusplus
#define DEFAULT_PARAM(x) =x
#else
#define DEFAULT_PARAM(x)
#endif

int PCA9685_config_only(PCA9685_config* config,
		int i2cfile,
		uint8_t dev_address,
		uint8_t mode1_settings  DEFAULT_PARAM(PCA9685_SETTING_MODE1_DEFAULTS),
		uint8_t mode2_settings  DEFAULT_PARAM(PCA9685_SETTING_MODE2_DEFAULTS),
		uint32_t default_pwm_period_us  DEFAULT_PARAM(PCA9685_DEFAULT_PERIOD_FOR_INTOSC),
		uint32_t osc_freq_Hz  DEFAULT_PARAM(PCA9685_DEFAULT_OSC)//Hz
		);

int PCA9685_config_and_open_i2c(PCA9685_config* config,
		int i2cbus,
		uint8_t dev_address,
		uint8_t mode1_settings  DEFAULT_PARAM(PCA9685_SETTING_MODE1_DEFAULTS),
		uint8_t mode2_settings  DEFAULT_PARAM(PCA9685_SETTING_MODE2_DEFAULTS),
		uint32_t default_pwm_period_us  DEFAULT_PARAM(PCA9685_DEFAULT_PERIOD_FOR_INTOSC),
		uint32_t osc_freq_Hz  DEFAULT_PARAM(PCA9685_DEFAULT_OSC)//Hz
		);

int PCA9685_close_i2c(PCA9685_config* config);

int PCA9685_setAllChannelsToZero(PCA9685_config* config);

int PCA9685_updateChannelRange(uint8_t channel_start,
		uint8_t channel_end,
		PCA9685_config* config);

int PCA9685_updateChannels(PCA9685_WORD_t channels,
				PCA9685_config* config);

int PCA9685_updateChannel(uint8_t channel,
		PCA9685_config* config);

int PCA9685_writeReg(uint8_t reg,
		uint8_t val,
		PCA9685_config* config,
		uint8_t mask DEFAULT_PARAM(0xFF));

int PCA9685_readReg(uint8_t reg,
		char* buf,
		PCA9685_config* config);

int PCA9685_wake(PCA9685_config* config);

int PCA9685_sleep(PCA9685_config* config);

int PCA9685_softReset(PCA9685_config* config);

int PCA9685_enableAutoIncrement(PCA9685_config* config);

int PCA9685_disableAutoIncrement(PCA9685_config* config);

int PCA9685_changePWMPeriod(uint32_t newPeriod_us);

int PCA9685_changeExtOSC(uint32_t new_osc_freq_hz,
		PCA9685_config* config);

///////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* PWM_PCA9685_USER_H_ */
