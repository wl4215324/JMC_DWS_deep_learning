/*
 * queue_for_image.c
 *
 *  Created on: Oct 26, 2019
 *      Author: tony
 */

#include "queue_for_image.h"


Video_Queue *init_video_queue()
{
	unsigned short i = 0;
	Video_Queue *p_queue = NULL;
	p_queue = (Video_Queue*)malloc(sizeof(Video_Queue));

	if(!p_queue)
	{
		goto init_error_exit;
	}

	p_queue->front_idx = 0;
	p_queue->rear_idx = 0;
	pthread_mutex_init(&p_queue->queue_lock, NULL);
	p_queue->frame_index_array = NULL;
	p_queue->frame_index_array = (Frame_Index*)malloc(sizeof(Frame_Index) * (QUEUE_ENTRY_COUNT+1));

	if(!p_queue->frame_index_array)
	{
		goto init_error_exit;
	}

	p_queue->frame_count_of_queue = QUEUE_ENTRY_COUNT + 1;

	for(i=0; i<(QUEUE_ENTRY_COUNT+1); i++)
	{
		(p_queue->frame_index_array+i)->image_frame_size = 0;
		(p_queue->frame_index_array+i)->image_frame_start = NULL;
	}

	p_queue->data_queue = init_basic_queue();

	if(!p_queue->data_queue)
	{
		goto init_error_exit;
	}

	return p_queue;

init_error_exit:
    if(p_queue)
    {
    	p_queue->front_idx = 0;
    	p_queue->rear_idx = 0;

        if(p_queue->data_queue)
        {
        	destroy_basic_queue(&(p_queue->data_queue));
        }

        pthread_mutex_destroy(&(p_queue->queue_lock));

        if(p_queue->frame_index_array)
        {
        	free(p_queue->frame_index_array);
        }

        p_queue = NULL;
    }

    return (Video_Queue*)(-1);
}


void destroy_video_queue(Video_Queue **video_queue)
{
	unsigned int i = 0;

	if(!(*video_queue))
	{
		return;
	}

	pthread_mutex_lock(&(*video_queue)->queue_lock);
	(*video_queue)->front_idx = 0;
	(*video_queue)->rear_idx = 0;

	if((*video_queue)->frame_index_array)
	{
		free((*video_queue)->frame_index_array);
	}

    if((*video_queue)->data_queue)
    {
    	destroy_basic_queue(&((*video_queue)->data_queue));
    }

    pthread_mutex_unlock(&(*video_queue)->queue_lock);
    pthread_mutex_destroy(&(*video_queue)->queue_lock);
    *video_queue = NULL;
}


void reset_video_queue(Video_Queue *video_queue)
{
	unsigned int i = 0;

	if(!video_queue)
	{
		return;
	}

	pthread_mutex_lock(&video_queue->queue_lock);
	video_queue->front_idx = 0;
	video_queue->rear_idx = 0;
	reset_basic_queue(video_queue->data_queue);

	if(video_queue->frame_index_array)
	{
		for(i=0; i<video_queue->frame_count_of_queue; i++)
		{
			(video_queue->frame_index_array+i)->image_frame_size = 0;
			(video_queue->frame_index_array+i)->image_frame_start = NULL;
		}
	}

	pthread_mutex_unlock(&video_queue->queue_lock);
}



