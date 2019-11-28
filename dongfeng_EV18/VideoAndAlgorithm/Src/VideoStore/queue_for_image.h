/*
 * queue_for_image.h
 *
 *  Created on: Oct 26, 2019
 *      Author: tony
 */

#ifndef QUEUE_FOR_IMAGE_H_
#define QUEUE_FOR_IMAGE_H_


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stddef.h>

#include "basic_queue.h"

#define  QUEUE_ENTRY_COUNT  300  // for 10s video, 10s x 25f/s

#define PUSH_REGARDLESS_OF_OVERFLOW

typedef struct {
	unsigned char *image_frame_start;
	unsigned int image_frame_size;
} Frame_Index;


typedef struct {
	pthread_mutex_t queue_lock;
	unsigned short front_idx;
	unsigned short rear_idx;
	unsigned short frame_count_of_queue;
	Frame_Index *frame_index_array;
	Basic_Queue *data_queue;
} Video_Queue;


Video_Queue *init_video_queue();

void destroy_video_queue(Video_Queue **video_queue);

void reset_video_queue(Video_Queue *video_queue);

void push_data_into_video_queue(Video_Queue *video_queue, unsigned char *push_data, \
		unsigned short push_data_len);

int pull_one_frame_from_video_queue(Video_Queue *video_queue, unsigned char *pull_data);

int save_video_frames_as_file(Video_Queue *video_queue, FILE *fd_video);
#endif /* QUEUE_FOR_IMAGE_H_ */



