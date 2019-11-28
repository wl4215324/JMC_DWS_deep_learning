/*
 * basic_queue.c
 *
 *  Created on: Oct 27, 2019
 *      Author: tony
 */

#include "basic_queue.h"


Basic_Queue* init_basic_queue()
{
	Basic_Queue *new_queue = NULL;
	new_queue = (Basic_Queue *) malloc(sizeof(Basic_Queue));

	if(!new_queue)
	{
		goto init_error_exit;
	}

	new_queue->head_idx = 0;
	new_queue->tail_idx = 0;
	new_queue->data_block_size = VIDEO_DATA_BLOCK_SIZE + 1;
	new_queue->data_block = NULL;
	new_queue->data_block = (unsigned char*)malloc((new_queue->data_block_size) * sizeof(unsigned char));

	if(!new_queue->data_block)
	{
		goto init_error_exit;
	}

	memset(new_queue->data_block, 0, new_queue->data_block_size);
	return new_queue;

init_error_exit:
    if(new_queue)
    {
    	if(new_queue->data_block)
    	{
    		free(new_queue->data_block);
    	}

    	free(new_queue);
    }

    return (Basic_Queue *)(-1);
}



void destroy_basic_queue(Basic_Queue **basic_queue)
{
	if(*basic_queue)
	{
		(*basic_queue)->head_idx = 0;
		(*basic_queue)->tail_idx = 0;

		if((*basic_queue)->data_block)
		{
			free((*basic_queue)->data_block);
			(*basic_queue)->data_block = NULL;
		}

		*basic_queue = NULL;
	}
}


unsigned char is_basic_queue_empty(Basic_Queue *basic_queue)
{
	unsigned char ret = 0;

	if(basic_queue && basic_queue->data_block)
	{
		ret = (basic_queue->head_idx == basic_queue->tail_idx) ? 1:0;
	}

	return ret;
}


unsigned char is_basic_queue_full(Basic_Queue *basic_queue)
{
	unsigned char ret = 0;

	if(basic_queue && basic_queue->data_block)
	{
		if((basic_queue->tail_idx+1)%basic_queue->data_block_size == basic_queue->head_idx)
		{
			ret = 1;
		}
	}

	return ret;
}


unsigned int get_data_len_of_basic_queue(Basic_Queue *basic_queue)
{
	unsigned int ret = 0;

	if(basic_queue && basic_queue->data_block && basic_queue->data_block_size)
	{
		ret = (basic_queue->tail_idx - basic_queue->head_idx + \
			   basic_queue->data_block_size) % basic_queue->data_block_size;
	}

	return ret;
}


unsigned int get_left_len_of_basic_queue(Basic_Queue *basic_queue)
{
	unsigned int ret = 0;

	if(basic_queue && basic_queue->data_block && basic_queue->data_block_size)
	{
		ret = (basic_queue->data_block_size + basic_queue->head_idx - \
				basic_queue->tail_idx - 1) % basic_queue->data_block_size;
	}

	return ret;
}


unsigned int push_data_into_basic_queue(Basic_Queue *basic_queue, unsigned char *push_data, \
		unsigned int push_data_len)
{
	unsigned int i = 0;
    unsigned int real_push_len = 0;

	if(basic_queue && basic_queue->data_block && basic_queue->data_block_size && \
	   push_data && push_data_len)
	{
		real_push_len = get_left_len_of_basic_queue(basic_queue);

		if(real_push_len > push_data_len)
		{
			real_push_len = push_data_len;
		}

		while(real_push_len)
		{
			*(basic_queue->data_block + basic_queue->tail_idx) = *(push_data + i);
			basic_queue->tail_idx = (basic_queue->tail_idx + 1) % basic_queue->data_block_size;
			real_push_len--;
			i++;
		}

		return i;
	}

	return 0;
}


unsigned int pull_data_from_basic_queue(Basic_Queue *basic_queue, unsigned char *pull_data, \
		unsigned int pull_data_len)
{
	unsigned int i = 0;
	unsigned int real_pull_len = 0;

	if(basic_queue && basic_queue->data_block && basic_queue->data_block_size && \
			pull_data && pull_data_len)
	{
		real_pull_len = get_data_len_of_basic_queue(basic_queue);

		if(real_pull_len > pull_data_len)
		{
			real_pull_len = pull_data_len;
		}

		while(real_pull_len)
		{
			*(pull_data + i) = *(basic_queue->data_block + basic_queue->head_idx);
			basic_queue->head_idx = (basic_queue->head_idx + 1) % basic_queue->data_block_size;
			i++;
			real_pull_len--;
		}

		return i;
	}

	return 0;
}


unsigned int take_out_data_from_basic_queue(Basic_Queue *basic_queue, unsigned int take_out_data_len)
{
	unsigned int available_data_len = 0;

	if(basic_queue && basic_queue->data_block && basic_queue->data_block_size && take_out_data_len)
	{
		available_data_len = get_data_len_of_basic_queue(basic_queue);

		if(available_data_len >= take_out_data_len)
		{
			memset(basic_queue->data_block+basic_queue->head_idx, 0, \
					take_out_data_len);
			basic_queue->head_idx = (basic_queue->head_idx + take_out_data_len) % \
					                 basic_queue->data_block_size;
			return take_out_data_len;
		}
		else
		{
			basic_queue->head_idx = basic_queue->tail_idx = 0;
			memset(basic_queue->data_block+basic_queue->head_idx, 0, \
					basic_queue->data_block_size);
			return available_data_len;
		}
	}

	return 0;
}



void reset_basic_queue(Basic_Queue *basic_queue)
{
	if(basic_queue)
	{
		basic_queue->head_idx = 0;
		basic_queue->tail_idx = 0;

		if(basic_queue->data_block)
		{
			memset(basic_queue->data_block, 0, basic_queue->data_block_size);
		}
	}
}
