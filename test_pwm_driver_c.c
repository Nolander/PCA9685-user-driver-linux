
#include "pwm-pca9685-user.h"



#define NUM_CHANNELS 16
#define MIN 860
#define MAX 12600
#define SWEEP_INCR 10
#define WAIT_INCR_uS 50000

static int test1_updateChannel();
static int test2_updateChannels();
static int test3_updateChannelRange();

PCA9685_config myConfig;

int main(int argc, char** argv){

	int err;


	//test 1

	err = PCA9685_config_and_open_i2c(&myConfig, 1, 0b10000000, 0b00000001, 0b00000101, PCA9685_FUTABAS3004_PWM_PERIOD,
			PCA9685_DEFAULT_OSC);

	err = PCA9685_wake(&myConfig);

	err = test1_updateChannel();

	err = PCA9685_sleep(&myConfig);

	//test 2

	err = PCA9685_config_only(&myConfig, myConfig.i2cFile, 0b10000000, 0b00000001, 0b00000101, PCA9685_FUTABAS3004_PWM_PERIOD,
			PCA9685_DEFAULT_OSC);

	err = PCA9685_wake(&myConfig);

	err = test2_updateChannels();

	err = PCA9685_sleep(&myConfig);

	//test 3

	err = PCA9685_config_only(&myConfig, myConfig.i2cFile, 0b10000000, 0b00000001, 0b00000101, PCA9685_FUTABAS3004_PWM_PERIOD,
			PCA9685_DEFAULT_OSC);

	err = PCA9685_wake(&myConfig);

	err = test3_updateChannelRange();

	err = PCA9685_sleep(&myConfig);

	//test 1

	err = PCA9685_config_and_open_i2c(&myConfig, 1, 0b10000000, 0b00100001, 0b00000101, PCA9685_FUTABAS3004_PWM_PERIOD,
			PCA9685_DEFAULT_OSC);

	err = PCA9685_wake(&myConfig);

	err = test1_updateChannel();

	err = PCA9685_sleep(&myConfig);

	//test 2

	err = PCA9685_config_only(&myConfig, myConfig.i2cFile, 0b10000000, 0b00100001, 0b00000101, PCA9685_FUTABAS3004_PWM_PERIOD,
			PCA9685_DEFAULT_OSC);

	err = PCA9685_wake(&myConfig);

	err = test2_updateChannels();

	err = PCA9685_sleep(&myConfig);

	//test 3

	err = PCA9685_config_only(&myConfig, myConfig.i2cFile, 0b10000000, 0b00100001, 0b00000101, PCA9685_FUTABAS3004_PWM_PERIOD,
			PCA9685_DEFAULT_OSC);

	err = PCA9685_wake(&myConfig);

	err = test3_updateChannelRange();

	err = PCA9685_sleep(&myConfig);

	// end

	// end


	err = PCA9685_close_i2c(&myConfig);

	return 0;
}

static int test1_updateChannel(){
	int i, j;
	int val_incr = (int)((MAX-MIN)/SWEEP_INCR);
	PCA9685_WORD_t chan_val;

	for(j = 0;j<SWEEP_INCR;++j){
		chan_val = MIN + val_incr*j;
		for(i = 0;i<NUM_CHANNELS;++i){
			myConfig.channels[i].dutyTime_us = chan_val;
			if(PCA9685_updateChannel(i, &myConfig)){
				return -1;
			}
		}

		usleep(WAIT_INCR_uS);
	}

	return 0;
}

static int test2_updateChannels(){
	int i, j;
	int val_incr = (int)((MAX-MIN)/SWEEP_INCR);
	PCA9685_WORD_t chan_val;

	for(j = 0;j<SWEEP_INCR;++j){
		chan_val = MIN + val_incr*j;
		for(i = 0;i<NUM_CHANNELS;++i){
			myConfig.channels[i].dutyTime_us = chan_val;
		}
		if(PCA9685_updateChannels(0xFFFF, &myConfig)){
			return -1;
		}
		usleep(WAIT_INCR_uS);
	}

	return 0;

}

static int test3_updateChannelRange(){
	int i, j;
	int val_incr = (int)((MAX-MIN)/SWEEP_INCR);
	PCA9685_WORD_t chan_val;

	for(j = 0;j<SWEEP_INCR;++j){
		chan_val = MIN + val_incr*j;
		for(i = 0;i<NUM_CHANNELS;++i){
			myConfig.channels[i].dutyTime_us = chan_val;
		}
		if(PCA9685_updateChannelRange(0, NUM_CHANNELS-1, &myConfig)){
			return -1;
		}
		usleep(WAIT_INCR_uS);
	}

	return 0;

}
