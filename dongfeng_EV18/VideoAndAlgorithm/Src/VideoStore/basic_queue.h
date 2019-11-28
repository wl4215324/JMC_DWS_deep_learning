/*
 * basic_queue.h
 *
 *  Created on: Oct 27, 2019
 *      Author: tony
 */

#ifndef BASIC_QUEUE_H_
#define BASIC_QUEUE_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define  KB_SIZE  1024
#define  MB_SIZE  KB_SIZE*KB_SIZE
#define  DATA_BUF_LEN_OF_ENTRY  20*KB_SIZE
#define  VIDEO_DATA_BLOCK_SIZE  6*MB_SIZE


typedef struct {
	unsigned int head_idx;
	unsigned int tail_idx;
	unsigned char *data_block;
	unsigned int data_block_size;
} Basic_Queue;


Basic_Queue* init_basic_queue();

void destroy_basic_queue(Basic_Queue **basic_queue);

unsigned char is_basic_queue_empty(Basic_Queue *basic_queue);

unsigned char is_basic_queue_full(Basic_Queue *basic_queue);

unsigned int get_data_len_of_basic_queue(Basic_Queue *basic_queue);

unsigned int get_left_len_of_basic_queue(Basic_Queue *basic_queue);

unsigned int push_data_into_basic_queue(Basic_Queue *basic_queue, unsigned char *push_data, \
		unsigned int push_data_len);

unsigned int pull_data_from_basic_queue(Basic_Queue *basic_queue, unsigned char *pull_data, \
		unsigned int pull_data_len);

unsigned int take_out_data_from_basic_queue(Basic_Queue *basic_queue, unsigned int take_out_data_len);

void reset_basic_queue(Basic_Queue *basic_queue);

#endif /* BASIC_QUEUE_H_ */
