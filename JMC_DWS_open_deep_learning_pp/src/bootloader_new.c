/*
 * bootloader_new.c
 *
 *  Created on: Dec 4, 2018
 *      Author: tony
 */

#include "bootloader_new.h"

bootloader_core_data *bootloader_global;


/* bootloader initialization */
int bootloader_init(unsigned int size)
{
	bootloader_core_data *p = NULL;

	if(size <= 0 || size > MAX_FILE_SIZE)
	{
		return -1;
	}

	p = (bootloader_core_data*)malloc(sizeof(bootloader_core_data));

	if(!p)
	{
		return -1;
	}

	p->bootloader_state = Request_Download;
	p->recv_data = (unsigned char*)malloc(size);
	p->recv_data_length = 0;
	p->block_index = 0;
	p->prev_block_index = 0;
	p->mem_addr = 0;
	p->mem_size = 0;
	p->crc32_cal = 0xffffffffL;

	if(!p->recv_data)
	{
		free(p);
		return -1;
	}

	bootloader_global = p;
	return 0;
}

void bootloader_exit()
{
	if(bootloader_global)
	{
		if(bootloader_global->recv_data)
		{
			free(bootloader_global->recv_data);
		}

		bootloader_global->bootloader_state = Request_Download;
		bootloader_global->recv_data_length = 0;
		free(bootloader_global);
	}
}

void bootloader_reset()
{

}


static int compare_array(const unsigned char *array_one, const unsigned char *array_two,\
		unsigned int length)
{
	unsigned int i = 0;

	if(!array_one || !array_two || (0 == length))
	{
		return -1;
	}

	for(i=0; i<length; i++)
	{
		if(*(array_one+i) != *(array_two+i))
		{
			return -1;
		}
	}

	return 0;
}



void download_data_process(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	unsigned int crc32_temp = 0;

	if(!bootloader_global)
	{
		/* negative response */
		*reply_mesg = 0x7F;
		*(reply_mesg+1) = 0x36;
		*(reply_mesg+2) = 0x72; //general programming error
		*reply_mesg_len = 3;
	}
	else if((bootloader_global->bootloader_state != Request_Download) && \
			(bootloader_global->bootloader_state != Download_Data))
	{
		/* negative response */
		*reply_mesg = 0x7F;
		*(reply_mesg+1) = 0x36;
		*(reply_mesg+2) = 0x24; //request sequence error
		*reply_mesg_len = 3;
	}
	else if(bootloader_global->recv_data_length > bootloader_global->mem_size)
	{
		*reply_mesg = 0x7F;
		*(reply_mesg+1) = 0x36;
		*(reply_mesg+2) = 0x24; //request sequence error
		*reply_mesg_len = 3;
	}
	else
	{
		bootloader_global->bootloader_state = Download_Data;
		bootloader_global->block_index =  *(can_mesg+1);

		/* if block index is continuous */
		if(bootloader_global->block_index == (unsigned char)(bootloader_global->prev_block_index+1))
		{
		    memcpy((bootloader_global->recv_data+bootloader_global->recv_data_length), (can_mesg+2), mesg_len-11);
			crc32_temp = crc32c(bootloader_global->crc32_cal, \
					(bootloader_global->recv_data+bootloader_global->recv_data_length), mesg_len-11);
			bootloader_global->crc32_cal = crc32_temp;
			bootloader_global->recv_data_length += mesg_len - 11;
			bootloader_global->prev_block_index = bootloader_global->block_index;

			*reply_mesg = 0x76;
			*(reply_mesg+1) = bootloader_global->block_index;
			*reply_mesg_len = 2;
		}
		/* if block index is identical */
		else if(bootloader_global->block_index == bootloader_global->prev_block_index)
		{
			if(compare_array((bootloader_global->recv_data+bootloader_global->recv_data_length-(mesg_len-11)), \
					can_mesg+2, mesg_len-11) == 0)
			{
				*reply_mesg = 0x76;
				*(reply_mesg+1) = bootloader_global->block_index;
				*reply_mesg_len = 2;
			}
			else  //if content of consecutive block is different, reply negative response
			{
				/* negative response */
				*reply_mesg = 0x7F;
				*(reply_mesg+1) = 0x36;
				*(reply_mesg+2) = 0x73; //request sequence error
				*reply_mesg_len = 3;
			}
		}
		else
		{
			/* negative response */
			*reply_mesg = 0x7F;
			*(reply_mesg+1) = 0x36;
			*(reply_mesg+2) = 0x73; //request sequence error
			*reply_mesg_len = 3;
		}
	}
}


