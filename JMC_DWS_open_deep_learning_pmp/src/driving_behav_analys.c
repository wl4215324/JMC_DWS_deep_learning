/*
 * driving_behav_analys.c
 *
 *  Created on: Nov 25, 2017
 *      Author: tony
 */

#include "driving_behav_analys.h"

#ifdef ENABLE_ACCELERATOR
TimerFlag timer_flag = {0x1f};
#else
TimerFlag timer_flag = {0xf};
#endif

TimerFlag OK_Switch_timer_flag = {0};

TimerFlag level2_closing_eye_timer_flag = {0};


/* callback function for timer, if timeout happened this function will be executed */
void timeout_execute_activity(TimerFlag* timer_flag, TimerEventType timer_event_type)
{
	struct timeval cur_time;

	gettimeofday(&cur_time, NULL);
	printf("function: %s, now second: %ld, timer_flag->timer_val: %d timer_event_type: %d\n", \
			__func__, cur_time.tv_sec, timer_flag->timer_val, timer_event_type);

	switch(timer_event_type)
	{
	case engine_start_after_15min:
		timer_flag->bits.engine_start_timer_stat = 0;
		timer_flag->bits.engine_start_afer_15min_flag = 0;
		break;

	case brake_active_after_20s:
		timer_flag->bits.brake_active_timer_stat = 0;
		timer_flag->bits.brake_active_after_20s_flag = 0;
		break;

	case driver_door_close_after_15min:
		timer_flag->bits.driver_door_close_timer_stat = 0;
		timer_flag->bits.driver_door_close_after_15min_flag = 0;
		break;

	case turning_light_active_after_20s:
		timer_flag->bits.turning_light_active_timer_stat = 0;
		timer_flag->bits.turning_light_active_after_20s_flag = 0;
		break;

	case accelerator_active_after_20s:
		timer_flag->bits.accelerator_active_timer_stat = 0;
		timer_flag->bits.accelerator_active_after_20s_flag = 0;
		break;

	case OK_Switch_timer_3s:
		timer_flag->timer_val = 0;
		printf("OK_Switch 3S timeout! \n");
		break;

		/* added on 05-21 */
	case level2_closing_eye_timer_1s:
		timer_flag->timer_val = 2;
		serial_output_var.close_eye_time += 1;
		printf("serial_output_var.close_eye_time: %d\n", serial_output_var.close_eye_time);
		break;
	}
}
