/*
 * eol_funct_test.c
 *
 *  Created on: Jan 18, 2019
 *      Author: tony
 */

#include "eol_funct_test.h"

EOL_working_state eol_working_state = EOL_IDLE;
EOL_Testing_Result eol_testing_ret = FACE_NOT_FOUND;

void EOL_routine_ctl(unsigned char *recv_buf, unsigned short recv_buf_len, \
		unsigned char *send_buf, unsigned short *send_buf_len)
{
	int i = 0;

	if(!recv_buf || recv_buf_len < 4)
	{
		DEBUG_INFO(recv_buf_len: %d\n, recv_buf_len);
		*send_buf = 0x7F;
		*(send_buf+1) = 0x31;
		*(send_buf+2) = 0x13; //negative code: message length error
		*send_buf_len = 3;

		for(i=0; i<recv_buf_len; i++)
			printf("  %X", *(recv_buf+i));
		printf("\n");

		return ;
	}

	if(START == *(unsigned char*)(recv_buf+1)) /* EOL routine start */
	{
		eol_working_state = EOL_WORKING;
		*send_buf = 0x71;
		*(send_buf+1) = START;
		*(send_buf+2) = (EOL_ROUTINE_ID&0xff00)>>8;
		*(send_buf+3) = EOL_ROUTINE_ID&0xff;
		*(send_buf+4) = 0x00;
		*send_buf_len = 5;
	}
	else if(STOP == *(unsigned char*)(recv_buf+1)) // EOL routine stop
	{
		/* if the previous state is idle, reply negative message */
		if(EOL_IDLE == eol_working_state)
		{
			*send_buf = 0x7F;
			*(send_buf+1) = 0x31;
			*(send_buf+2) = 0x24; //negative code: sequence error
			*send_buf_len = 3;
		}
		else if(EOL_WORKING == eol_working_state)
		{
			eol_working_state = EOL_IDLE;
			*send_buf = 0x71;
			*(send_buf+1) = STOP;
			*(send_buf+2) = (EOL_ROUTINE_ID&0xff00)>>8;
			*(send_buf+3) = EOL_ROUTINE_ID&0xff;
			*(send_buf+4) = 0x00;
			*send_buf_len = 5;
		}
	}
	else if(QUERY == *(unsigned char*)(recv_buf+1)) // EOL routine query
	{
		/* if the previous state is idle, reply negative message */
		if(EOL_IDLE == eol_working_state)
		{
			*send_buf = 0x7F;
			*(send_buf+1) = 0x31;
			*(send_buf+2) = 0x24; //negative code: sequence error
			*send_buf_len = 3;
		}
		else if(EOL_WORKING == eol_working_state)
		{
			*send_buf = 0x71;
			*(send_buf+1) = QUERY;
			*(send_buf+2) = (EOL_ROUTINE_ID&0xff00)>>8;
			*(send_buf+3) = EOL_ROUTINE_ID&0xff;
			*(send_buf+4) = eol_testing_ret;
			*send_buf_len = 5;
		}
	}
	else // unsupported sub function
	{
		*send_buf = 0x7F;
		*(send_buf+1) = 0x31;
		*(send_buf+2) = 0x12; //negative code: unsupported sub function
		*send_buf_len = 3;
	}
}