void request_download_process(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	unsigned char i = 0;
	unsigned char mem_addr_bytes, mem_size_bytes;

	mem_addr_bytes = *(can_mesg+2)&0x0F;  //get memory address bytes
	mem_size_bytes = (*(can_mesg+2)&0xF0)>>4;  //get memory size bytes

	/* 0x34 0x00 0x44 0xadd4 0xadd3 0xadd2 0xadd1 0xsize4 0xsize3 0xsize2 0xsize1 */
	if((*(can_mesg+1) != 0) || (mem_addr_bytes > 4) || (mem_size_bytes > 4))
	{
		/* negative response */
		*reply_mesg = 0x7F;
		*(reply_mesg+1) = 0x34;
		*(reply_mesg+2) = 0x31;  //out of range
		*reply_mesg_len = 3;
	}

	/* get memory address */
	for(i=0; i<mem_addr_bytes; i++)
	{
		if(i > 0)
		{
			bootloader_global->mem_addr <<= 8;
		}

		bootloader_global->mem_addr |= \
				*(can_mesg+3+i);
	}

	/* get memory size */
	for(i=0; i<mem_size_bytes; i++)
	{
		if(i > 0)
		{
			bootloader_global->mem_size <<= 8;
		}

		bootloader_global->mem_size |= \
				*(can_mesg+3+mem_addr_bytes+i);
	}

	/* if bootloader initialization failed, negative reply was send to tester */
	if(bootloader_init(bootloader_global->mem_size) < 0)
	{
		/* negative response */
		*reply_mesg = 0x7F;
		*(reply_mesg+1) = 0x34;
		*(reply_mesg+2) = 0x70;  //reject download
		*reply_mesg_len = 3;
	}
	else
	{
		SHA1Reset(&file_sha);

		/* positive response */
		*reply_mesg = 0x74;
		*(reply_mesg+1) = 0x10;
		*(reply_mesg+2) = MAX_BYTES_OF_EACH_BLOCK;
		*reply_mesg_len = 3;
	}
}


void exit_download_process(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	if(!bootloader_global)
	{
		/* negative response */
		*reply_mesg = 0x7F;
		*(reply_mesg+1) = 0x37;
		*(reply_mesg+2) = 0x24; //general programming error
		*reply_mesg_len = 3;
	}
	else if(bootloader_global->bootloader_state != Download_Data)
	{
		/* negative response */
		*reply_mesg = 0x7F;
		*(reply_mesg+1) = 0x37;
		*(reply_mesg+2) = 0x24; //general programming error
		*reply_mesg_len = 3;
	}
	else
	{
		bootloader_global->bootloader_state = Request_Exit;

		if(bootloader_global->recv_data_length < bootloader_global->mem_size)
		{
			/* negative response */
			*reply_mesg = 0x7F;
			*(reply_mesg+1) = 0x37;
			*(reply_mesg+2) = 0x24; //general programming error
			*reply_mesg_len = 3;
		}
		else
		{
			/* positive response */
			*reply_mesg = 0x77;
			*reply_mesg_len = 1;
		}
	}
}


void check_programming_integrity(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	unsigned int crc32_temp = 0;

	if(!bootloader_global)
	{
		/* negative response */
		*reply_mesg = 0x71;
		*(reply_mesg+1) = 0x01;
		*(reply_mesg+2) = 0x02;
		*(reply_mesg+3) = 0x02;
		*(reply_mesg+4) = 0x01;
		*reply_mesg_len = 5;
	}
	else if(bootloader_global->bootloader_state != Request_Exit)
	{
		/* negative response */
		*reply_mesg = 0x71;
		*(reply_mesg+1) = 0x01;
		*(reply_mesg+2) = 0x02;
		*(reply_mesg+3) = 0x02;
		*(reply_mesg+4) = 0x01;
		*reply_mesg_len = 5;
	}
	else if(mesg_len < (9+8))
	{
		/* negative response */
		*reply_mesg = 0x71;
		*(reply_mesg+1) = 0x01;
		*(reply_mesg+2) = 0x02;
		*(reply_mesg+3) = 0x02;
		*(reply_mesg+4) = 0x01;
		*reply_mesg_len = 5;
	}
	else
	{
		crc32_temp = (*(can_mesg+4)<<24)|(*(can_mesg+5)<<16)|(*(can_mesg+6)<<8)|*(can_mesg+7);

		if(crc32_temp == (bootloader_global->crc32_cal^0xffffffff))
		{
			bootloader_global->bootloader_state = Check_Integrity;
			/* positive response */
			*reply_mesg = 0x71;
			*(reply_mesg+1) = 0x01;
			*(reply_mesg+2) = 0x02;
			*(reply_mesg+3) = 0x02;
			*(reply_mesg+4) = 0x00;
			*reply_mesg_len = 5;
		}
		else
		{
			/* negative response */
			*reply_mesg = 0x71;
			*(reply_mesg+1) = 0x01;
			*(reply_mesg+2) = 0x02;
			*(reply_mesg+3) = 0x02;
			*(reply_mesg+4) = 0x01;
			*reply_mesg_len = 5;
		}
	}
}



