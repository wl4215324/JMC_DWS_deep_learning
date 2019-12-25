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
#include "warning_logic.h"
#include "user_timer.h"
#include "video_layer.h"
#include "production_test.h"
}

user_timer test_timer[9] ;

extern Factory_Test_Image factory_test_data;

static void YUV420spRotateNegative90(uchar *dst, const uchar *src, int srcWidth, int height)
{
	static int nWidth = 0, nHeight = 0;
	static int wh = 0;
	static int uvHeight = 0;

	if (srcWidth != nWidth || height != nHeight)
	{
		nWidth = srcWidth;
		nHeight = height;
		wh = srcWidth * height;
		uvHeight = height >> 1; //uvHeight = height / 2
	}

	//旋转Y
	int k = 0;
	for (int i = 0; i < srcWidth; i++)
	{
		int nPos = srcWidth - 1;
		for (int j = 0; j < height; j++)
		{
			dst[k] = src[nPos - i];
			k++;
			nPos += srcWidth;
		}
	}

	return;
}

static void YUV420spRotate90(uchar *dst, const uchar *src, int srcWidth, int srcHeight)
{
	static int nWidth = 0, nHeight = 0;
	static int wh = 0;
	static int uvHeight = 0;
	if (srcWidth != nWidth || srcHeight != nHeight)
	{
		nWidth = srcWidth;
		nHeight = srcHeight;
		wh = srcWidth * srcHeight;
		uvHeight = srcHeight >> 1; //uvHeight = height / 2
	}

	//旋转Y
	int k = 0;
	for (int i = 0; i < srcWidth; i++)
	{
		int nPos = 0;
		for (int j = 0; j < srcHeight; j++)
		{
			dst[k] = src[nPos + i];
			k++;
			nPos += srcWidth;
		}
	}

	return;
}


int get_video(char *buf, int size)
{
	pthread_mutex_lock(&camera_buf_lock);
	memcpy(YUV420_buf, buf, size);
	pthread_mutex_unlock(&camera_buf_lock);
	return 0;
}


void* serial_commu_task(void* argv)
{
	unsigned char recv_buf[256];
	unsigned char send_buf[128];

	int i = 0;
	int rel_recv_length = 0, spec_recv_length = 130;
	int rel_send_length = 0, spec_send_length = 0;

	while(true)
	{
		if((rel_recv_length = shmfifo_get(pRecvComFifo, recv_buf, spec_recv_length)) > 0)
		{
//			DEBUG_INFO(pRecvComFifo size: %d\n, shmfifo_len(pRecvComFifo));
//			DEBUG_INFO(rel_recv_length: %d\n, rel_recv_length);
//
//			for(i=0; i<rel_recv_length; i++)
//			{
//				printf("%02X", recv_buf[i]);
//			}
//
//			printf("\n");

			parse_recv_pack_send(recv_buf, rel_recv_length, send_buf, &spec_send_length);
//			DEBUG_INFO(spec_send_length: %d\n, spec_send_length);
//			for(i=0; i<spec_send_length; i++)
//			{
//				printf("%2X", send_buf[i]);
//			}
//			printf("\n");

			if(shmfifo_left_size(pSendComFifo) >= spec_send_length)
			{
				//parse_recv_pack_send(recv_buf, rel_recv_length, send_buf, &spec_send_length);
				pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, send_buf, &spec_send_length);
				rel_send_length = shmfifo_put(pSendComFifo, send_buf, spec_send_length);

//				DEBUG_INFO(rel_send_length: %d\n, rel_send_length);
//
//				for(i=0; i<rel_send_length; i++)
//				{
//					printf("%02X", send_buf[i]);
//				}
//
//				printf("\n");
			}
		}

		usleep(200000);
	}

	return NULL;
}


#define  IMAGE_WIDTH  1280
#define  IMAGE_HEIGHT  720
#define  CUT_OFF_SIZE  25

#define WIDTH_CUT  50
#define HEIGHT_CUT  35


int delay_10S_func (unsigned long data)
{
	DEBUG_INFO(data is: %ld\n, data);

	switch(data)
	{
	case 0:  //turn light
		CAN_signal_flags.bits.turn_light_flag = 0;
		break;

	case 1:  //brake pedal
		CAN_signal_flags.bits.brake_flag = 0;
		break;

	case 2:  //door
		CAN_signal_flags.bits.door_flag = 0;
		break;

	case 3:  //gear
		CAN_signal_flags.bits.gear_flag = 0;
		break;
	}

	return 0;
}


