/*
 * SerialCommuAndBtMain.c
 *
 *  Created on: Apr 3, 2019
 *      Author: tony
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include "serial_pack_parse.h"
#include "serial_port_commu.h"
#include "bootloader.h"
#include "producer_consumer_shmfifo.h"

//shmfifo *p_shmfifo = shmfifo_init(10000, 2000);


int main(int argc, char* argv[])
{
	int i  = 0, retry_cnt = 0;
	int read_ret = 0;
	int fd = 0;
	fd_set rfds;
	int recv_length, spec_recv_len;
	int ret_send_length = 0;
	unsigned char recv_buf[512];
	unsigned char send_buf[256];
	struct timeval tp;

	struct timeval tv = {
			.tv_sec = 3,
			.tv_usec = 500000,
	};

	if((fd = open_set_serial_port()) < 0)
	{
		DEBUG_INFO(open internal serial com error!\n);
		return -1;
	}
	else
	{
		DEBUG_INFO(open_set_serial_port success!\n);
	}

	tcflush(fd, TCIOFLUSH);

	if(bootloader_logic_init(&JMC_bootloader_logic))
	{
		DEBUG_INFO(bootloader_logic_init error!\n);
		return -1;
	}
	else
	{
		DEBUG_INFO(bootloader_logic_init success!\n);
	}

	pRecvComFifo = shmfifo_init(RECV_BUF_SHM_KEY, RECV_BUF_SIZE);
	pSendComFifo = shmfifo_init(SEND_BUF_SHM_KEY, SEND_BUF_SIZE);

	if((pRecvComFifo == (void*)-1) || !pRecvComFifo)
	{
		DEBUG_INFO(pRecvComFifo init error!\n);
		return -1;
	}
	else
	{
		DEBUG_INFO(pRecvComFifo init success!\n);
	}

	if((pSendComFifo == (void*)-1) || !pRecvComFifo)
	{
		DEBUG_INFO(pSendComFifo init error!\n);
		return -1;
	}
	else
	{
		DEBUG_INFO(pSendComFifo init success!\n);
	}

	while(true)
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		if((read_ret = select(fd+1, &rfds, NULL, NULL, &tv)) > 0)
		{
			/*read serial data from rs232 */
			if((recv_length = read_one_frame(fd, recv_buf, &spec_recv_len)) > 0)
			{
				retry_cnt = 0;
				serial_commu_recv_state = 0;
				gettimeofday(&tp, NULL);
				printf("%ld ms recv_length is: %d data:", (tp.tv_sec*1000+tp.tv_usec/1000), recv_length);

				for(i=0; i<recv_length; i++ )
				{
					printf("%02X", recv_buf[i]);
				}

			    printf("\n");

			    if(D2_MESSAGE == *(recv_buf + MESSAGE_TYPE_ID))  // type D2 message processing
				{
					DEBUG_INFO(spec_recv_len: %d  shmfifo left size: %d\n, \
							spec_recv_len, shmfifo_left_size(pRecvComFifo));

					shmfifo_put(pRecvComFifo, recv_buf, spec_recv_len);

					ret_send_length = D2_MESSAGE_LENGTH;

					DEBUG_INFO(shmfifo_get length: %d\n, shmfifo_get(pSendComFifo, send_buf, ret_send_length));
					send_spec_len_data(fd, send_buf, ret_send_length);

					gettimeofday(&tp, NULL);
					printf("%ld ms send_length is: %d data:", (tp.tv_sec*1000+tp.tv_usec/1000), ret_send_length);

					for(i=0; i<ret_send_length; i++ )
					{
						printf("%02X", send_buf[i]);
					}

					printf("\n");
				}
			    else if((D6_MESSAGE == *(recv_buf + MESSAGE_TYPE_ID)) || \
				   (D3_MESSAGE == *(recv_buf + MESSAGE_TYPE_ID)) || \
				   (D5_MESSAGE == *(recv_buf + MESSAGE_TYPE_ID)) )
				{
					parse_recv_pack_send(recv_buf, spec_recv_len, send_buf, &ret_send_length);
					send_spec_len_data(fd, send_buf, ret_send_length);
				}
			}
		}
		else if(0 == read_ret)
		{
			DEBUG_INFO(serial port receiving timeout!\n);

			ret_send_length = D2_MESSAGE_LENGTH;

			if(shmfifo_len(pSendComFifo) >= ret_send_length)
			{
				shmfifo_get(pSendComFifo, send_buf, ret_send_length);
				send_spec_len_data(fd, send_buf, ret_send_length);
			}

			if(ret_send_length > 0)
			{
				gettimeofday(&tp, NULL);
				DEBUG_INFO(%d ms send_length is: %d data:, (tp.tv_sec*1000+tp.tv_usec/1000), ret_send_length);

				for(i=0; i<ret_send_length; i++ )
				{
					printf("%02X", send_buf[i]);
				}

				printf("\n");
			}

			if(retry_cnt++ > 1000)
			{
				retry_cnt = 0;
				serial_commu_recv_state = -1;
				tcflush(fd, TCIFLUSH);
			}
			else
			{
				usleep(300000);
			}
		}
		else
		{
			DEBUG_INFO(serial port receiving error!\n);
			serial_commu_recv_state = -1;
			usleep(200000);
		}
	}

	return 0;
}