void push_data_into_video_queue(Video_Queue *video_queue, unsigned char *push_data, \
		unsigned short push_data_len)
{
	unsigned char available_entry_count = 0;

	pthread_mutex_lock(&(video_queue->queue_lock));

	if(video_queue && video_queue->frame_index_array && \
	   video_queue->data_queue &&  push_data && push_data_len)
	{
		while(1)
		{
			/* if video data block is full, take out one frame from frame queue and data queue */
			if(push_data_len > get_left_len_of_basic_queue(video_queue->data_queue))
			{
				take_out_data_from_basic_queue(video_queue->data_queue, \
						(video_queue->frame_index_array+video_queue->front_idx)->image_frame_size);
				(video_queue->frame_index_array+video_queue->front_idx)->image_frame_start = NULL;
				(video_queue->frame_index_array+video_queue->front_idx)->image_frame_size = 0;
				video_queue->front_idx = (video_queue->front_idx+1)%video_queue->frame_count_of_queue;
			}
			else
			{
				/* if frame queue is full, take out one frame from frame queue and data queue */
				if((video_queue->rear_idx+1) % video_queue->frame_count_of_queue == \
						video_queue->front_idx)
				{
					take_out_data_from_basic_queue(video_queue->data_queue, \
							(video_queue->frame_index_array+video_queue->front_idx)->image_frame_size);
					(video_queue->frame_index_array+video_queue->front_idx)->image_frame_start = NULL;
					(video_queue->frame_index_array+video_queue->front_idx)->image_frame_size = 0;
					video_queue->front_idx = (video_queue->front_idx+1)%video_queue->frame_count_of_queue;
				}
				else  // push one image into frame index queue and data queue
				{
					(video_queue->frame_index_array+video_queue->rear_idx)->image_frame_size = push_data_len;
					(video_queue->frame_index_array+video_queue->rear_idx)->image_frame_start = \
							video_queue->data_queue->data_block + video_queue->data_queue->tail_idx;
					push_data_into_basic_queue(video_queue->data_queue, push_data, push_data_len);
					video_queue->rear_idx = (video_queue->rear_idx+1) % video_queue->frame_count_of_queue;
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&(video_queue->queue_lock));
}


void print_video_queue(Video_Queue *video_queue)
{
	unsigned char frames_num = 0;
	unsigned char i = 0;
	unsigned int j = 0;

	//printf("");

	if(video_queue)
	{
		pthread_mutex_lock(&(video_queue->queue_lock));

		if(video_queue->data_queue && video_queue->frame_index_array)
		{
			frames_num = (video_queue->rear_idx - video_queue->front_idx + \
					video_queue->frame_count_of_queue) % video_queue->frame_count_of_queue;

			for(i=0; i<frames_num; i++)
			{
				printf("frame %d data: ", i);

				for(j=0; j<(video_queue->frame_index_array+video_queue->front_idx)->image_frame_size; j++)
				{
					printf("%04X ", *((video_queue->frame_index_array+video_queue->front_idx)->image_frame_start+j));
				}

				printf("\n");
			}
		}

		pthread_mutex_unlock(&(video_queue->queue_lock));
	}
}




int pull_one_frame_from_video_queue(Video_Queue *video_queue, unsigned char *pull_data)
{
	int ret = -1;
	unsigned int min_len = 0;

	if(video_queue && pull_data)
	{
		pthread_mutex_lock(&(video_queue->queue_lock));

		/* if video frame  queue is empty, return 0 */
		if(video_queue->front_idx == video_queue->rear_idx)
		{
			ret = 0;
		}
		else if(is_basic_queue_empty(video_queue->data_queue))  //if video block queue is empty
		{
			ret = 0;
		}
		else
		{
			if((video_queue->frame_index_array+video_queue->front_idx)->image_frame_size && \
			   (video_queue->frame_index_array+video_queue->front_idx)->image_frame_start )
			{
				if((video_queue->frame_index_array+video_queue->front_idx)->image_frame_size > \
						get_data_len_of_basic_queue(video_queue->data_queue))
				{
					min_len = get_data_len_of_basic_queue(video_queue->data_queue);
				}
				else
				{
					min_len = (video_queue->frame_index_array+video_queue->front_idx)->image_frame_size;
				}

				ret = pull_data_from_basic_queue(video_queue->data_queue, pull_data, min_len);
				(video_queue->frame_index_array+video_queue->front_idx)->image_frame_size = 0;
				(video_queue->frame_index_array+video_queue->front_idx)->image_frame_start = NULL;
				video_queue->front_idx = (video_queue->front_idx+1) % video_queue->frame_count_of_queue;
			}
		}

		pthread_mutex_unlock(&(video_queue->queue_lock));
		return ret;
	}
	else
	{
		return -1;
	}
}


int save_video_frames_as_file(Video_Queue *video_queue, FILE *fd_video)
{
	unsigned int ret_len = 0;
	unsigned char pull_buf[1024];

	if(video_queue && fd_video)
	{
		pthread_mutex_lock(&(video_queue->queue_lock));

		while(is_basic_queue_empty(video_queue->data_queue) == 0)
		{
			ret_len = pull_data_from_basic_queue(video_queue->data_queue, pull_buf, sizeof(pull_buf));
			fwrite(pull_buf, ret_len, 1, fd_video);
		}

        fsync(fileno(fd_video));
		fclose(fd_video);
		pthread_mutex_unlock(&(video_queue->queue_lock));

		reset_video_queue(video_queue);
		return 0;
	}
	else
	{
		return -1;
	}
}



#if 0
unsigned char is_queue_empty(Video_Queue *video_queue)
{
	unsigned char ret = 0;

	if(!video_queue)
	{
		return ret;
	}

	pthread_mutex_lock(&(video_queue->queue_lock));

	if((video_queue->front_idx == video_queue->tail_idx) && \
	   (video_queue->queue_array+video_queue->front_idx) && \
	   ((video_queue->queue_array+video_queue->front_idx)->data_len == 0))
	{
		ret = 1;
	}

	pthread_mutex_unlock(&(video_queue->queue_lock));
	return ret;
}


unsigned char is_queue_full(Video_Queue *video_queue)
{
	unsigned char ret = 0;

	if(!video_queue)
	{
		return ret;
	}

	pthread_mutex_lock(&(video_queue->queue_lock));

	if((video_queue->front_idx == (video_queue->tail_idx+1)%QUEUE_ENTRY_COUNT) && \
	   (video_queue->queue_array+video_queue->tail_idx) && \
	   ((video_queue->queue_array+video_queue->tail_idx)->data_len&0xFFFF > 0))
	{
		ret = 1;
	}

	pthread_mutex_unlock(&(video_queue->queue_lock));

	return ret;
}
#endif


