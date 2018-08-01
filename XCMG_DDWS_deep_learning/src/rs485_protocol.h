/*
 * rs485_protocol.h
 *
 *  Created on: Jul 30, 2018
 *      Author: tony
 */

#ifndef RS485_PROTOCOL_H_
#define RS485_PROTOCOL_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "applicfg.h"


#define RS485_MESSAGE_ID_OFFSET  0
#define RS485_MESSAGE_ID_SIZE    4

#define RS485_MESSAGE_LENG_OFFSET  4
#define RS485_MESSAGE_LENG_SIZE    2

#define RS485_MESSAGE_TYPE_OFFSET  6
#define RS485_MESSAGE_TYPE_SIZE    2

#define RS485_MESSAGE_SN_OFFSET  8
#define RS485_MESSAGE_SN_SIZE    2

#define RS485_MESSAGE_CONT_OFFSET  10

extern unsigned char rs485_warning_status;

extern unsigned char rs485_recv_buf[256];
extern unsigned char rs485_send_buf[256];

extern int reply_master_message(const unsigned char *recv_buf, unsigned short recv_buf_leng, \
		unsigned char warning_status, unsigned char *send_buf, unsigned short *send_buf_leng);

extern int read_one_rs485_frame(int fd, unsigned char* recv_buff, int* recv_frame_leng);

extern int send_rs485_warning_status(unsigned char warning_status, unsigned char *send_buf, \
	unsigned short *send_buf_leng);
#endif /* RS485_PROTOCOL_H_ */
