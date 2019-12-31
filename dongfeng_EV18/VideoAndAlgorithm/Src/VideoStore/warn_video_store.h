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

//#define  SAVE_WARN_VIDEO_FILE

typedef struct {
	T7_Video_Encode *t7_video_encode;
	Video_Queue *video_file_queue;
	user_timer   *file_store_timer;
	file_status_t *file_status;
	char sd_card_status;
	char video_file_name[128];
	pthread_mutex_t file_lock;
	pthread_cond_t file_cond;

} Video_File_Resource;


Video_File_Resource *init_video_store(uint32_t src_width, uint32_t src_height, uint32_t dst_width, uint32_t dst_height, \
		uint8_t bit_rate, uint8_t frame_rate, VENC_CODEC_TYPE encoder_type);

void destroy_video_store(Video_File_Resource **video_file_resource);

void *save_warn_video(void *argv);

int notify_save_file(unsigned long data);

#endif /* WARN_VIDEO_STORE_H_ */
