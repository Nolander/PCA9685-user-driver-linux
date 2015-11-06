# PCA9685-user-driver-linux

All you need is pwm-pca9685-user.h and pwm-pca9685-user.c

This is not thread safe if you are communicating with other devices on the same i2c bus, but it can share the bus
with other devices as all the i2c bus communication is done in the same thread.