int dimission_delay_60s_func(unsigned long data)
{
	serial_output_var.close_eye_one_level_warn = 2;
	return 0;
}


void input_variables_judge(SerialInputVar serial_input_var, \
		extern_signal_flags *CAN_signal_flags)
{
	static unsigned char exe_once_flag[4] = {0, 0, 0, 0};

	if(serial_input_var.vehicle_speed >= DFMS_ENABLED_SPEED)
	{
		CAN_signal_flags->bits.vehicle_speed_flag = 0;
	}
	else if(serial_input_var.vehicle_speed <= DFMS_DISABLED_SPEED)
	{
		CAN_signal_flags->bits.vehicle_speed_flag = 1;
	}

	if(!serial_input_var.turn_light)
	{
		if(!exe_once_flag[0])
		{
			exe_once_flag[0] = 1;
			if(list_empty(&(test_timer[0].entry)))
			{
				init_user_timer(&test_timer[0], GET_TICKS_TEST+10000000, delay_10S_func, 0);
				add_user_timer(&test_timer[0]);
			}
		}
	}
	else
	{
		CAN_signal_flags->bits.turn_light_flag = 1;
		detach_user_timer(&test_timer[0]);
		exe_once_flag[0] = 0;
	}

	if(((serial_input_var.vehicle_model == 1) && !serial_input_var.EBS_brake_switch) || \
	   ((serial_input_var.vehicle_model == 2) && !serial_input_var.bcm_brake_switch) )  //if brake pedal depressed
	{
		if(!exe_once_flag[1])
		{
			exe_once_flag[1] = 1;

			if(list_empty(&(test_timer[1].entry)))
			{
				init_user_timer(&test_timer[1], GET_TICKS_TEST+10000000, delay_10S_func, 1);
				add_user_timer(&test_timer[1]);
			}
		}
	}
	else
	{
		CAN_signal_flags->bits.brake_flag = 1;
		detach_user_timer(&test_timer[1]);
		exe_once_flag[1] = 0;
	}

	if(!serial_input_var.driver_door && !serial_input_var.side_door)
	{
		if(!exe_once_flag[2])
		{
			exe_once_flag[2] = 1;

			if(list_empty(&(test_timer[2].entry)))
			{
				init_user_timer(&test_timer[2], GET_TICKS_TEST+10000000, delay_10S_func, 2);
				add_user_timer(&test_timer[2]);
			}
		}
	}
	else
	{
		CAN_signal_flags->bits.door_flag = 1;
		detach_user_timer(&test_timer[2]);
		exe_once_flag[2] = 0;
	}

	if((serial_input_var.vehicle_model == 1 && serial_input_var.TCU_gear >= 0x7D && serial_input_var.TCU_gear <= 0xFA) ||\
	   (serial_input_var.vehicle_model == 2 && serial_input_var.RCM_gear == 0x01))
	{
		if(!exe_once_flag[3])
		{
			exe_once_flag[3] = 1;

			if(list_empty(&(test_timer[3].entry)))
			{
				init_user_timer(&test_timer[3], GET_TICKS_TEST+10000000, delay_10S_func, 3);
				add_user_timer(&test_timer[3]);
			}
		}
	}
	else
	{
		CAN_signal_flags->bits.gear_flag = 1;
		detach_user_timer(&test_timer[3]);
		exe_once_flag[3] = 0;
	}

	if((serial_input_var.vehicle_model == 1 && serial_input_var.ps_power_mode == 0x03) ||\
	   (serial_input_var.vehicle_model == 2 && serial_input_var.bcm_power_mode == 0x03))  // power mode is ON
	{
		CAN_signal_flags->bits.power_mode = 0;
	}
	else  // power mode is other states
	{
		CAN_signal_flags->bits.power_mode = 1;
	}

	return;
}


unsigned char smoke_over_10_secs = 0, smoke_frozen_1_min = 0;
unsigned char phone_over_10_secs = 0, phone_frozen_1_min = 0;


