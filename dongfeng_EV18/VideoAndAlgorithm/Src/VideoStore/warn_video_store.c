/*
 * app_main.c
 *
 *  Created on: Nov 15, 2019
 *      Author: tony
 */

#include "warn_video_store.h"
#include "files_manager.h"
#include "file_operation.h"
#include "bug.h"

Video_File_Resource *dsm_video_record = NULL;
Video_File_Resource *monitor_video_record = NULL;


int notify_save_file(unsigned long data)
{
	DEBUG_INFO(enter \n);
	Video_File_Resource *video_file_resource = (Video_File_Resource *)data;

	if(video_file_resource)
	{
		pthread_mutex_lock(&(video_file_resource->file_lock));

		if(video_file_resource->sd_card_status == 0)
		{
			video_file_resource->sd_card_status = 1;
		}

		pthread_cond_signal(&(video_file_resource->file_cond));
		pthread_mutex_unlock(&(video_file_resource->file_lock));
	}

	DEBUG_INFO(exit\n);
	return 0;
}



void *save_warn_video(void *argv)
{
	Video_File_Resource *video_file_resource = (Video_File_Resource *)argv;

	if(!video_file_resource)
	{
		DEBUG_INFO(arguments invalid please check!\n);
		return NULL;
	}

	FILE *fp = NULL;

	while(1)
	{
		pthread_mutex_lock(&(video_file_resource->file_lock));

		if(video_file_resource->sd_card_status != 1)
		{
			pthread_cond_wait(&(video_file_resource->file_cond), &(video_file_resource->file_lock));
		}

		if(video_file_resource->file_status->file_dir_status == FILE_DIR_NORMAL)
		{
			fp = fopen(video_file_resource->video_file_name, "w");
			fwrite(video_file_resource->t7_video_encode->sps_pps_data.pBuffer, \
					video_file_resource->t7_video_encode->sps_pps_data.nLength, 1, fp);
			save_video_frames_as_file(video_file_resource->video_file_queue, fp);
			video_file_resource->sd_card_status = 0;
		}

		pthread_mutex_unlock(&(video_file_resource->file_lock));
	}

	return NULL;
}



Video_File_Resource *init_video_store(uint32_t src_width, uint32_t src_height, uint32_t dst_width, uint32_t dst_height, \
		uint8_t bit_rate, uint8_t frame_rate, VENC_CODEC_TYPE encoder_type)
{
	Video_File_Resource *video_file_resource = NULL;

	if(src_width <= 0 || src_height <= 0 || dst_width <= 0 || bit_rate <= 0 || frame_rate <= 0)
	{
		goto init_error_exit;
	}

	if(bit_rate > MAX_BIT_RATE_MBIT)
	{
		bit_rate = MAX_BIT_RATE_MBIT;
	}

	if(frame_rate > MAX_FRAME_RATE)
	{
		frame_rate = MAX_FRAME_RATE;
	}

	video_file_resource = (Video_File_Resource*)malloc(sizeof(Video_File_Resource));

	if(!video_file_resource)
	{
		goto init_error_exit;
	}

	video_file_resource->sd_card_status = -1;
	memset(video_file_resource->video_file_name, 0, sizeof(video_file_resource->video_file_name));
	pthread_mutex_init(&(video_file_resource->file_lock), NULL);
	pthread_cond_init(&(video_file_resource->file_cond), NULL);

	video_file_resource->t7_video_encode = init_video_encoder(src_width, src_height, dst_width, dst_height, \
			bit_rate, frame_rate, encoder_type);

	if(video_file_resource->t7_video_encode == (T7_Video_Encode*)(-1))
	{
		goto init_error_exit;
	}

	video_file_resource->video_file_queue = init_video_queue();

	if(video_file_resource->video_file_queue == (Video_Queue *)(-1))
	{
		goto init_error_exit;
	}

	video_file_resource->file_store_timer = (user_timer*)malloc(sizeof(user_timer));
	if(!video_file_resource->file_store_timer)
	{
		goto init_error_exit;
	}

	init_user_timer(video_file_resource->file_store_timer, GET_TICKS_TEST, notify_save_file, (unsigned long)video_file_resource);

	video_file_resource->file_status = initFileListDir("/mnt/sdcard/mmcblk1p1/", 0, bit_rate, 10);

	if(video_file_resource->file_status == (file_status_t*)(-1))
	{
		video_file_resource->sd_card_status = -1;
	}
	else
	{
		video_file_resource->sd_card_status = 0;
	}

	return video_file_resource;

init_error_exit:
    if(video_file_resource)
    {
    	if(video_file_resource->t7_video_encode)
    	{
    		destroy_video_encoder(&(video_file_resource->t7_video_encode));
    	}

    	if(video_file_resource->video_file_queue)
    	{
    		destroy_video_queue(&(video_file_resource->video_file_queue));
    	}

    	if(video_file_resource->file_store_timer)
    	{
    		free(video_file_resource->file_store_timer);
    	}

    	free(video_file_resource);
    }
    return (Video_File_Resource*)(-1);
}



void destroy_video_store(Video_File_Resource **video_file_resource)
{
	if(video_file_resource && *video_file_resource)
	{
		pthread_mutex_lock(&((*video_file_resource)->file_lock));
		destroy_video_encoder(&((*video_file_resource)->t7_video_encode));
		destroy_video_queue(&((*video_file_resource)->video_file_queue));
		free((*video_file_resource)->file_store_timer);
		pthread_mutex_unlock(&((*video_file_resource)->file_lock));

		pthread_mutex_destroy(&((*video_file_resource)->file_lock));
		pthread_cond_destroy(&((*video_file_resource)->file_cond));
		free(*video_file_resource);
		*video_file_resource = NULL;
	}
}







