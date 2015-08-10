
#include "pwm-pca9685-user.h"



#define NUM_CHANNELS 16
#define MIN 860
#define MAX 1260
#define SWEEP_INCR 10
#define WAIT_INCR_uS 500000


PCA9685_config myConfig;

int main(int argc, char** argv){

	int err;
	int i, j;
	int val_incr = (int)((MAX-MIN)/SWEEP_INCR);
	PCA9685_WORD_t chan_val;

	if(err = PCA9685_config_and_open_i2c(&myConfig, 1, 0b10000000,
			0b00000001, 0b00001001, PCA9685_FUTABAS3004_PWM_PERIOD)){
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
