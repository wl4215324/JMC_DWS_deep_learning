/*
 * bootloader_new.h
 *
 *  Created on: Dec 4, 2018
 *      Author: tony
 */

#ifndef BOOTLOADER_NEW_H_
#define BOOTLOADER_NEW_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "crc32.h"
#include "sha_1.h"
#include "crc16.h"
#include "file_operate.h"

#define  CUSTOMIZED_HEADER_START  0
#define  CUSTOMIZED_HEADER_SIZE   96

#define  HEADER_CHIP_TYPE_SIZE    1

#define  HEADER_FILE_TYPE_START   (CUSTOMIZED_HEADER_START + HEADER_CHIP_TYPE_SIZE)
#define  HEADER_FILE_TYPE_SIZE    2

#define  HEADER_FILE_BYTES_START  (HEADER_FILE_TYPE_START + HEADER_FILE_TYPE_SIZE)
#define  HEADER_FILE_BYTES_SIZE   4

#define  HEADER_FILE_FULL_NAME_START  (HEADER_FILE_BYTES_START + HEADER_FILE_BYTES_SIZE)
#define  HEADER_FILE_FULL_NAME_SIZE   87

#define  HEADER_CRC16_START  (HEADER_FILE_FULL_NAME_START+HEADER_FILE_FULL_NAME_SIZE)
#define  HEADER_CRC16_SIZE   2

#define  ONE_M_SIZE  1024*1024
#define  MAX_FILE_SIZE  10*ONE_M_SIZE

#define  MAX_BYTES_OF_EACH_BLOCK  100

#define  FILE_IS_FOR_ARM  1


typedef enum
{
	Request_Download = 0,
	Download_Data,
	Request_Exit,
	Check_Integrity,
	Check_Dependency,
} bootloader_process;

typedef struct
{
	bootloader_process bootloader_state;
	unsigned char* recv_data;
	int recv_data_length;
	unsigned char block_index;
	unsigned char prev_block_index;
	unsigned int mem_addr;
	unsigned int mem_size;
	unsigned int crc32_cal;
} bootloader_core_data;

//extern bootloader_core_data *bootloader_global;


#endif /* BOOTLOADER_NEW_H_ */
