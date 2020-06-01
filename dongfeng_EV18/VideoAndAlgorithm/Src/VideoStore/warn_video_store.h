/*
 * warn_video_store.h
 *
 *  Created on: Nov 21, 2019
 *      Author: tony
 */

#ifndef WARN_VIDEO_STORE_H_
#define WARN_VIDEO_STORE_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "video_encode.h"
#include "queue_for_image.h"
#include "timer.h"
#include "user_timer.h"
#include "bug.h"
#include "files_manager.h"

#define  MAX_BIT_RATE_MBIT  8
#define  MAX_FRAME_RATE  30

#define  SD_MOUNT_DIRECTORY  "/mnt/sdcard/mmcblk1p1/"

#define  SAVE_WARN_VIDEO_FILE

typedef enum {
	SD_UNMOUNT = -1,
	SD_MOUNT,
    SD_FULL,
    SD_NORMAL,
} SD_Card_Status;

typedef struct {
	T7_Video_Encode *t7_video_encode;  //video encoder for 10s video
	T7_Video_Encode *t7_jpeg_encode;  //video encoder for warnning jpg image
	unsigned char *alert_proof_image; //yuv420p data buffer for warning image
	unsigned int alert_proof_size;  //yuv420p data length for one frame
	Video_Queue *video_file_queue;  //circly queue for 10s video
	user_timer   *file_store_timer; //5s timer will be launched when warnning is triggered
	file_status_t *file_status;  //file manager for
	char sd_card_status;  //sd card is mounted or not
	char video_file_name[128]; //full pathname for one new warnning
	char warn_sys_time[32]; //date time for warnning come out
	char warn_position[32]; //longitude and latitude for warnning come out
	char warn_type[16]; //for record warn type
	pthread_mutex_t file_lock;
	pthread_cond_t file_cond;
} Video_File_Resource;


Video_File_Resource *init_video_store(uint32_t src_width, uint32_t src_height, \
		uint32_t dst_width, uint32_t dst_height, uint8_t bit_rate, uint8_t frame_rate, \
		uint8_t camera_idx, VENC_CODEC_TYPE encoder_type);

void destroy_video_store(Video_File_Resource **video_file_resource);

void *save_warn_video(void *argv);

int notify_save_file(unsigned long data);

#endif /* WARN_VIDEO_STORE_H_ */
