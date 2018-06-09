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

	motor_pwm_period = config_param.motor_pwm_period;
	motor_pwm_duty = config_param.motor_pwm_duty;
	printf("motor_pwm_period: %d\n", motor_pwm_period);

	while(true)
	{
		motor_pwm_period = 8;  //8*125ms = 1S

		//motor_pwm_duty = motor_pwm_duty * motor_pwm_period/100;  //PWM duty, unit is ms
		motor_pwm_duty = 6;

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
					set_led_brightness(100);
				}
				else if(1 == serial_input_var.small_lamp)  //if small light turn on
				{
					/* camera led lamp working under low power mode */
					set_led_brightness(config_param.led_power_level);
				}
		    }

			/* vibration motor control logic */
			if((OK_SWITCH_DISABLE == serial_input_var.OK_switch) && \
					(0 == OK_Switch_timer_flag.timer_val)) /* if OK_switch is enabled */
			{
				/*if level 3 warning occurred now, motor begin to vibrate*/
				if(LEVEL_THREE_WARNING == serial_output_var.warnning_level.warning_state)
				//if(serial_output_var.warnning_level&0x3f > 0)
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

					min_vibration_time = 2;

#if 0
					if(1 == serial_output_var.distract_warn)
					{
						min_vibration_time = 4; // minimum vibration time for distract warning is 4 seconds
					}
					else
					{
						min_vibration_time = 2; // minimum vibration time for other warnings is 2 seconds
					}
#endif
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

				free_spec_type_alarm(OK_Switch_timer_3s);
			}
			else if((OK_SWITCH_DISABLE == serial_input_var.OK_switch) && \
					(1 == OK_Switch_timer_flag.timer_val))
			{
				gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);  //stop motor working
				i = 0;
				j = 0;
			}
			else if((OK_SWITCH_ENABLE == serial_input_var.OK_switch) && \
					(1 == OK_Switch_timer_flag.timer_val))
			{
				gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);  //stop motor working
				i = 0;
				j = 0;
			}
			else if((OK_SWITCH_ENABLE == serial_input_var.OK_switch) && \
					(0 == OK_Switch_timer_flag.timer_val))  //if OK_switch is enabled, the motor stops vibrating.
			{
				gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);  //stop motor working
				i = 0;
				j = 0;

				/**/
				OK_Switch_timer_flag.timer_val = 1;
				SetAlarm(&OK_Switch_timer_flag, OK_Switch_timer_3s, &timeout_execute_activity,\
						S_TO_TIMEVAL(3), 0);
				printf("OK_Switch_timer_flag.timer_val: %X\n", OK_Switch_timer_flag.timer_val);
			}
		}
		//else if(0 == serial_input_var.DDWS_switch)  //if DDWS function is disable
		else
		{
			set_led_brightness(0);  //pull down gpio for camera led, close camera led
			gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);  //pull down gpio for motor, close vibrate motor
			i = 0;
			j = 0;
		}

		milliseconds_sleep(125);
	}

	pthread_exit(NULL);
}
