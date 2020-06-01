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
#include "../CurlPost/curl_post.h"

Video_File_Resource *dsm_video_record = NULL;
Video_File_Resource *monitor_video_record = NULL;

extern UrlParams * url_params;


int notify_save_file(unsigned long data)
{
	Video_File_Resource *video_file_resource = (Video_File_Resource *)data;

	if(video_file_resource && video_file_resource != (Video_File_Resource*)(-1) && \
	   video_file_resource->sd_card_status == SD_MOUNT && \
	   video_file_resource->file_status != (file_status_t *)(-1))
	{
		pthread_mutex_lock(&(video_file_resource->file_lock));

		if(video_file_resource->file_status->file_dir_status == FILE_DIR_NORMAL)
		{
			video_file_resource->file_status->file_dir_status = FILE_TO_BE_SAVED;
		}

		pthread_cond_signal(&(video_file_resource->file_cond));
		pthread_mutex_unlock(&(video_file_resource->file_lock));
	}

	return 0;
}



void *save_warn_video(void *argv)
{
	Video_File_Resource *video_file_resource = (Video_File_Resource *)argv;
	char *file_name = NULL;

	if(!video_file_resource || video_file_resource == (Video_File_Resource*)(-1))
	{
		DEBUG_INFO(arguments invalid please check!\n);
		return NULL;
	}

	FILE *fp = NULL;
	char *token = NULL;

	while(1)
	{
		pthread_mutex_lock(&(video_file_resource->file_lock));
        //wait until be notified to save file
		if(video_file_resource->sd_card_status != SD_MOUNT || \
		   video_file_resource->file_status == (file_status_t*)(-1) || \
		   ( video_file_resource->file_status != (file_status_t*)(-1) && \
		   (video_file_resource->file_status->file_dir_status != FILE_TO_BE_SAVED) ))
		{
			pthread_cond_wait(&(video_file_resource->file_cond), &(video_file_resource->file_lock));
		}

		DEBUG_INFO(video_file_resource->sd_card_status:%d \n, video_file_resource->sd_card_status);

		if(video_file_resource->sd_card_status == SD_MOUNT) //if
		{
			video_file_resource->file_status->file_dir_status = FILE_TO_BE_SAVED;
			//first create dir accroding to file name
			if(mkdir(video_file_resource->video_file_name, 0755) == 0) //create dir successfully
			{
				file_name = video_file_resource->video_file_name + \
						    strlen(video_file_resource->video_file_name);
				strcpy(file_name, "/warn.h264");
				//save h264 video
				fp = fopen(video_file_resource->video_file_name, "w");
				fwrite(video_file_resource->t7_video_encode->sps_pps_data.pBuffer, \
						video_file_resource->t7_video_encode->sps_pps_data.nLength, 1, fp);
				save_video_frames_as_file(video_file_resource->video_file_queue, fp);

				memset(file_name, '\0', strlen(file_name));
				strcpy(file_name, "/warn.jpg");
				//save jpg
				fp = fopen(video_file_resource->video_file_name, "w");
				encode_video_frame_according_to_vir_addr(video_file_resource->t7_jpeg_encode, \
						video_file_resource->alert_proof_image, video_file_resource->alert_proof_image+1280*720);
				fwrite(video_file_resource->t7_jpeg_encode->outputBuffer.pData0, \
						video_file_resource->t7_jpeg_encode->outputBuffer.nSize0, 1, fp);
				fsync(fileno(fp));
				fclose(fp);



				upload_warn_proof(url_params->usr_name, url_params->password, url_params->vin_code, \
						video_file_resource->warn_type, video_file_resource->warn_sys_time, \
						video_file_resource->warn_position, "test", video_file_resource->video_file_name);
//				get_token("admin", "rootroot", &token);
//				printf("token: %s\n", token);
			}
			else //if create dir error
			{
				DEBUG_INFO();
				//clear video storage queue
				reset_video_queue(video_file_resource->video_file_queue);
				//clear image storage buffer
				memset(video_file_resource->alert_proof_image, 0, video_file_resource->alert_proof_size);
			}
		}
		else
		{
			DEBUG_INFO();
			//clear video storage queue
			reset_video_queue(video_file_resource->video_file_queue);
			//clear image storage buffer
			memset(video_file_resource->alert_proof_image, 0, video_file_resource->alert_proof_size);
		}

		video_file_resource->file_status->file_dir_status = FILE_DIR_NORMAL;
		memset(video_file_resource->warn_type, '\0', sizeof(video_file_resource->warn_type));
		pthread_mutex_unlock(&(video_file_resource->file_lock));
	}

	return NULL;
}


/*
 * create and initialize Video_File_Resource object for video encoding and storage
 */
