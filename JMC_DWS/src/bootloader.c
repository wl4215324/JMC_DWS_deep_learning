/*
 * bootloader.c
 *
 *  Created on: Jan 31, 2018
 *      Author: tony
 */

#include "bootloader.h"
#include "serial_pack_parse.h"


int routine_ctrl(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	unsigned int recv_crc32 = 0;
	unsigned int calc_crc32 = 0;

	if(ROUTINE_CTRL != *can_mesg)
	{
		return -1;
	}

	switch(*(can_mesg+1))
	{
	case 0x01:  //start routine

		/* check programming integrity  0x31 0x01 0x02 0x02*/
		if(((0x02 == *(can_mesg+2)) && (0x02 == *(can_mesg+3))))
		{
			*reply_mesg = 0x71;
			*(reply_mesg+1) = 0x01;
			*(reply_mesg+2) = 0x02;
			*(reply_mesg+3) = 0x02;

			recv_crc32 = (unsigned int)((*(can_mesg+4)>>24)|(*(can_mesg+5)>>16)|(*(can_mesg+6)>>8)|*(can_mesg+7));
			/* calculate crc32 code for buffer*/

			if(recv_crc32 == calc_crc32)
			{
				*(reply_mesg+4) = 0x00; //if receiving CRC32 is equal to calculated CRC
			}
			else
			{
				*(reply_mesg+4) = 0x01; //if receiving CRC32 is not equal to calculated CRC
			}

			*reply_mesg_len = 5;
		}
		else if((0x02 == *(can_mesg+2)) && (0x03 == *(can_mesg+3)))  //check programming precondition: 0x31 0x01 0x02 0x03
		{
			/*checking engine speed and vehicle speed whether are both zero*/
			if(((serial_input_var.engine_speed >> 3) <= 0) && ((serial_input_var.vehicle_speed>>8) <= 0))
			{
				*reply_mesg = 0x71;
				*(reply_mesg+1) = 0x01;
				*(reply_mesg+2) = 0x02;
				*(reply_mesg+3) = 0x03;
				*(reply_mesg+4) = 0x00;
				*reply_mesg_len = 0x05;
			}
			else
			{
				/*if not meet programming precondition, refuse request*/
				*reply_mesg = 0x7F;
				*(reply_mesg+1) = 0x31;
				*(reply_mesg+2) = 0x22;
				*reply_mesg_len = 0x03;
			}
		}
		else if((0xFF == *(can_mesg+2)) && (0x00 == *(can_mesg+3)))  // erase memory: 0x31 0x01 0xff 0x00
		{
			*reply_mesg = 0x71;
			*(reply_mesg+1) = 0x01;
			*(reply_mesg+2) = 0xff;
			*(reply_mesg+3) = 0x00;
			*(reply_mesg+4) = 0x00;
			*reply_mesg_len = 0x05;
		}
		else if((0xFF == *(can_mesg+2)) && (0x01 == *(can_mesg+3)))  // check programming dependence: 0x31 0x01 0xff 0x01
		{
			*reply_mesg = 0x71;
			*(reply_mesg+1) = 0x01;
			*(reply_mesg+2) = 0xff;
			*(reply_mesg+3) = 0x01;
			*(reply_mesg+4) = 0x00;
			*reply_mesg_len = 0x05;
		}
		else
		{
			/* can message format error */
			*reply_mesg = 0x7F;
			*(reply_mesg+1) = 0x31;
			*(reply_mesg+2) = 0x13;
			*reply_mesg_len = 0x03;
		}
		break;

	case 0x02:  //stop routine
		return -1;
		break;

	case 0x03:  //request routine result
		return -1;
		break;

	default:
		return -1;
		break;
	}

	return 1;
}


int read_dat_by_id(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	if(READ_DAT_BY_ID != *can_mesg)
	{
		return -1;
	}
    /*read fingerprint information*/
	if((0xF1 == *(can_mesg+1)) && (0x5B == *(can_mesg+2)))
	{
		*reply_mesg = 0x62;
		*(reply_mesg+1) = 0xF1;
		*(reply_mesg+2) = 0x5B;
		*reply_mesg_len = 3;
	}

	return 1;
}



void bootloader_can_message_procession(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	switch(*can_mesg)
	{
	case DIAG_SESS_CTRL:
		break;

	case ECU_RESET:
		break;

	case CLEAR_DIAG_INFO:
		break;

	case READ_DTC_INFO:
		break;

	case READ_DAT_BY_ID:
		break;

	case READ_MEM_BY_ADDR:
		break;

	case SECURE_ACCESS:
		break;

	case COMMUN_CTRL:
		break;

	case READ_DAT_BY_PID:
		break;

	case DYNA_DEFI_DAT_ID:
		break;

	case WRITE_DAT_BY_ID:
		break;

	case INPUT_OUTPUT_CTRL_BY_ID:
		break;

	case ROUTINE_CTRL:
		routine_ctrl(can_mesg, mesg_len, reply_mesg, reply_mesg_len);
		break;

	case REQUEST_DOWNLOAD:
		break;

	case TRANSFER_DATA:
		break;

	case REQ_TRANSFER_DAT_EXIT:
		break;

	case WRITE_MEM_BY_ID:
		break;

	}
}

