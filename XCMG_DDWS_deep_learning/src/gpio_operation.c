/*
 * gpio_operation.c
 *
 *  Created on: Nov 30, 2017
 *      Author: tony
 */

#include "gpio_operation.h"

/*
 * camera led lamp and vibration motor control thread function.
 *
 */
void* vibrate_motor(void* argv)
{
	unsigned short motor_pwm_period = 0;
	unsigned short motor_pwm_duty = 0;
	unsigned short i = 0, j;
	unsigned short min_vibration_time = 0; //minimum vibration time, unit: second

	while(true)
	{
		sleep(10);
	}

	pthread_exit(NULL);
}
