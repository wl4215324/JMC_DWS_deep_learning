/*
 * gpio_operation.c
 *
 *  Created on: Nov 30, 2017
 *      Author: tony
 */

#include "gpio_operation.h"


void* vibrate_motor(void* argv)
{
	unsigned short motor_pwm_period = 0;
	unsigned short motor_pwm_duty = 0;
	unsigned short i = 0, j;

	while(true)
	{
		motor_pwm_period = config_param.motor_pwm_period;
		motor_pwm_duty = config_param.motor_pwm_duty;

		motor_pwm_period = 10;  //10*100ms
		motor_pwm_duty = motor_pwm_duty * motor_pwm_period/100;  //ms

		if(motor_pwm_duty >= motor_pwm_period)
		{
			motor_pwm_duty = motor_pwm_period;
		}

		//printf("motor_pwm_period: %d, motor_pwm_duty: %d\n", motor_pwm_period, motor_pwm_duty);

		if(0 == serial_input_var.DDWS_switch)  //if DWS enable
		{
		    if((serial_input_var.vehicle_speed >> 8) < (config_param.vehicle_speed > 10 ? (config_param.vehicle_speed-10):0))
		    {
		    	set_led_brightness(0);
		    }
		    else if((serial_input_var.vehicle_speed >> 8) > config_param.vehicle_speed)
		    {
				if(0 == serial_input_var.small_lamp)  //if small light went out
				{
					/* pull up gpio for camera led, working in maximum power mode */
					set_led_brightness(50);
				}
				else if(1 == serial_input_var.small_lamp)  //if small light turn on
				{
					/* pull up gpio for camera led, working in low power mode */
					set_led_brightness(config_param.led_power_level);
				}
		    }

			/* if OK_switch is enabled */
			if(0 == serial_input_var.OK_switch)
			{
				/*if level 3 warning occurred now, motor begin to vibrate*/
				if(3 == serial_output_var.warning_state)
				{
					if(i%motor_pwm_period < motor_pwm_duty)
					{
						gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 1);
						i++;
					}
					else if(i%motor_pwm_period < motor_pwm_period)
					{
						gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
						i++;
					}
				}
				else
				{
					j = i / motor_pwm_period;

					if(i > 0)
					{
						if(j < 2)
						{
							if(i%motor_pwm_period < motor_pwm_duty)
							{
								gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 1);
								i++;
							}
							else if(i%motor_pwm_period < motor_pwm_period)
							{
								gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
								i++;
							}
						}
						else
						{
							gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
							i = 0;
							j = 0;
						}
					}
					else
					{
						gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
						i = 0;
						j = 0;
					}
				}

#if 0
				/*if level 3 warning occurred now, motor begin to vibrate*/
				if(3 == serial_output_var.warning_state)
				{
					if((i >= 0 && i < motor_pwm_duty) || \
							(i >= motor_pwm_period && i < (motor_pwm_duty+motor_pwm_period)))
					{
						gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 1);
						i++;
					}
					else if((i >= motor_pwm_duty && i < motor_pwm_period) || \
							(i >= (motor_pwm_period+motor_pwm_duty) && i < 2*motor_pwm_period))
					{
						gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
						i++;
					}
					else
					{
						gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
						i = 2*motor_pwm_period;
					}
				}
				else
				{
					if(i > 0 && i < 2*motor_pwm_period)
					{
						if((i >= 0 && i < motor_pwm_duty) || \
								(i >= motor_pwm_period && i < (motor_pwm_duty+motor_pwm_period)))
						{
							gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 1);
							i++;
						}
						else if((i >= motor_pwm_duty && i < motor_pwm_period) || \
								(i >= (motor_pwm_period+motor_pwm_duty) && i < 2*motor_pwm_period))
						{
							gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
							i++;
						}
						else
						{
							gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
							i = 2*motor_pwm_period;
						}
					}
					else if(i >= 2*motor_pwm_period)
					{
						/*if there is no level 3 warning, stop motor*/
						i = 0;
						gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
					}
				}
#endif
			}
			else if(1 == serial_input_var.OK_switch)  // if OK_switch is disabled
			{
				i = 0;
				j = 0;
				gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
			}
		}
		else if(1 == serial_input_var.DDWS_switch)  // if DWS disable
		{
			set_led_brightness(0);  //pull down gpio for camera led, close camera led
			i = 0;
			j = 0;
			gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);  //pull down gpio for motor, close vibrate motor
		}

		milliseconds_sleep(100);
	}

	pthread_exit(NULL);
}
