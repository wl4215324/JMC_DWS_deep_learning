/*
 * waring_logic.h
 *
 *  Created on: May 28, 2019
 *      Author: tony
 */

#ifndef WARING_LOGIC_H_
#define WARING_LOGIC_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define  DFMS_ENABLED_SPEED  20
#define  DFMS_DISABLED_SPEED  10

typedef enum {
	CLOSE = 1,
	STANDBY,
	ACTIVE,
	FAULT,
} DFMS_state;


typedef union{
	struct {
		unsigned char vehicle_speed_flag:1;
		unsigned char turn_light_flag:1;
		unsigned char brake_flag:1;
		unsigned char door_flag:1;
		unsigned char gear_flag:1;
		unsigned char dfms_enable_flag:1;
	}bits;

	unsigned char signal_state;
} extern_signal_flags;


extern void warning_logic_state_machine(unsigned char fault_or_normal, extern_signal_flags signal_flags,\
		DFMS_state *state_machine);


extern unsigned char DFMS_health_state;
extern extern_signal_flags CAN_signal_flags;
extern DFMS_state DFMS_State;



#endif /* WARING_LOGIC_H_ */