int save_data_as_file(void)
{
	unsigned short crc16_cal = 0;
	char *file_full_name, *p, *file_name;
	FILE *src_file = NULL;

	crc16_cal = crc16(0, bootloader_global->recv_data, CUSTOMIZED_HEADER_SIZE-2);

	if(crc16_cal == *(unsigned short*)(bootloader_global->recv_data+CUSTOMIZED_HEADER_SIZE-2))
	{
		file_full_name = bootloader_global->recv_data + HEADER_FILE_FULL_NAME_START;

		if(strlen(file_full_name) > HEADER_FILE_FULL_NAME_SIZE)
		{
			return -1;
		}
		else
		{
			file_name = file_full_name;

			while((p = strchr(file_name, '/')))
			{
				file_name = p + 1;
			}

			if(strlen(file_name) <= 0)
			{
				return -1;
			}
			else
			{
				p = file_name - 1;
				*p = '\0';

				if(createMultiLevelDir(file_full_name) < 0)
				{
					return -1;
				}
				else
				{
					*p = '/';

					if((src_file = fopen(file_full_name, "w+")) == NULL)
					{
						return -1;
					}
					else
					{
						fseek(src_file, 0, SEEK_SET);  //go to head of file
						fwrite(bootloader_global->recv_data+CUSTOMIZED_HEADER_SIZE, \
							   bootloader_global->recv_data_length-CUSTOMIZED_HEADER_SIZE-SHA1HashSize, \
							   1, src_file);
						fflush(src_file);
						fclose(src_file);
						sync();
						sync();
						chmod(file_full_name, 755);
					}
				}
			}
		}
	}
	else
	{
		return -1;
	}

	return 0;
}


void check_programming_dependency(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	unsigned char i = 0;
	unsigned char cal_sha1_result[SHA1HashSize];

	if(!bootloader_global)
	{
		/* negative response */
		*reply_mesg = 0x71;
		*(reply_mesg+1) = 0x01;
		*(reply_mesg+2) = 0xFF;
		*(reply_mesg+3) = 0x01;
		*(reply_mesg+4) = 0x01;
		*reply_mesg_len = 5;
	}
	else if(bootloader_global->bootloader_state != Check_Integrity)
	{
		/* negative response */
		*reply_mesg = 0x71;
		*(reply_mesg+1) = 0x01;
		*(reply_mesg+2) = 0xFF;
		*(reply_mesg+3) = 0x01;
		*(reply_mesg+4) = 0x01;
		*reply_mesg_len = 5;
	}
	else
	{
		SHA1Input(&file_sha, bootloader_global->recv_data, bootloader_global->recv_data_length-SHA1HashSize);
		SHA1Result(&file_sha, cal_sha1_result);

		for(i=0; i<SHA1HashSize; i++)
		{
			printf("%2x ", *(cal_sha1_result+i));
		}

		puts("");

		if((compare_array((bootloader_global->recv_data + bootloader_global->recv_data_length - SHA1HashSize), \
						   cal_sha1_result, SHA1HashSize)) != 0)
		{
			/* negative response */
			*reply_mesg = 0x71;
			*(reply_mesg+1) = 0x01;
			*(reply_mesg+2) = 0xFF;
			*(reply_mesg+3) = 0x01;
			*(reply_mesg+4) = 0x01;
			*reply_mesg_len = 5;
		}
		else
		{
			bootloader_global->recv_data_length -= SHA1HashSize;
			bootloader_global->bootloader_state = Check_Dependency;

			if(*bootloader_global->recv_data == FILE_IS_FOR_ARM)
			{
//				if(save_data_as_file())
//				{
//
//				}

	            /* positive response */
				*reply_mesg = 0x71;
				*(reply_mesg+1) = 0x01;
				*(reply_mesg+2) = 0xFF;
				*(reply_mesg+3) = 0x01;
				*(reply_mesg+4) = 0x00;
				*reply_mesg_len = 5;
			}
			else
			{
				/* negative response */
				*reply_mesg = 0x71;
				*(reply_mesg+1) = 0x01;
				*(reply_mesg+2) = 0xFF;
				*(reply_mesg+3) = 0x01;
				*(reply_mesg+4) = 0x01;
				*reply_mesg_len = 5;
			}
		}
	}
}


int bootloader_state_process(const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len)
{
	/* download data */
	if(0x36 == *can_mesg)
	{
		download_data_process(can_mesg, mesg_len, reply_mesg, reply_mesg_len);
	}
	else if(0x34 == *can_mesg)  //request download
	{
		request_download_process(can_mesg, mesg_len, reply_mesg, reply_mesg_len);
	}
	else if(0x37 == *can_mesg)  //exit download
	{
		exit_download_process(can_mesg, mesg_len, reply_mesg, reply_mesg_len);
	}
	/* check programming integrity */
	else if((0x31 == *can_mesg) && (0x01 == *(can_mesg+1)) && \
				(0x02 == *(can_mesg+2)) && (0x02 == *(can_mesg+3)))
	{
		check_programming_integrity(can_mesg, mesg_len, reply_mesg, reply_mesg_len);
	}
	/* check programming dependency */
	else if((0x31 == *can_mesg) && (0x01 == *(can_mesg+1)) && \
			(0xFF == *(can_mesg+2)) && (0x01 == *(can_mesg+3)))
	{
		check_programming_dependency(can_mesg, mesg_len, reply_mesg, reply_mesg_len);
	}
	else
	{

	}

	return 0;
}