Video_File_Resource *init_video_store(uint32_t src_width, uint32_t src_height, \
		uint32_t dst_width, uint32_t dst_height, uint8_t bit_rate, uint8_t frame_rate, \
		uint8_t camera_idx, VENC_CODEC_TYPE encoder_type)
{
	Video_File_Resource *video_file_resource = NULL;

	/* judge the validity of parameters */
	if(src_width <= 0 || src_height <= 0 || dst_width <= 0 || bit_rate <= 0 || frame_rate <= 0)
	{
		goto init_error_exit;
	}

	//if encoder type is valid
	if(encoder_type < VENC_CODEC_H264 || encoder_type > VENC_CODEC_VP8)
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

	/* create object Video_File_Resource */
	video_file_resource = (Video_File_Resource*)malloc(sizeof(Video_File_Resource));

	if(!video_file_resource)
	{
		goto init_error_exit;
	}

	/* initialize object */
	video_file_resource->sd_card_status = SD_UNMOUNT;
	memset(video_file_resource->video_file_name, 0, sizeof(video_file_resource->video_file_name));
	memset(video_file_resource->warn_position, 0, sizeof(video_file_resource->warn_position));
	memset(video_file_resource->warn_sys_time, 0, sizeof(video_file_resource->warn_sys_time));
	memset(video_file_resource->warn_type, 0, sizeof(video_file_resource->warn_type));
	pthread_mutex_init(&(video_file_resource->file_lock), NULL);
	pthread_cond_init(&(video_file_resource->file_cond), NULL);

	//create and initialize video encoder
	video_file_resource->t7_video_encode = init_video_encoder(src_width, src_height, dst_width, dst_height, \
			bit_rate, frame_rate, encoder_type);

	if(video_file_resource->t7_video_encode == (T7_Video_Encode*)(-1))
	{
		goto init_error_exit;
	}

	//create and initialize jpeg encoder
	video_file_resource->t7_jpeg_encode = init_video_encoder(src_width, src_height, dst_width, dst_height, \
			bit_rate, frame_rate, VENC_CODEC_JPEG);

	if(video_file_resource->t7_jpeg_encode == (T7_Video_Encode*)(-1))
	{
		goto init_error_exit;
	}

	//apply space for warnning image
	video_file_resource->alert_proof_size = 0;
	video_file_resource->alert_proof_image = (unsigned char*)malloc(src_width*src_height*1.5);  //for yuv420p image
	if(video_file_resource->alert_proof_image == NULL)
	{
		goto init_error_exit;
	}

	/* initialize frame queue for encoded video */
	video_file_resource->video_file_queue = init_video_queue();

	if(video_file_resource->video_file_queue == (Video_Queue *)(-1))
	{
		goto init_error_exit;
	}

	/* create timer to notify saving encoded video file after 5 seconds of warnning occurence */
	video_file_resource->file_store_timer = (user_timer*)malloc(sizeof(user_timer));
	if(!video_file_resource->file_store_timer)
	{
		goto init_error_exit;
	}

	/* initialize timer */
	init_user_timer(video_file_resource->file_store_timer, GET_TICKS_TEST, notify_save_file, (unsigned long)video_file_resource);

	/* initialize SD card and directories for video file storage */
	video_file_resource->file_status = initFileListDir(SD_MOUNT_DIRECTORY, camera_idx, bit_rate, 10); //0:camera 0, 10: 10 seconds

	if(video_file_resource->file_status == (file_status_t*)(-1))
	{
		video_file_resource->sd_card_status = SD_UNMOUNT;
	}
	else
	{
		video_file_resource->sd_card_status = SD_MOUNT;
	}

	DEBUG_INFO(video_file_resource->sd_card_status: %d\n, video_file_resource->sd_card_status);
	return video_file_resource;

init_error_exit:
    if(video_file_resource)
    {
    	if(video_file_resource->t7_video_encode)
    	{
    		destroy_video_encoder(&(video_file_resource->t7_video_encode));
    	}

    	if(video_file_resource->t7_jpeg_encode)
    	{
    		destroy_video_encoder(&(video_file_resource->t7_jpeg_encode));
    	}

    	if(video_file_resource->alert_proof_image)
    	{
    		free(video_file_resource->alert_proof_image);
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
		destroy_video_encoder(&((*video_file_resource)->t7_jpeg_encode));

		if((*video_file_resource)->alert_proof_image)
		{
			free((*video_file_resource)->alert_proof_image);
		}

		destroy_video_queue(&((*video_file_resource)->video_file_queue));

		if((*video_file_resource)->file_store_timer)
		{
			detach_user_timer((*video_file_resource)->file_store_timer);
			free((*video_file_resource)->file_store_timer);
		}

		pthread_mutex_unlock(&((*video_file_resource)->file_lock));
		pthread_mutex_destroy(&((*video_file_resource)->file_lock));
		pthread_cond_destroy(&((*video_file_resource)->file_cond));
		free(*video_file_resource);
		*video_file_resource = NULL;
	}
}







