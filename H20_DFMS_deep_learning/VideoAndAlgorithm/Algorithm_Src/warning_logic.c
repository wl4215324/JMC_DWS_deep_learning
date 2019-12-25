/*
 * warning_logic.c
 *
 *  Created on: May 28, 2019
 *      Author: tony
 */

#include "warning_logic.h"

unsigned char DFMS_health_state = 0;
extern_signal_flags CAN_signal_flags = {0x3f};
DFMS_state DFMS_State = CLOSE;


void warning_logic_state_machine(unsigned char fault_or_normal, extern_signal_flags signal_flags,\
		DFMS_state *state_machine)
{
	if(signal_flags.bits.power_mode)
	{
		*state_machine = CLOSE;
		return ;
	}

	switch(*state_machine)
	{
	case ACTIVE:
		if(fault_or_normal)  //fault happened
		{
			*state_machine = FAULT;
		}
		else if(signal_flags.signal_state)  //not all of external signals meet requirement
		{
			*state_machine = STANDBY;
		}
		break;

	case STANDBY:
		if(fault_or_normal)  //fault happened
		{
			*state_machine = FAULT;
		}
		else if(!signal_flags.signal_state)  //all of external signals meet requirement
		{
			*state_machine = ACTIVE;
		}
		break;


	case CLOSE:
	case FAULT:
		if(!fault_or_normal)  //no fault happened
		{
			*state_machine = STANDBY;
		}
		break;

	default:
		break;
	}
}
