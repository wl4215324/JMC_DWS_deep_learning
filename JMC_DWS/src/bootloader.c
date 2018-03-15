/*
 * bootloader.c
 *
 *  Created on: Jan 31, 2018
 *      Author: tony
 */

#include "bootloader.h"
#include "serial_pack_parse.h"

Seed seed = {
		.level_one = 0,
		.level_FBL = 0
};

SecretKey secret_key = {
		.level_one = 0,
		.level_FBL = 0
};


struct list_head driver_file_list;
struct list_head app_file_list;


void init_bootloader()
{
	INIT_LIST_HEAD(&driver_file_list);
	INIT_LIST_HEAD(&app_file_list);
}



/*
 *
 */
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
		/* check programming integrity  0x31 0x01 0x02 0x02 */
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



/*
 *  The following function is used to deal with diagnostic session type
 */
int diagnose_session_ctrl(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	if((DIAG_SESS_CTRL  == *can_mesg) && (mesg_len >= 2))
	{
		*reply_mesg = 0x50;  //positive response code
		*reply_mesg_len += 1;

		switch(*(can_mesg+1))
		{
		case 0x01:  //default session request
			*(reply_mesg+1) = 0x01;
			*reply_mesg_len += 1;
			memset(reply_mesg+2, 0, 4);
			*reply_mesg_len += 4;
			break;

		case 0x02:  //programming session request
			*(reply_mesg+1) = 0x02;
			*reply_mesg_len += 1;
			memset(reply_mesg+2, 0, 4);
			*reply_mesg_len += 4;
			break;

		case 0x03:  //extended diagnostic session  request
			*(reply_mesg+1) = 0x03;
			*reply_mesg_len += 1;
			memset(reply_mesg+2, 0, 4);
			*reply_mesg_len += 4;
			break;

		default:  // other type's request
			*reply_mesg = 0x7F;  //negative response
			*(reply_mesg+1) = 0x10;
			*(reply_mesg+2) = 0x12;
			*reply_mesg_len += 2;
			break;
		}

		return 0;
	}
	else
	{
		*reply_mesg = 0x7F;  //negative response request
		*(reply_mesg+1) = 0x10;
		*(reply_mesg+2) = 0x12;
		*reply_mesg_len = 3;

		return -1;
	}
}


/*
 *
 */
int control_DTC_setting(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	if((CONTROL_DTC_SETTING == *can_mesg) && (mesg_len >= 2))
	{
		if((0x01 == *(can_mesg+1)) || (0x02 == *(can_mesg+1)))  //0x01: "on", 0x02: "off"
		{
			*reply_mesg = 0xC5;
			*(reply_mesg+1) = *(can_mesg+1);
			*reply_mesg_len += 2;
		}
		else
		{
			*reply_mesg = 0x7F;
			*(reply_mesg+1) = 0x85;
			*(reply_mesg+2) = 0x12;  //not supported sub function
			*reply_mesg_len += 3;
		}

		return 0;
	}
	else
	{
		*reply_mesg = 0x7F;
		*(reply_mesg+1) = 0x85;
		*(reply_mesg+2) = 0x13;  //not supported sub function
		*reply_mesg_len += 3;
		return -1;
	}
}


/*
 * The following function is used to deal with secure access
 */
int secure_access(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	unsigned int temp_key = 0;

	if((SECURE_ACCESS == *can_mesg) && (mesg_len >= 2))
	{
		switch(*(can_mesg+1))
		{
		case 0x01:  //request seed to reach security level: unlocked(level 1)
			*reply_mesg = 0x67;
			*(reply_mesg+1) = 0x01;
			*(reply_mesg+2) = (seed.level_one&0xF000)>>24;
			*(reply_mesg+3) = (seed.level_one&0x0F00)>>16;
			*(reply_mesg+4) = (seed.level_one&0x00F0)>>8;
			*(reply_mesg+5) = seed.level_one&0x000F;
			*reply_mesg_len += 6;
			break;

		case 0x02:  //request key to reach security level: unlocked(level 1)
			temp_key = (*(can_mesg+2)<<24|*(can_mesg+3)<<16|*(can_mesg+4)<<8|*(can_mesg+5));
			*reply_mesg = 0x67;
			*(reply_mesg+1) = 0x02;
			*reply_mesg_len += 2;
			break;

		case 0x09:  //request seed to reach security level: unlocked(flash, level FBL)
			*reply_mesg = 0x67;
			*(reply_mesg+1) = 0x02;
			*(reply_mesg+2) = (seed.level_FBL&0xF000)>>24;
			*(reply_mesg+3) = (seed.level_FBL&0x0F00)>>16;
			*(reply_mesg+4) = (seed.level_FBL&0x00F0)>>8;
			*(reply_mesg+5) = seed.level_FBL&0x000F;
			*reply_mesg_len += 6;
			break;

		case 0x0A:  //request key to reach security level: unlocked(flash, level FBL)
			temp_key = (*(can_mesg+2)<<24|*(can_mesg+3)<<16|*(can_mesg+4)<<8|*(can_mesg+5));
			*reply_mesg = 0x67;
			*(reply_mesg+1) = 0x0A;
			*reply_mesg_len += 2;
			break;

		default:
			*reply_mesg = 0x7F;
			*(reply_mesg+1) = 0x27;
			*(reply_mesg+2) = 0x12;  //not supported sub function
			*reply_mesg_len += 3;
			break;
		}

		return 0;
	}
	else  //negative response
	{
		*reply_mesg = 0x7F;
		*(reply_mesg+1) = 0x27;
		*(reply_mesg+2) = 0x13;  //length or format error
		*reply_mesg_len += 3;

		return 0;
	}

	return -1;
}

