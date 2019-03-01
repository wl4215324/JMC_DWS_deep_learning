/*
 * eol_funct_test.h
 *
 *  Created on: Jan 18, 2019
 *      Author: tony
 */

#ifndef EOL_FUNCT_TEST_H_
#define EOL_FUNCT_TEST_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "applicfg.h"


#define  EOL_ROUTINE_ID  0x5A00

typedef enum {
	START = 1,
	STOP,
	QUERY,
} EOL_command;

typedef enum {
	EOL_IDLE = 1,
	EOL_WORKING,
} EOL_working_state;

typedef enum {
	FACE_NOT_FOUND,
	FACE_FOUND,
} EOL_Testing_Result;


extern EOL_working_state eol_working_state;
extern EOL_Testing_Result eol_testing_ret;

extern void EOL_routine_ctl(unsigned char *recv_buf, unsigned short recv_buf_len, \
		unsigned char *send_buf, unsigned short *send_buf_len);

#endif /* EOL_FUNCT_TEST_H_ */