int unfreeze_1min_restriction(unsigned long input_param)
{
	if(input_param == 0)
	{
		smoke_frozen_1_min = 0;
		smoke_over_10_secs = 0;
	}
	else
	{
		phone_frozen_1_min = 0;
		phone_over_10_secs = 0;
	}
	//smoke_frozen_1_min = 0;
	return 0;
}


int depress_warn_afer_10_secs(unsigned long input_param)
{
	if(input_param == 0)
	{
		smoke_over_10_secs = 1;
	}
	else
	{
		phone_over_10_secs = 1;
	}

	//smoke_over_10_secs = 1;
	return 0;
}


void smoke_warn_logic_process(unsigned char warn, unsigned int *out_put)
{
	static unsigned char last_warn = 0;

	if(warn == 0x04)  //if current warn is smoking
	{
		if(smoke_frozen_1_min)  //if current state is frozen
		{
			*out_put = 0;
			return ;
		}
		else  //if current state is not frozen
		{
			if(smoke_over_10_secs)  //if continuously smoke over 10 seconds
			{
				smoke_frozen_1_min = 1;

				if(list_empty(&(test_timer[6].entry)))
				{
				    init_user_timer(&test_timer[6], GET_TICKS_TEST+60000000, unfreeze_1min_restriction, 0);
				    add_user_timer(&test_timer[6]);
				}

				*out_put = 0;
			}
			else  // if smoke within 10 seconds
			{
				*out_put = 0x04;

				if(list_empty(&(test_timer[5].entry)))
				{
				    init_user_timer(&test_timer[5], GET_TICKS_TEST+10000000, depress_warn_afer_10_secs, 0);
				    add_user_timer(&test_timer[5]);
				}
			}
		}
	}
	else  // if current state is not smoking
	{
		//smoke_over_10_secs = 0;
		//detach_user_timer(&test_timer[5]);  //delete 10 seconds timer of smoke warn
		if(last_warn == 0x04)  // if current warn is not smoking but last warn is smoking
		{
			if(!smoke_frozen_1_min)  //if current state is not frozen
			{
				smoke_frozen_1_min = 1;  // freeze smoke warn for 1 minute

				if(list_empty(&(test_timer[6].entry)))
				{
				    init_user_timer(&test_timer[6], GET_TICKS_TEST+60000000, unfreeze_1min_restriction, 0);
				    add_user_timer(&test_timer[6]);
				}

				//smoke_over_10_secs = 0;
				//detach_user_timer(&test_timer[5]);  //delete 10 seconds timer of smoke warn
			}

			DEBUG_INFO(\n);
		}
		else
		{
			smoke_over_10_secs = 0;
			detach_user_timer(&test_timer[5]);  //delete 10 seconds timer of smoke warn
		}

		*out_put = warn;
	}

	last_warn = warn;
}


void phone_warn_logic_process(unsigned char warn, unsigned int *out_put)
{
	static unsigned char last_warn = 0;

	if(warn == 0x08)  //if current warn is phone
	{
		if(phone_frozen_1_min)  //if current state is frozen
		{
			*out_put = 0;
			return ;
		}
		else
		{
			if(phone_over_10_secs)  //if continuously phone over 10 seconds
			{
				phone_frozen_1_min = 1;

				if(list_empty(&(test_timer[8].entry)))
				{
				    init_user_timer(&test_timer[8], GET_TICKS_TEST+60000000, unfreeze_1min_restriction, 1);
				    add_user_timer(&test_timer[8]);
				}

				*out_put = 0;
			}
			else  //if continuously phone within 10 seconds
			{
				*out_put = 0x08;

				if(list_empty(&(test_timer[7].entry)))
				{
				    init_user_timer(&test_timer[7], GET_TICKS_TEST+10000000, depress_warn_afer_10_secs, 1);
				    add_user_timer(&test_timer[7]);
				}
			}
		}
	}
	else
	{
		//phone_over_10_secs = 0;
		//detach_user_timer(&test_timer[7]);  //delete 10 seconds timer of phone warn

		if(last_warn == 0x08)  // if last warn is phone, but this warn is not phone
		{
			if(!phone_frozen_1_min)
			{
				phone_frozen_1_min = 1;  // freeze phone warn for 1 minute

				if(list_empty(&(test_timer[8].entry)))
				{
				    init_user_timer(&test_timer[8], GET_TICKS_TEST+60000000, unfreeze_1min_restriction, 1);
				    add_user_timer(&test_timer[8]);
				}
			}
		}
		else
		{
			phone_over_10_secs = 0;
			detach_user_timer(&test_timer[7]);  //delete 10 seconds timer of phone warn
		}

		*out_put = warn;
	}

	last_warn = warn;
}


