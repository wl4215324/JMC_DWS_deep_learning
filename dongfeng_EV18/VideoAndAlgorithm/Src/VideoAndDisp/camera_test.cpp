/*
 * camera_test.c
 *
 *  Created on: Mar 5, 2019
 *      Author: tony
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <getopt.h>
#include <fcntl.h>            
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h> 
#include <pthread.h>

#include "sunxi_camera.h"

extern "C" {
#include <signal.h>
#include "../VideoStore/user_timer.h"
#include "../VideoStore/warn_video_store.h"
#include "../iniparser/usr_conf.h"
#include "../CurlPost/curl_post.h"
#include "water_mark_interface.h"
}

#include "t7_camera_v4l2.h"
#include "video_layer_test.h"
#include "gl_display.h"
#include "../Algorithm/Src/run_algorithm.hpp"


//void signal_handler(int sig)
//{
//	printf("signal is: %d\n", sig);
//	release_camera_v4l2();
//	exit(-1);
//}

extern Video_File_Resource *dsm_video_record ;
extern Video_File_Resource *monitor_video_record;

int main(int argc, char **argv)
{
/*
	signal(SIGINT, signal_handler);  //interrupt signal for ctrl+c
	signal(SIGKILL, signal_handler);  //kill signal for kill
	signal(SIGSEGV, signal_handler);  // signal for segment fault
*/

    /* initialize configuration file */
	init_conf_file((char*)INI_CONF_FILE_PATH);

    /* initialize timer frame */
	TimerInit();

#ifdef ADD_WATERMARK
	initWaterMark(720);
#endif


#ifdef UPLOAD_WARN_PROOF_TEST
	char *temp = NULL;
	get_token("admin", "rootroot", &temp);
	printf("token: %s\n", temp);
	post_proof(temp, "ev18_dms_test_123", "yawn","2020-04-20 18:27:00", "116.46406,39.898109", "yawn", "/extp/yawn.jpg");
#endif  // for UPLOAD_WARN_PROOF_TEST


#ifdef SAVE_WARN_VIDEO_FILE
	//initialize encoder for dsm
	dsm_video_record = init_video_store(720, 1280, 720, 1280, 4, 25, 0, VENC_CODEC_H264);

	if(dsm_video_record == (Video_File_Resource*)(-1))
	{
		printf("dsm_video_record initial error!\n");
		return -1;
	}

	//initialize encoder for monitor camera
	monitor_video_record = init_video_store(1280, 720, 1280, 720, 4, 25, 1, VENC_CODEC_H264);
	if(monitor_video_record == (Video_File_Resource*)(-1))
	{
		printf("monitor_video_record initial error!\n");
		return -1;
	}
#endif

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t video_and_disp, dfms_task, monitor_tast;
    pthread_t dsm_video_record_task;
    pthread_t monitor_video_record_task;

//  int result = pthread_create(&video_and_disp, &attr, video_layer_test, NULL);
//  pthread_attr_destroy(&attr);

    int result = 0;
    result = pthread_create(&video_and_disp, &attr, capture_video, NULL);

#ifdef SAVE_WARN_VIDEO_FILE
    sleep(1);
    if(dsm_video_record != (Video_File_Resource*)(-1))
    	result = pthread_create(&dsm_video_record_task, &attr, save_warn_video, (void*)dsm_video_record);

    if(monitor_video_record != (Video_File_Resource*)(-1))
    	result = pthread_create(&monitor_video_record_task, &attr, save_warn_video, (void*)monitor_video_record);
#endif

    pthread_attr_destroy(&attr);

    /* run algorithms for dws and off-wheel */
    if(!init_algorithm())
    {
    	run_algorithm( );
    }

    while(1)
    {
    	pause();
    }

	return 0;
}

