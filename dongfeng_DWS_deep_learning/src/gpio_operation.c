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

	unsigned short local_speed = 0;

	while(true)
	{
#if 0
		motor_pwm_period = config_param.motor_pwm_period;
		motor_pwm_duty = config_param.motor_pwm_duty;

		motor_pwm_period = 8;  //8*125ms = 1S
		motor_pwm_duty = motor_pwm_duty * motor_pwm_period/100;  //PWM duty, unit is ms

		if(motor_pwm_duty >= motor_pwm_period)
		{
			motor_pwm_duty = motor_pwm_period;
		}

		//printf("motor_pwm_period: %d, motor_pwm_duty: %d\n", motor_pwm_period, motor_pwm_duty);
		if(1 == serial_input_var.DDWS_switch)  //if DDWS function is enable
		{
			/* camera led control logic */
		    if((serial_input_var.vehicle_speed >> 8) < (config_param.vehicle_speed > 10 ? (config_param.vehicle_speed-10):0))
		    {
		    	set_led_brightness(0);
		    }
		    else if((serial_input_var.vehicle_speed >> 8) > config_param.vehicle_speed)
		    {
				if(0 == serial_input_var.small_lamp)  //if small light went out
				{
					/* camera led lamp working under maximum power mode */
					set_led_brightness(92);
				}
				else if(1 == serial_input_var.small_lamp)  //if small light turn on
				{
					/* camera led lamp working under low power mode */
					set_led_brightness(config_param.led_power_level);
				}
		    }

			/* vibration motor control logic */
			if(1 == serial_input_var.OK_switch) /* if OK_switch is enabled */
			{
				/*if level 3 warning occurred now, motor begin to vibrate*/
				if(LEVEL_THREE_WARNING == serial_output_var.warnning_level.warning_state)
				{
					if(i%motor_pwm_period < motor_pwm_duty) //motor begin to vibrate
					{
						gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 1);
						i++;
					}
					else if(i%motor_pwm_period < motor_pwm_period) //motor begin to stop
					{
						gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
						i++;
					}

					if(1 == serial_output_var.distract_warn)
					{
						min_vibration_time = 4; // minimum vibration time for distract warning is 4 seconds
					}
					else
					{
						min_vibration_time = 2; // minimum vibration time for other warnings is 2 seconds
					}
				}
				else // if level 3 warning disappeared, check vibrating time is less than minimum vibration time
				{
					j = i / motor_pwm_period;

					/* check vibration time is more than minimum time before level 3 warning disappeared */
					if(i > 0)
					{
						if(j < min_vibration_time)  //if vibrating time is less than minimum vibration time
						{
							/* vibrating motor continue working */
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
						else //if vibrating time is more than minimum vibration time
						{
							/* vibrating motor stop working */
							gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
							i = 0;
							j = 0;
							min_vibration_time = 0;
						}
					}
					else
					{
						gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);
						i = 0;
						j = 0;
						min_vibration_time = 0;
					}
				}
			}
			else if(0 == serial_input_var.OK_switch)  // if OK_switch is disabled, the motor stops vibrating.
			{
				gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);  //stop motor working
				i = 0;
				j = 0;
			}
		}
		else if(0 == serial_input_var.DDWS_switch)  //if DDWS function is disable
		{
			set_led_brightness(0);  //pull down gpio for camera led, close camera led
			gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);  //pull down gpio for motor, close vibrate motor
			i = 0;
			j = 0;
		}

		milliseconds_sleep(125);
#endif


		local_speed = serial_input_var.vehicle_speed >> 8;

		/* camera led control logic */
	    if((serial_input_var.vehicle_speed >> 8) < config_param.vehicle_speed)
	    {
	    	/* turn off camera led lamp */
	    	set_led_brightness(100);
	    }
	    else
	    {
	    	set_led_brightness(100);
#if 0
	    	/* if small lamp is off,*/
			if(0 == serial_input_var.small_lamp)
			{
				/* camera led lamp working under maximum power mode */
				set_led_brightness(99);
			}
			else if(1 == serial_input_var.small_lamp)  //if small light turn on
			{
				/* camera led lamp working under low power mode */
				set_led_brightness(config_param.led_power_level);
			}
#endif
	    }

	    milliseconds_sleep(1000);
	}

	pthread_exit(NULL);
}