void dfms_warn_mapping(unsigned int alarm, SerialOutputVar *serial_output_var)
{
	unsigned int output_result1, output_result2;

	switch(alarm)
	{
	case 0x01:  //dimission
		if(list_empty(&(test_timer[4].entry)))
	    {
		    init_user_timer(&test_timer[4], GET_TICKS_TEST+60000000, dimission_delay_60s_func, 0);
		    add_user_timer(&test_timer[4]);
	    }
		break;

	case 0x10:  //close eye
		serial_output_var->close_eye_one_level_warn = 1;
		break;

	case 0x11:  //yawn
		serial_output_var->close_eye_one_level_warn = 32;
		break;

	case 0x12:  //phone
		serial_output_var->close_eye_one_level_warn = 8;
		break;

	case 0x13:  //smoke
		serial_output_var->close_eye_one_level_warn = 4;
		break;

	case 0x14:  //distract
		serial_output_var->close_eye_one_level_warn = 16;
		break;

	default:
		//exe_once_flag[4] = 0;
		detach_user_timer(&test_timer[4]);
		serial_output_var->close_eye_one_level_warn = 0;
		break;
	}

	smoke_warn_logic_process(serial_output_var->close_eye_one_level_warn, &output_result1);
	phone_warn_logic_process(output_result1, &output_result2);
	serial_output_var->close_eye_one_level_warn = output_result2 | 0xC0;
	serial_output_var->close_eye_two_level_warn = DFMS_State|0xF0;
	return;
}


