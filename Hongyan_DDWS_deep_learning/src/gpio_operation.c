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

	while(i++ < 10)
	{
		if((j += 10) > 100)
		{
			j = 100;
		}

		set_led_brightness(j);

		milliseconds_sleep(300);
	}

	while(true)
	{
		milliseconds_sleep(5000);
	}

	pthread_exit(NULL);
}
