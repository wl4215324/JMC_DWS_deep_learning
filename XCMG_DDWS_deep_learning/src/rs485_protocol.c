/*
 * rs485_protocol.c
 *
 *  Created on: Jul 30, 2018
 *      Author: tony
 */

#include "rs485_protocol.h"
#include "serial_pack_parse.h"

unsigned char rs485_recv_buf[256];
unsigned char rs485_send_buf[256];
unsigned char rs485_warning_status = 0;

const static unsigned char rs485_mesg_head[4] = {0xAA, 0x55, 0xCC, 0x33};

const static unsigned char master_mesg_struct[7][4] = {
		{0xAC, 0xC1, 0x00, 0x00},  //read system parameters
		{0xAC, 0xC2, 0x00, 0x00},  //write system parameters
		{0xAC, 0xC6, 0x00, 0x00},  //read system version
		{0xAC, 0xC8, 0x00, 0x00},  //read warning status 1
		{0xAC, 0xC9, 0xFF, 0xFF},  //read warning picture
		{0xAC, 0xCA, 0xFF, 0xFF},  //grab current picture
		{0xAC, 0xCB, 0x00, 0x00}   //read warning status 2
};


static unsigned short g_McRctable_16[256] =
{
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,

	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,

	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,

	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,

	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,

	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,

	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,

	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,

	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,

	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,

	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,

	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,

	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,

	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,

	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,

	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,

	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,

	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,

	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,

	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,

	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,

	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,

	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,

	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,

	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,

	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,

	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,

	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,

	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,

	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,

	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,

	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

static unsigned short getCRC16(unsigned char *pdata, int len)
{
	unsigned short cRc_16 = 0xFFFF;
	unsigned char temp;

	while(len-- > 0)
	{
		temp = cRc_16 & 0xFF;
		cRc_16 = (cRc_16 >> 8) ^ g_McRctable_16[(temp ^ *pdata++) & 0xFF];
	}

	return cRc_16;
}

int send_rs485_warning_status(unsigned char warning_status, \
		unsigned char *send_buf, unsigned short *send_buf_leng)
{
	unsigned short CRC_16 = 0;
	*send_buf_leng = 0;

	memcpy(send_buf+RS485_MESSAGE_ID_OFFSET, rs485_mesg_head, RS485_MESSAGE_ID_SIZE);
	*send_buf_leng += RS485_MESSAGE_ID_SIZE;

	*(send_buf+RS485_MESSAGE_LENG_OFFSET) = 0;
	*(send_buf+RS485_MESSAGE_LENG_OFFSET+1) = 13;
	*send_buf_leng += RS485_MESSAGE_LENG_SIZE;

	memcpy(send_buf+RS485_MESSAGE_TYPE_OFFSET, master_mesg_struct[6], 4);
	*send_buf_leng += 4;

	*(send_buf+RS485_MESSAGE_CONT_OFFSET) = warning_status;
	*send_buf_leng += 1;

	CRC_16 = getCRC16(send_buf, *send_buf_leng);
	*(send_buf+RS485_MESSAGE_CONT_OFFSET+1) = (unsigned char)(CRC_16>>8);
	*(send_buf+RS485_MESSAGE_CONT_OFFSET+2) = (unsigned char)(CRC_16&0xff);
	*send_buf_leng += 2;

	return 0;
}

int reply_master_message(const unsigned char *recv_buf, unsigned short recv_buf_leng, \
		unsigned char warning_status, unsigned char *send_buf, unsigned short *send_buf_leng)
{
	unsigned short CRC_16 = 0;

	if(!recv_buf || (12 > recv_buf_leng))
	{
		return -1;
	}

	memcpy(send_buf+RS485_MESSAGE_ID_OFFSET, rs485_mesg_head, RS485_MESSAGE_ID_SIZE);
	*send_buf_leng += RS485_MESSAGE_ID_SIZE;

	if(0xAC == *(recv_buf+RS485_MESSAGE_TYPE_OFFSET))
	{
		switch(*(recv_buf+RS485_MESSAGE_TYPE_OFFSET+1))
		{
		case 0xC1:  //read system parameters
			memcpy(send_buf+RS485_MESSAGE_TYPE_OFFSET, master_mesg_struct[0], 4);
			*send_buf_leng += 4;

			memcpy(send_buf+RS485_MESSAGE_CONT_OFFSET, recv_buf+RS485_MESSAGE_CONT_OFFSET, 2);
			*send_buf_leng += 2;
			break;

		case 0xC2:  //write system parameters
			memcpy(send_buf+RS485_MESSAGE_TYPE_OFFSET, master_mesg_struct[1], 4);
			*send_buf_leng += 4;
			CRC_16 = getCRC16(send_buf, *send_buf_leng);

			*(send_buf+RS485_MESSAGE_CONT_OFFSET) = (unsigned char)(CRC_16>>8);
			*(send_buf+RS485_MESSAGE_CONT_OFFSET+1) = (unsigned char)(CRC_16&0xff);
			*send_buf_leng += 2;
			break;

		case 0xC6:  //read system version
			memcpy(send_buf+RS485_MESSAGE_TYPE_OFFSET, master_mesg_struct[2], 4);
			*send_buf_leng += 4;

			memcpy(send_buf+RS485_MESSAGE_CONT_OFFSET, recv_buf+RS485_MESSAGE_CONT_OFFSET, 2);
			*send_buf_leng += 2;
			break;

		case 0xC8:  //read warning status 1
			memcpy(send_buf+RS485_MESSAGE_TYPE_OFFSET, master_mesg_struct[3], 4);
			*send_buf_leng += 4;

			*(send_buf+RS485_MESSAGE_CONT_OFFSET) = warning_status;
			*send_buf_leng += 1;

			CRC_16 = getCRC16(send_buf, *send_buf_leng);
			*(send_buf+RS485_MESSAGE_CONT_OFFSET+1) = (unsigned char)(CRC_16>>8);
			*(send_buf+RS485_MESSAGE_CONT_OFFSET+2) = (unsigned char)(CRC_16&0xff);
			*send_buf_leng += 2;
			break;

		case 0xC9:  //read warning picture
			memcpy(send_buf+RS485_MESSAGE_TYPE_OFFSET, master_mesg_struct[4], 4);
			*send_buf_leng += 4;

			memcpy(send_buf+RS485_MESSAGE_CONT_OFFSET, recv_buf+RS485_MESSAGE_CONT_OFFSET, 2);
			*send_buf_leng += 2;
			break;

		case 0xCA:  //grab current picture
			memcpy(send_buf+RS485_MESSAGE_TYPE_OFFSET, master_mesg_struct[5], 4);
			*send_buf_leng += 4;

			memcpy(send_buf+RS485_MESSAGE_CONT_OFFSET, recv_buf+RS485_MESSAGE_CONT_OFFSET, 2);
			*send_buf_leng += 2;
			break;

		case 0xCB:  //read warning status 2
			memcpy(send_buf+RS485_MESSAGE_TYPE_OFFSET, master_mesg_struct[6], 4);
			*send_buf_leng += 4;

			*(send_buf+RS485_MESSAGE_CONT_OFFSET) = warning_status;
			*send_buf_leng += 1;

			CRC_16 = getCRC16(send_buf, *send_buf_leng);
			*(send_buf+RS485_MESSAGE_CONT_OFFSET+1) = (unsigned char)(CRC_16>>8);
			*(send_buf+RS485_MESSAGE_CONT_OFFSET+2) = (unsigned char)(CRC_16&0xff);
			*send_buf_leng += 2;
			break;
		}
	}
	else
	{
		return -1;
	}

	return 0;
}


/*
 * read one intact data frame from external rs485 serial port
 */
int read_one_rs485_frame(int fd, unsigned char* recv_buff, int* recv_frame_leng)
{
	int i = 0, j;
	int retry_cnt = 0;
	int retval = 0;
	unsigned char temp_buf[10];
	unsigned char message_type, var_cnt;
	unsigned short temp_var;
	unsigned short message_len = 0, crc16_cal = 0, crc16_recv = 0;
	*recv_frame_leng = 0;

	while(1)
	{
		if(i < 10)
		{
			retval = read_spec_len_data(fd, temp_buf, sizeof(temp_buf));

			if(retval < 0)
			{
				return -1;
			}
			else if(retval == 0)
			{
				if(retry_cnt++ > 10)
				{
					return -1;
				}
				else
				{
					continue;
					usleep(10000);
				}
			}
			else if(retval >= 10)
			{
				retry_cnt = 0;

				if(*(int*)(temp_buf) == *(int*)(rs485_mesg_head))
				{
					message_len = *(temp_buf+RS485_MESSAGE_LENG_OFFSET);
					message_len <<=8;
					message_len |= (*(temp_buf+RS485_MESSAGE_LENG_OFFSET+1));
					i = 10;
					*recv_frame_leng = 10;
					memcpy(recv_buff, temp_buf, *recv_frame_leng);
				}
			}
			else
			{
				return -1;
			}
		}
		else
		{
			retval = read_spec_len_data(fd, recv_buff+RS485_MESSAGE_CONT_OFFSET, (message_len-10));

			if(retval < 0)
			{
				return -1;
			}
			else if(retval == 0)
			{
				if(retry_cnt++ > 10)
				{
					return -1;
				}
				else
				{
					continue;
					usleep(10000);
				}
			}
			else if(retval >= (message_len-10))
			{
				crc16_recv = *(recv_buff+message_len-2);
				crc16_recv <<= 8;
				crc16_recv |= *(recv_buff+message_len-1);

				crc16_cal = getCRC16(recv_buff, message_len-2);

				if(crc16_recv == crc16_cal)
				{
					*recv_frame_leng = message_len;
					return *recv_frame_leng;
				}
				else
				{
					return -1;
				}
			}
			else
			{
				return -1;
			}
		}
	}

	return -1;
}