/*
 * The following function is used to deal with request download(0x34)
 */
int request_download(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	LogicBlock *tmp = NULL;


	/*if receiving can message is downloading request */
	if((0x34 == *can_mesg) && (0x00 == *(can_mesg+1)) && (0x44 == *(can_mesg+2)) \
			&& (mesg_len >= 11))
	{
		if(!list_empty(&driver_file_list))  //if driver file list is empty
		{
			tmp = (LogicBlock*)malloc(sizeof(LogicBlock));  //create driver file logic block

			if(NULL == tmp)  //create memory for driver file logic block error
			{
				*reply_mesg = 0x7F;  //negative response
				*(reply_mesg+1) = 0x34;
				*(reply_mesg+2) = 0x70; //negative response code
				*reply_mesg_len = 3;
				return -1;
			}
			else  //create memory for driver file logic block successfully
			{
				tmp->file_type = 0;  //type code for driver file
				tmp->MaxNumOfBlockLeng = MAX_NUMBER_OF_BLOCK_LENG;
				tmp->block_index = 0;
				tmp->block_state = RequestDownload;
				tmp->crc32_cal = 0;
				memset(&tmp->finger_print, 0, sizeof(FingerPrint));
				tmp->mem_addr = 0;
				tmp->mem_size = 0;
				INIT_LIST_HEAD(&tmp->segment_list_head);
				list_add_tail(&tmp->block_list, &driver_file_list);

				*reply_mesg = 0x34;  //positive response
				*(reply_mesg+1) = 0x00;  //length format identifier
				*(reply_mesg+2) = ((MAX_NUMBER_OF_BLOCK_LENG & 0xf0)>>8);  //high byte for max number of block length
				*(reply_mesg+3) = MAX_NUMBER_OF_BLOCK_LENG & 0x0f;  //low byte for max number of block length
				*reply_mesg_len = 4;
				return 0;
			}
		}
		else  //if driver file file is not empty
		{
			if(!list_empty(&app_file_list))  //if application file list is empty
			{

			}
			else
			{
				//(app_file_list.prev->prev
				//list_entry(app_file_list.prev, , );
			}
		}
	}
	else /*if receiving can message did not match downloading request format*/
	{
		*reply_mesg = 0x7F;  //negative response
		*(reply_mesg+1) = 0x34;
		*(reply_mesg+2) = 0x13; //negative response code
		*reply_mesg_len = 3;
		return -1;
	}

	return -1;
}

/*
 * the following function is used to receive boot loader messages from CAN bus, and then perform some
 * actions guided by CAN message, finally reply CAN messages to diagnostic apparatus.
 *
 */
void bootloader_can_message_processing(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	switch(*can_mesg)
	{
	case DIAG_SESS_CTRL:  //for diagnostic session control
		diagnose_session_ctrl(can_mesg, mesg_len, reply_mesg, reply_mesg_len);
		break;

	case ECU_RESET:  //for ECU reset
		break;

	case CLEAR_DIAG_INFO:  //for clear diagnostic information
		break;

	case READ_DTC_INFO:  //for read DTC information
		break;

	case READ_DAT_BY_ID:  //for read data by ID
		break;

	case READ_MEM_BY_ADDR:
		break;

	case SECURE_ACCESS:
		secure_access(can_mesg, mesg_len, reply_mesg, reply_mesg_len);
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
		request_download(can_mesg, mesg_len, reply_mesg, reply_mesg_len);
		break;

	case TRANSFER_DATA:
		break;

	case REQ_TRANSFER_DAT_EXIT:
		break;

	case WRITE_MEM_BY_ID:
		break;

	case CONTROL_DTC_SETTING:
		control_DTC_setting(can_mesg, mesg_len, reply_mesg, reply_mesg_len);
		break;
	}
}

