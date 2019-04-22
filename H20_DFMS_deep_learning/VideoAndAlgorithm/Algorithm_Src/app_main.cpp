/*
 * app_main.c
 *
 *  Created on: Apr 3, 2019
 *      Author: tony
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "algo.h"
#include "gl_display.h"

extern "C"{
#include "producer_consumer_shmfifo.h"
#include "applicfg.h"
#include "t7_camera_v4l2.h"
#include "serial_pack_parse.h"
}


int get_video(char *buf, int size)
{
	memcpy(YUV420_buf, buf, size);
	return 0;
}


void* serial_commu_task(void* argv)
{
	unsigned char recv_buf[256];
	unsigned char send_buf[128];

	unsigned int i = 0;
	int rel_recv_length = 0, spec_recv_length = 130;
	int rel_send_length = 0, spec_send_length = 0;

	while(true)
	{
		if((rel_recv_length = shmfifo_get(pRecvComFifo, recv_buf, spec_recv_length)) > 0)
		{
			DEBUG_LINE();
			DEBUG_INFO(rel_recv_length: %d\n, rel_recv_length);

			for(i=0; i<rel_recv_length; i++)
			{
				printf("%2X", recv_buf[i]);
			}

			printf("\n");

			parse_recv_pack_send(recv_buf, rel_recv_length, send_buf, &spec_send_length);
		}

		rel_send_length = shmfifo_put(pSendComFifo, send_buf, spec_send_length);
		usleep(100000);
	}

	return NULL;
}


int main(int argc, char *argv[])
{
	google::InitGoogleLogging("XXX");
	FLAGS_stderrthreshold = google::ERROR;

	pRecvComFifo = shmfifo_init(RECV_BUF_SHM_KEY, RECV_BUF_SIZE);
	pSendComFifo = shmfifo_init(SEND_BUF_SHM_KEY, SEND_BUF_SIZE);

	if((pRecvComFifo == (void*)-1) || !pRecvComFifo)
	{
		DEBUG_INFO(pRecvComFifo init error!\n);
		return -1;
	}
	else
	{
		DEBUG_INFO(pRecvComFifo init success!\n);
	}

	if((pSendComFifo == (void*)-1) || !pRecvComFifo)
	{
		DEBUG_INFO(pSendComFifo init error!\n);
		return -1;
	}
	else
	{
		DEBUG_INFO(pSendComFifo init success!\n);
	}

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int *para_for_video_thread = (int*)malloc(sizeof(int));
    *para_for_video_thread = 0;

    errno = 0;
    pthread_t video_thread, display_thread;
    callback = get_video;
    int result = pthread_create(&video_thread, &attr,
                    (void* (*)(void*))capture_video, (void *)para_for_video_thread);
    //pthread_attr_destroy(&attr);

    result = pthread_create(&display_thread, &attr,
                    (void* (*)(void*))disp_image, NULL);

    result = pthread_create(&display_thread, &attr,
                    (void* (*)(void*))serial_commu_task, NULL);

    pthread_attr_destroy(&attr);

	Algo *algo = new Algo();
	algo->algoParam.callClassifyScore = 0.95f;
	algo->algoParam.smokeClassifyScore = 0.95f;
	algo->algoParam.yawnClassifyScore = 0.95f;
	algo->algoParam.attenRightScore = 0.9f;
	algo->algoParam.attenLeftScore = 0.9f;
	algo->algoParam.attenUpScore = 0.9f;
	algo->algoParam.attenDownScore = 0.9f;
	algo->algoParam.eyeClassifyScore = 0.7f;
	algo->algoParam.normalScore = 0.9f;

	//检测阈值
	algo->algoParam.callConfidence = 0.3f;
	algo->algoParam.smokeConfidence = 0.3f;
	algo->algoParam.yawnConfidence = 0.3f;
	algo->algoParam.eyeConfidence = 0.3;
	algo->algoParam.smokeDot = 1;

	//报警队列时长
	algo->algoParam.smokeClassifyLevel = 2;
	algo->algoParam.callClassifyLevel = 2;
	algo->algoParam.attenLevel = 2;
	algo->algoParam.yawnLevel = 2;
	algo->algoParam.eyeLevel = 2;
	algo->algoParam.nopLevel = 10;

	//报警队列报警阈值
	algo->algoParam.smokeClassifyDeqConf = 0.6;
	algo->algoParam.callClassifyDeqConf = 0.6;
	algo->algoParam.attenDeqConf = 0.6;
	algo->algoParam.yawnDeqConf = 0.6;
	algo->algoParam.eyeDeqConf = 0.6;

	algo->init(Y8, 1280, 720, algo->algoParam);

	Mat yuv420_image = Mat(720, 1280, CV_8UC1);
	int work_mode = 0;
	int alarm = 0;
	Mat colorImage;

	int image_cnt = 0;
	char saved_image_name[128];

	while(true)
	{
		memcpy(yuv420_image.data, YUV420_buf, 1280*720);
		cvtColor(yuv420_image, colorImage, CV_GRAY2BGR);

		alarm = algo->detectFrame(colorImage, 1, work_mode);

		DEBUG_INFO(alarm: %d\n, alarm);

//		sprintf(saved_image_name, "./camera_%d.jpg", i++);
//		cv::imwrite(saved_image_name, colorImage);
//		spec_recv_length = 130;
//
//		if((rel_recv_length = shmfifo_get(pRecvComFifo, recv_buf, spec_recv_length)) > 0)
//		{
//			DEBUG_LINE();
//			DEBUG_INFO(rel_recv_length: %d\n, rel_recv_length);
//
//			for(i=0; i<rel_recv_length; i++)
//			{
//				printf("%2X", recv_buf[i]);
//			}
//
//			printf("\n");
//		}

		//parse_recv_pack_send(recv_buf, rel_recv_length, send_buf, &spec_send_length);
		//rel_send_length = shmfifo_put(pSendComFifo, send_buf, spec_send_length);

		//ldw_dis_handle->display_buffer(YUV420_buf);
		//disp_image(YUV420_buf);

		serial_output_var.reserved = alarm;

		usleep(50000);
	}

	return 0;
}
