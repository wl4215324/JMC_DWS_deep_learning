/*
 * bootloader.h
 *
 *  Created on: Jan 23, 2018
 *      Author: tony
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include "list.h"
#include "crc32.h"


#define MINORBITS   16
#define MINORMASK   ((1U << MINORBITS) - 1)
#define MAJOR(dev)  ((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)  ((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi) (((ma) << MINORBITS) | (mi))


#define  DIAG_SESS_CTRL    0x10
#define  ECU_RESET         0x11
#define  CLEAR_DIAG_INFO   0x14
#define  READ_DTC_INFO     0x19
#define  READ_DAT_BY_ID    0x22
#define  READ_MEM_BY_ADDR  0x23
#define  SECURE_ACCESS     0x27
#define  COMMUN_CTRL       0x28
#define  READ_DAT_BY_PID   0x2A
#define  DYNA_DEFI_DAT_ID  0x2C
#define  WRITE_DAT_BY_ID   0x2E
#define  INPUT_OUTPUT_CTRL_BY_ID  0x2F
#define  ROUTINE_CTRL      0x31
#define  REQUEST_DOWNLOAD  0x34
#define  TRANSFER_DATA     0x36
#define  REQ_TRANSFER_DAT_EXIT  0x37
#define  WRITE_MEM_BY_ID   0x3D
#define  CONTROL_DTC_SETTING  0x85
#define  MAX_NUMBER_OF_BLOCK_LENG  0x100

#define  APPLICATION_NAME  "/home/user/jmc_dws"
#define  APPLICATION_NAME_BAKUP "/home/user/jmc_dws_bakup"

#define  DRIVER_DOWNLOAD_ADDR   0x01
#define  DRIVER_LENTH_OF_BYTES  0x01

#define  APP_DOWNLOAD_ADDR    0x0A0000
#define  APP_LENGTH_OF_BYTES  70000

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

#define  BT_RESULT_SAVE_FILE  "/home/user/BT_mode"



struct BootloaderData{
	unsigned int state;
	unsigned char *precv_frame;
};


typedef enum {
	pre_programming = 0,
	being_programming,
	after_programming
} BootloaderState;


typedef enum {
	DiagnosticSessionControl = 0,
	RoutineControl,
	ControlDTCSetting,
	CommunicationControl,
	ReadDataByIdentifier
} PreProgramSubstate;


typedef enum {
	ExtendedSession = 1,
	CheckPreprogrammingCondition,
	SetDTCoff,
	ForbidCommunication,
    ReadDataByDID,
    ProgrammingSession,
    SafeAccessForSeed,
    SafeAccessForKey,
    DownloadDriver,
    WriteFinger,
    EraseMemory,
    DownloadApplication,
    CheckIntegrity,
    CheckDependency,
    ResetECU,
    ExtendedSessionAgain,
    ResumeCommunication,
    ResumeDTC,
    DefaultSession,
    ClearBootloaderInfor,
    FinshBootloader
} JMCBootloaderSubsequence;


typedef struct {
	unsigned int level_one;
	unsigned int level_FBL;
} Seed;

typedef struct {
	unsigned int level_one;
	unsigned int level_FBL;
} SecretKey;


/* data segment */
typedef struct {
	struct list_head segment_list;
	unsigned char block_index;
	unsigned char prev_block_index;
	unsigned int mem_addr;
	unsigned int mem_size;
    unsigned int segment_len;
    unsigned char *data;
} DataSegment;


/* finger print of one logic block */
typedef struct{
	unsigned char *block_index;
	unsigned char YY;
	unsigned char MM;
	unsigned char DD;
	char serial_num[6];
} FingerPrint;



/* logic block creation process-state */
typedef enum {
	WriteFingerPrint = 1,
	ErasingMemory,
	RequestDownload,
	DownloadData,
    FinishDownload,
    CheckingIntegrity
}LogicBlockState;



/* logic block */
typedef struct {
	unsigned char file_type;  //0:driver file, 1: application file
	LogicBlockState block_state;
	unsigned char block_index;
	unsigned char prev_block_index;
	unsigned char block_download_result;
	unsigned int mem_addr;
	unsigned int mem_size;
	unsigned int crc32_cal;
	FingerPrint finger_print;
	unsigned short MaxNumOfBlockLeng;
	struct list_head data_segment_head;
} LogicBlock;


typedef struct {
	struct list_head logic_block_list;
	LogicBlock logic_block_data;
} LogicBlockNode;


/* bootloader business logic data struct */
typedef struct {
	JMCBootloaderSubsequence bootloader_subseq;
	Seed seed;
	SecretKey secret_key;
	struct list_head driver_list_head;
	struct list_head app_list_head;
} BootloaderBusinessLogic;

extern BootloaderBusinessLogic  JMC_bootloader_logic;

int bootloader_logic_init(BootloaderBusinessLogic *bootloader_logic);

int bootloader_main_process(BootloaderBusinessLogic *bootloader_logic, \
		const unsigned char* can_mesg, unsigned short mesg_len, \
		unsigned char* reply_mesg, unsigned short* reply_mesg_len);

#endif /* BOOTLOADER_H_ */
