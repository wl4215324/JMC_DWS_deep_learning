/*
 * files_manager.h
 *
 *  Created on: Nov 13, 2019
 *      Author: tony
 */

#ifndef FILES_MANAGER_H_
#define FILES_MANAGER_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#define CM_SAVE_FILE_MARK   "save"
#define CM_LOCK_FILE_MARK   "lock"
#define CM_MAX_RECORDFILE_NUM 400
#define CM_MAX_FILE_LEN 64
#define CM_THUMB_DIR ".thumb/"
#define CAMERA_MSG_DVR_STORE_ERR  0x40000
#define CAMERA_MSG_DVR_NEW_FILE   0x80000

#define MAX_SUPPORT_CAM 10 //2  // mod lss

#define SECTION_CAMERA_PREFIX "camera"

#define SECTION_CAMERA0 "camera0"
#define SECTION_CAMERA1 "camera1"
#define SECTION_CAMERA360 "camera360"

#define CAMERA_NAME_LEN 16

#define CurRecordFileNameType_NORMAL 1
#define CurRecordFileNameType_LOCK 2
#define CurRecordFileNameType_SAVE 3

#define RecordStat_NORMAL 1
#define RecordStat_STOPCAR 2

#define LockOps_CreateNewNow 1
#define LockOps_CreateNewAfterClose 2

#define MISCWORK_CMD_RM 0XFF01
#define MISCWORK_CMD_SAVETHUMB 0XFF02
#define RECORD_CMD_TIMEOUT 0XFF03
#define RECORD_CMD_VIDEO 0XFF04
#define RECORD_CMD_AUDIO 0XFF05


#define ENC_RINGBUF_LEVEL 16
#define ENC_RINGBUF_DATALEN (300<<10)


#define PIC_NAME_LEN 128
#define MAX_PIC_NUM 512
#define LOCK_VIDEO    "lockVideo"
#define PARK_MONITOR  "parkMonitor"
#define CAMERA_360_ID 9

#define CM_MAX_PICTURE_NUM  1024


typedef enum {
	SDCARD_NOT_MOUNT = -1,
	FILE_DIR_NOT_CREATE = 0,
	FILE_DIR_NORMAL = 1,
} File_Dir_Status;


typedef struct file_status
{
    unsigned int cur_file_idx;    //current file index
    unsigned int cur_max_filenum;  //max file count
    unsigned int cur_dir_file_num; // availavle files count
    unsigned int file_size_MB;
    char cur_filesname[CM_MAX_RECORDFILE_NUM][CM_MAX_FILE_LEN];
    char cur_filedir [CM_MAX_FILE_LEN];
    File_Dir_Status file_dir_status;
} file_status_t;


file_status_t *initFileListDir(char *dirname, unsigned char camera_index, \
		unsigned char bit_rate_Mb, unsigned short duration_s);

int genfilename(char *name, file_status_t *file_manager);

#endif /* FILES_MANAGER_H_ */
