
#include "../pwm-pca9685-user.h"



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

	err = PCA9685_config_and_open_i2c(&myConfig, 0,0, PCA9685_SETTING_MODE1_AUTOINCR, 0 );

	for(j = 0;j<SWEEP_INCR;++j){
		chan_val = MIN + val_incr*j;
		for(i = 0;i<NUM_CHANNELS;++i){
			myConfig.channels[i].onTime.val = chan_val;
		}
		PCA9685_updateChannelRange(0, NUM_CHANNELS-1, PCA9685_FUTABAS3004_PWM_PERIOD, &myConfig);
		usleep(WAIT_INCR_uS);
	}

	PCA9685_sleep(&myConfig);

	PCA9685_close_i2c(&myConfig);

	return 0;
}
