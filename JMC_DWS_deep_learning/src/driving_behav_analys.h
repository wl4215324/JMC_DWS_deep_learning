/*
 * driving_behav_analys.h
 *
 *  Created on: Nov 25, 2017
 *      Author: tony
 */

#ifndef DRIVING_BEHAV_ANALYS_H_
#define DRIVING_BEHAV_ANALYS_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#define VEHICLE_SPEED_THRESHOLD  50

typedef union{
	unsigned short timer_val;
	struct
	{
		unsigned char engine_start_afer_15min_flag:1;
		unsigned char brake_active_after_20s_flag:1;
		unsigned char driver_door_close_after_15min_flag:1;
	    unsigned char turning_light_active_after_20s_flag:1;
	    unsigned char accelerator_active_after_20s_flag:1;
	    unsigned char : 0;
	    unsigned char engine_start_timer_stat:1;
		unsigned char brake_active_timer_stat:1;
		unsigned char driver_door_close_timer_stat:1;
	    unsigned char turning_light_active_timer_stat:1;
	    unsigned char accelerator_active_timer_stat:1;
	}bits;

} TimerFlag;


typedef enum{
	engine_start_after_15min = 0,
	brake_active_after_20s,
	driver_door_close_after_15min,
	turning_light_active_after_20s,
	accelerator_active_after_20s,
	OK_Switch_timer_3s
} TimerEventType;

extern TimerFlag timer_flag ;

extern TimerFlag OK_Switch_timer_flag ;

extern void timeout_execute_activity(TimerFlag* timer_flag, TimerEventType timer_event_type);


#endif /* DRIVING_BEHAV_ANALYS_H_ */