int main(int argc, char *argv[])
{
	unsigned char i = 0;
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

	TimerInit();

	for(i=0; i<sizeof(test_timer)/sizeof(user_timer); i++)
	{
		init_user_timer(test_timer+i, GET_TICKS_TEST, delay_10S_func, 0);
	}

#ifdef FACTORY_TEST
	init_factory_data(&factory_test_data);
#endif

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int *para_for_video_thread = (int*)malloc(sizeof(int));
    *para_for_video_thread = 0;

    errno = 0;
    pthread_t video_thread, display_thread, serial_commu_thread;

    callback = get_video;
    int result = pthread_create(&video_thread, &attr,
                    (void* (*)(void*))capture_video, (void *)para_for_video_thread);

    result = pthread_create(&display_thread, &attr,
                    (void* (*)(void*))disp_image, NULL);

    result = pthread_create(&serial_commu_thread, &attr,
                    (void* (*)(void*))serial_commu_task, NULL);

    pthread_attr_destroy(&attr);

	Algo *algo = new Algo();
	algo->init(Y8, 720, 1280);
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
	// detection thresholds only work in detection mode
	// not work in demo mode
	algo->algoParam.callConfidence = 0.3f;
	algo->algoParam.smokeConfidence = 0.3f;
	algo->algoParam.yawnConfidence = 0.3f;
	algo->algoParam.eyeConfidence = 0.3;
	algo->algoParam.smokeDot = 1;

    // alarm required time = alarm_queue_length * alarm_threshold

	//报警队列时长(unit second)
	// one determinant of alarm required time
	algo->algoParam.smokeClassifyLevel = 2;  //smoke > 1S
	algo->algoParam.callClassifyLevel = 6;   // phone > 4S
	algo->algoParam.attenLevel = 5;  //distract > 3S
	algo->algoParam.yawnLevel = 5;   // yawn > 3S
	algo->algoParam.eyeLevel = 3;    // close eye > 2S
	algo->algoParam.nopLevel = 10;

	//报警队列报警阈值
	// the other determinant of alarm required time
	algo->algoParam.smokeClassifyDeqConf = 0.6;
	algo->algoParam.callClassifyDeqConf = 0.6;
	algo->algoParam.attenDeqConf = 0.6;
	algo->algoParam.yawnDeqConf = 0.6;
	algo->algoParam.eyeDeqConf = 0.6;  //origin 0.6

	algo->setAlgoParam(algo->algoParam);
#ifdef CAMERA_ROTATE
	Mat yuv420_image = Mat(720, 1280, CV_8UC1);
#else

	Mat yuv420_image = Mat(1280, 720, CV_8UC1);
#endif

	int work_mode = 1;
	int alarm = 0, last_alarm = 0;
	Mat colorImage;

	unsigned char send_buf[32];
	int send_buf_len = 0;

	while(true)
	{
#ifdef CAMERA_ROTATE
		memcpy(yuv420_image.data, YUV420_buf, 1280*720);
#else
		//YUV420spRotateNegative90(yuv420_image.data, YUV420_buf, 1280, 720); //ratate cost 16ms
#ifdef FACTORY_TEST
		if(factory_test_data.ecu_mode == NORMAL_WROK)
		{
			YUV420spRotate90(yuv420_image.data, YUV420_buf, 1280, 720);
		}
		else
		{
			YUV420spRotate90(yuv420_image.data, factory_test_data.test_image, 1280, 720);
		}
#else
		YUV420spRotate90(yuv420_image.data, YUV420_buf, 1280, 720);
#endif

#endif
		cvtColor(yuv420_image, colorImage, CV_GRAY2BGR);
		input_variables_judge(serial_input_var, &CAN_signal_flags);
		warning_logic_state_machine(DFMS_health_state, CAN_signal_flags, &DFMS_State);

#ifdef FACTORY_TEST
		if(DFMS_State == ACTIVE || factory_test_data.ecu_mode == FACTORY_TEST)
#else
		if(DFMS_State == ACTIVE)
#endif
		{
			alarm = algo->detectFrame(colorImage, 1, work_mode);
		}
		else if((DFMS_State == CLOSE) || (DFMS_State == FAULT) || (DFMS_State == STANDBY))
		{
			alarm = 0;
		}

		DEBUG_INFO(alarm: %d\n, alarm);
		DEBUG_INFO(DFMS_State is : %0d\n, DFMS_State);

		pthread_mutex_lock(&serial_output_var_mutex);
		dfms_warn_mapping(alarm, &serial_output_var);
		pthread_mutex_unlock(&serial_output_var_mutex);

		if(last_alarm != alarm)
		{
			pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, send_buf, &send_buf_len);

			if(shmfifo_left_size(pSendComFifo) >= send_buf_len)
			{
				shmfifo_put(pSendComFifo, send_buf, send_buf_len);
			}
		}

		last_alarm = alarm;
		usleep(100000);
	}

	return 0;
}



void save_camera_image( )
{
    int image_width = IMAGE_WIDTH - 2*WIDTH_CUT;
    int image_height = IMAGE_HEIGHT -2*HEIGHT_CUT;

    Mat yuv420_image = Mat(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1);
    Mat cut_image = Mat(image_height, image_width, CV_8UC1);
    Mat restore_image = Mat(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1);

    int i = 0, j = 0, n = 0, k = 0;
    char file_name[64] = "";

    for(k=0;k < 5; k++)
    {
    	sleep(1);
        memcpy(yuv420_image.data, YUV420_buf, 1280*720);

        for(i = HEIGHT_CUT; i< image_height+HEIGHT_CUT; i++)
        {
        	for(j=WIDTH_CUT; j<image_width+WIDTH_CUT; j++)
        	{
        		cut_image.data[n++] = yuv420_image.data[i*IMAGE_WIDTH+j];
        	}
        }

//
        cv::resize(cut_image, restore_image, cv::Size(IMAGE_WIDTH, IMAGE_HEIGHT), (0,0), (0,0), cv::INTER_LINEAR);

        sprintf(file_name, "origin_image_%d.jpg", k);
        cv::imwrite(file_name, yuv420_image);
//
        sprintf(file_name, "cut%d_image_%d.jpg", CUT_OFF_SIZE, k);
        cv::imwrite(file_name, cut_image);
//
//
        sprintf(file_name, "restore%d_image_%d.jpg", CUT_OFF_SIZE, k);
        cv::imwrite(file_name, restore_image);

        n = 0;
    }
}
