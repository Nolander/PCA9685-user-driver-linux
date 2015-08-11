/*
 * example.cpp
 *
 *  Created on: Aug 11, 2015
 *      Author: Nolan
 */

#include "pwm-pca9685-user.h"

#define NUM_CHANNELS 16
#define MIN 860
#define MAX 1260
#define SWEEP_INCR 10
#define WAIT_INCR_uS 500000

PCA9685_config myConfig;

uint8_t MY_ADDRESS = 0b10000000;
uint8_t MY_MODE1 = PCA9685_SETTING_MODE1_AUTOINCR | PCA9685_SETTING_MODE1_ALLCALL;
uint8_t MY_MODE2 = PCA9685_SETTING_MODE2_OCH | PCA9685_SETTING_MODE2_OUTNE_0;

int main(int argc, char** argv){

	int err;
	int i, j;
	int val_incr = (int)((MAX-MIN)/SWEEP_INCR);
	PCA9685_WORD_t chan_val;

	if(err = PCA9685_config_and_open_i2c(&myConfig, 1, MY_ADDRESS, MY_MODE1, MY_MODE2, PCA9685_FUTABAS3004_PWM_PERIOD)){
		printf("couldnt config and open i2c: err %d\n", err);
		return -1;
	}

	err = PCA9685_wake(&myConfig);

	for(j = 0;j<SWEEP_INCR;++j){
		chan_val = MIN + val_incr*j;
		for(i = 0;i<NUM_CHANNELS;++i){
			myConfig.channels[i].dutyTime_us = chan_val;
		}
		if(err = PCA9685_updateChannelRange(0, NUM_CHANNELS-1, &myConfig)){
			printf("couldnt update channel range: err %d\n", err);
			return -1;
		}
		usleep(WAIT_INCR_uS);
	}

	err = PCA9685_sleep(&myConfig);

	err = PCA9685_close_i2c(&myConfig);

	return 0;
}


