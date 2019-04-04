/*
 * SerialCommuAndBtMain.c
 *
 *  Created on: Apr 3, 2019
 *      Author: tony
 */

#include "serial_pack_parse.h"
#include "serial_port_commu.h"
#include "bootloader.h"
#include "producer_consumer_shmfifo.h"

//shmfifo *p_shmfifo = shmfifo_init(10000, 2000);
shmfifo *pRecvComFifo = NULL;
shmfifo *pSendComFifo = NULL;

int main(int argc, char* argv[])
{
	int i  = 0, retry_cnt = 0;
	int fd = 0;
	fd_set rfds;
	int recv_length, spec_recv_len;
	unsigned short ret_send_length = 0, spec_send_length = 0;
	unsigned char serial_recv_buf[256];
	unsigned char send_buf[256];
	int send_buf_len;
	struct timeval tp;
	unsigned short check_sum = 0;

	struct timeval tv = {
			.tv_sec = 1,
			.tv_usec = 300000,
	};

	if((fd = open_set_serial_port()) < 0)
	{
		DEBUG_INFO("open internal serial com error!\n");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	if(!bootloader_logic_init(&JMC_bootloader_logic))
	{
		DEBUG_INFO("bootloader_logic_init error!\n");
		return -1;
	}

	pRecvComFifo = shmfifo_init(10000, 1024);
	pSendComFifo = shmfifo_init(10000, 512);

	if(pRecvComFifo == (void*)-1)
	{
		DEBUG_INFO("pRecvComFifo init error!\n");
		return -1;
	}

	if(pSendComFifo == (void*)-1)
	{
		DEBUG_INFO("pSendComFifo init error!\n");
		return -1;
	}

	while(true)
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		if(select(fd+1, &rfds, NULL, NULL, &tv) > 0)
		{
			/*read serial data from rs232 */
			if((recv_length = read_one_frame(fd, serial_recv_buf, &spec_recv_len)) > 0)
			{
				retry_cnt = 0;
				serial_commu_recv_state = 0;

				gettimeofday(&tp, NULL);
				printf("%ld ms recv_length is: %d data:", (tp.tv_sec*1000+tp.tv_usec/1000), recv_length);

				for(i=0; i<recv_length; i++ )
				{
					printf("%02X", serial_recv_buf[i]);
				}

			    printf("\n");

				if(D6_MESSAGE == *(serial_recv_buf + MESSAGE_TYPE_ID))
				{
					bootloader_main_process(&JMC_bootloader_logic, serial_recv_buf+MESSAGE_TYPE_ID+1, \
							recv_length, (send_buf+MESSAGE_VAR_NUM), &ret_send_length);
					ret_send_length = ret_send_length + 7;
					*(send_buf+0) = 0xAA;
					*(send_buf+1) = 0x55;
					*(send_buf+2) = GET_HIG_BYTE_FROM_WORD(ret_send_length);
					*(send_buf+3) = GET_LOW_BYTE_FROM_WORD(ret_send_length);
					*(send_buf+4) = GET_HIG_BYTE_FROM_WORD(~ret_send_length);
					*(send_buf+5) = GET_LOW_BYTE_FROM_WORD(~ret_send_length);
					*(send_buf+6) = D6_MESSAGE;
					check_sum = calc_check_sum(send_buf+2, ret_send_length-2);
					*(send_buf+ret_send_length) = GET_HIG_BYTE_FROM_WORD(check_sum);
					*(send_buf+ret_send_length+1) = GET_LOW_BYTE_FROM_WORD(check_sum);
					ret_send_length += 2;

					send_spec_len_data(fd, ret_send_length, &ret_send_length);
				}
				else
				{
					shmfifo_put(pRecvComFifo, serial_recv_buf, spec_recv_len);
				}


			}
			else
			{
				goto recv_error;
			}
		}
		else
		{
recv_error:
			if(retry_cnt++ > 1000)
			{
				serial_commu_recv_state = -1;
				tcflush(fd, TCIFLUSH);
				retry_cnt = 0;
			}
			else
			{
				usleep(10000);
			}
		}
	}

	return 0;
}
