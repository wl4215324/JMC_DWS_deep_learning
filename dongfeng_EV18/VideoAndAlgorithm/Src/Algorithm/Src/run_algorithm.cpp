/*
 * run_algorithm.c
 *
 *  Created on: Oct 12, 2019
 *      Author: tony
 */


#include "run_algorithm.hpp"
#include "algo.h"
#include "algoFXP.h"

extern "C" {
#include <pthread.h>
#include <string.h>
#include "serial_pack_parse.h"
#include "producer_consumer_shmfifo.h"
#include "warning_logic.h"
#include "../../VideoStore/user_timer.h"
#include "../../VideoStore/warn_video_store.h"
#include "../../VideoStore/files_manager.h"
#include "../../VideoAndDisp/water_mark_interface.h"
#include "../../iniparser/usr_conf.h"
#include "../../CurlPost/curl_post.h"
}

static pthread_mutex_t dfws_image_buf_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned char dfws_image_buf[1280*720*3/2];

static pthread_mutex_t monitor_image_buf_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned char monitor_image_buf[1280*720*3/2];

static unsigned char fxp_alarm = 0;
static pthread_mutex_t fxp_alarm_lock = PTHREAD_MUTEX_INITIALIZER;

static user_timer warn_interval_timers[7];
static user_timer sync_systime_timer;

extern Video_File_Resource *dsm_video_record;
extern Video_File_Resource *monitor_video_record;
extern SingleWaterMark watermark_array[MAX_WATERMARK_NUM];
extern SingleWaterMark monitor_watermark_array[MAX_WATERMARK_NUM];
extern WaterMarkUsrInterface *t7_watermark;

extern UrlParams *url_params;

void copy_dfms_image(unsigned char* src_image)
{
	pthread_mutex_lock(&dfws_image_buf_lock);
	memcpy(dfws_image_buf, src_image, sizeof(dfws_image_buf));
	pthread_mutex_unlock(&dfws_image_buf_lock);
}

void copy_monitor_image(unsigned char* src_image)
{
	pthread_mutex_lock(&monitor_image_buf_lock);
	memcpy(monitor_image_buf, src_image, sizeof(monitor_image_buf));
	pthread_mutex_unlock(&monitor_image_buf_lock);
}


void YUV420spRotate90(uchar *dst, const uchar *src, int srcWidth, int srcHeight)
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


void YUV420spRotateNegative90(uchar *dst, const uchar *src, int srcWidth, int height)
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



void input_variables_judge(SerialInputVar serial_input_var, extern_signal_flags *CAN_signal_flags)
{
	if(serial_input_var.vehicle_speed >= DFMS_ENABLED_SPEED)  // if speed is more than 20km/h
	{
		CAN_signal_flags->bits.vehicle_speed_flag = 0;
	}
	else if(serial_input_var.vehicle_speed < DFMS_DISABLED_SPEED)
	{
		CAN_signal_flags->bits.vehicle_speed_flag = 1;
	}

#ifdef ADD_CAN_SIGNAL_LOGIC
	if(serial_input_var.left_turn_light || serial_input_var.right_turn_light)  //if any of turn lights is lighted
	{
		CAN_signal_flags->bits.turn_light_flag = 1;
	}
	else
	{
		CAN_signal_flags->bits.turn_light_flag = 0;
	}

	if(serial_input_var.driver_door) //if driver door is closed
	{
		CAN_signal_flags->bits.door_flag = 0;
	}
	else
	{
		CAN_signal_flags->bits.door_flag = 1;
	}

	if(serial_input_var.VCU_gear == 0x03)  //if gear status is D
	{
		CAN_signal_flags->bits.gear_flag = 0;
	}
	else
	{
		CAN_signal_flags->bits.gear_flag = 1;
	}

	if(serial_input_var.brake_switch == 0x01)  //if brake is stepped on
	{
		CAN_signal_flags->bits.brake_flag = 1;
	}
	else
	{
		CAN_signal_flags->bits.brake_flag = 0;
	}

	if(serial_input_var.DFMS_switch == 0x01)  //if DFMS is enabled
	{
		CAN_signal_flags->bits.dfms_enable_flag = 0;
	}
	else
	{
		CAN_signal_flags->bits.dfms_enable_flag = 1;
	}
#else
	CAN_signal_flags->bits.turn_light_flag = 0;
	CAN_signal_flags->bits.door_flag = 0;
	CAN_signal_flags->bits.gear_flag = 0;
	CAN_signal_flags->bits.brake_flag = 0;
	CAN_signal_flags->bits.dfms_enable_flag = 0;
#endif

	return;
}

int warn_delay_process(unsigned long data)
{
	return 0;
}


static int sync_systime_with_gps(unsigned long data)
{
	SerialInputVar *serial_input_var = (SerialInputVar*)data;
	struct tm *local;
	unsigned long long sec_t;
	char str_date_time[32];
	unsigned int now_sec = time(NULL);
	sec_t = serial_input_var->msec_after_1970 / 1000;

	if(sec_t >=  now_sec)  //if gps time is normally
	{
		set_datetime_according_ms(serial_input_var->msec_after_1970);
	}
	else
	{
		sec_t = now_sec;
		set_datetime_according_ms(sec_t*1000);
	}

	local = localtime((time_t*)&sec_t);
	strftime(str_date_time, 30, "%Y-%m-%d %H:%M:%S", local);   //24小时制
	change_inifile(INI_CONF_FILE_PATH, SYS_TTIME_SEC":"DATE_TIME_KEY, str_date_time);
	return 0;
}


#define  WARN_INTERVAL_10S  10000000
#define  WARN_RECORD_DELAY_5S  5000000
#define  SYNC_SYS_TIME_PERIOD  60*1000000


void dfms_warn_mapping(unsigned char off_wheel_alarm, unsigned char dfms_alarm, SerialOutputVar *serial_output_var, \
		DFMS_state dfms_work_state)
{
	if(off_wheel_alarm == WHEEL_ALARM)
	{
#ifdef  ADD_WARN_INTERVAL
		if(is_timer_detach(warn_interval_timers))
		{
			init_user_timer(warn_interval_timers, GET_TICKS_TEST+WARN_INTERVAL_10S, warn_delay_process, 0);
			add_user_timer(warn_interval_timers);
			serial_output_var->DFMS_warn.off_wheel = 1;
		}
#else
		serial_output_var->DFMS_warn.off_wheel = 1;
#endif
	}
	else
	{
		serial_output_var->DFMS_warn.off_wheel = 0;
	}

	switch(dfms_alarm)
	{
	case YAWN_ALARM: //打哈欠报警
#ifdef  ADD_WARN_INTERVAL
		if(is_timer_detach(warn_interval_timers+1))
		{
			init_user_timer(warn_interval_timers+1, GET_TICKS_TEST+WARN_INTERVAL_10S, warn_delay_process, 1);
			add_user_timer(warn_interval_timers+1);
		    serial_output_var->DFMS_warn.yawn = 1;
		}
#else
		serial_output_var->DFMS_warn.yawn = 1;
#endif
		break;

	case ATTEN_ALARM: //分神
#ifdef  ADD_WARN_INTERVAL
		if(is_timer_detach(warn_interval_timers+2))
		{
			init_user_timer(warn_interval_timers+2, GET_TICKS_TEST+WARN_INTERVAL_10S, warn_delay_process, 2);
			add_user_timer(warn_interval_timers+2);
		    serial_output_var->DFMS_warn.distraction = 1;
		}
#else
		serial_output_var->DFMS_warn.distraction = 1;
#endif
		break;

	case CALL_ALARM: //打电话报警
#ifdef  ADD_WARN_INTERVAL
		if(is_timer_detach(warn_interval_timers+3))
		{
			init_user_timer(warn_interval_timers+3, GET_TICKS_TEST+WARN_INTERVAL_10S, warn_delay_process, 3);
			add_user_timer(warn_interval_timers+3);
		    serial_output_var->DFMS_warn.phoning = 1;
		}
#else
		serial_output_var->DFMS_warn.phoning = 1;
#endif
		break;

	case SMOKE_ALARM: //抽烟报警
#ifdef  ADD_WARN_INTERVAL
		if(is_timer_detach(warn_interval_timers+4))
		{
			init_user_timer(warn_interval_timers+4, GET_TICKS_TEST+WARN_INTERVAL_10S, warn_delay_process, 4);
			add_user_timer(warn_interval_timers+4);
		    serial_output_var->DFMS_warn.smoking = 1;
		}
#else
		serial_output_var->DFMS_warn.smoking = 1;
#endif
		break;

	case EYES_ALARM: //闭眼报警
#ifdef  ADD_WARN_INTERVAL
		if(is_timer_detach(warn_interval_timers+5))
		{
			init_user_timer(warn_interval_timers+5, GET_TICKS_TEST+WARN_INTERVAL_10S, warn_delay_process, 5);
			add_user_timer(warn_interval_timers+5);
		    serial_output_var->DFMS_warn.close_eye = 1;
		}
#else
		serial_output_var->DFMS_warn.close_eye = 1;
#endif
		break;

	default:
		serial_output_var->DFMS_warn_bits &= 0x01;
		break;
	}

	serial_output_var->DFMS_work_state = dfms_work_state;
	return;
}


void* run_DFMS_algorithm(void *argc)
{
	google::InitGoogleLogging("XXX");
	FLAGS_stderrthreshold = google::ERROR;

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
	/*
	algo->algoParam.smokeClassifyLevel = 7;
	algo->algoParam.callClassifyLevel = 12;  //origin 2
	algo->algoParam.attenLevel = 4;
	algo->algoParam.yawnLevel = 4;
	algo->algoParam.eyeLevel = 2;
	algo->algoParam.nopLevel = 10;
	*/
	algo->algoParam.smokeClassifyLevel = 3;
	algo->algoParam.callClassifyLevel = 3;  //origin 2
	algo->algoParam.attenLevel = 2;
	algo->algoParam.yawnLevel = 2;
	algo->algoParam.eyeLevel = 2;
	algo->algoParam.nopLevel = 10;

	//报警队列报警阈值
	// the other determinant of alarm required time
	algo->algoParam.smokeClassifyDeqConf = 0.7;
	algo->algoParam.callClassifyDeqConf = 0.7;
	algo->algoParam.attenDeqConf = 0.6;
	algo->algoParam.yawnDeqConf = 0.5;
	algo->algoParam.eyeDeqConf = 0.6;  //origin 0.6

	algo->setAlgoParam(algo->algoParam);
	//algo->init(Y8, 720, 1280, algo->algoParam);
	Mat yuv420_image = Mat(1280*3/2, 720, CV_8UC1);
	Mat colorImage;

	unsigned char dfms_alarm = 0;
	unsigned char last_dfms_alarm = 0;
	unsigned char temp_fxp_alarm = 0;
	unsigned char last_fxp_alarm = 0;

	int demo_mode = 1;

	unsigned char send_buf[32];
	int send_buf_len = 0;

	char file_name[64] = "";
	unsigned int i = 0;
	char jpg_watermark[128] = "";
	char temp_warn_type[16] = "";

	while(true)
	{
		pthread_mutex_lock(&dfws_image_buf_lock);
		//YUV420spRotate90(yuv420_image.data, dfws_image_buf, 1280, 720);
		//yuv420_image.data = dfws_image_buf;
		memcpy(yuv420_image.data, dfws_image_buf, sizeof(dfws_image_buf));
		pthread_mutex_unlock(&dfws_image_buf_lock);

		//cvtColor(yuv420_image, colorImage, CV_GRAY2BGR);
		cvtColor(yuv420_image, colorImage, CV_YUV2RGB_I420);
		input_variables_judge(serial_input_var, &CAN_signal_flags);
		//DEBUG_INFO(CAN_signal_flags: %02x\n, CAN_signal_flags.signal_state);
		warning_logic_state_machine(DFMS_health_state, CAN_signal_flags, &DFMS_State);
		//DEBUG_INFO(DFMS_State is : %0d\n, DFMS_State);

		if(DFMS_State == ACTIVE)  //in active mode, run dms algorithm
		{
			dfms_alarm = algo->detectFrame(colorImage, 1, demo_mode);  //run algorithm
			//DEBUG_INFO(algorithm out value: %02X\n, dfms_alarm);
		}
		else if((DFMS_State == CLOSE) || (DFMS_State == FAULT) || (DFMS_State == STANDBY))
		{
			//if in passive, don't run dsm algorithm and assign 0 to result
			dfms_alarm = 0;
		}

//		DEBUG_INFO(run function detectFrame\n);
//		dfms_alarm = algo->detectFrame(colorImage, 1, demo_mode);
//		DEBUG_INFO(algorithm out value: %02X\n, dfms_alarm);

//		if(dfms_alarm > 1)
//		{
//	        sprintf(file_name, "/mnt/sdcard/mmcblk1p1/came_image_%d_%02x.jpg", i, dfms_alarm);
//	        cv::imwrite(file_name, colorImage);
//		}

		/* Use one more local variable to shorten lock code length */
		pthread_mutex_lock(&fxp_alarm_lock);
		temp_fxp_alarm = fxp_alarm;
		pthread_mutex_unlock(&fxp_alarm_lock);

		pthread_mutex_lock(&serial_output_var_mutex);
		dfms_warn_mapping(temp_fxp_alarm, dfms_alarm, &serial_output_var, DFMS_State);
		pthread_mutex_unlock(&serial_output_var_mutex);


//		if(i++ >= 500)
//		{
//			i = 0;
//			serial_output_var.DFMS_warn_bits = 0x5E;
//		}
//		else
//		{
//			serial_output_var.DFMS_warn_bits = 0;
//		}

#ifdef SAVE_WARN_VIDEO_FILE
		if((serial_output_var.DFMS_warn_bits) & 0x5E) //if dws warning happend
		{
			if(serial_output_var.DFMS_warn.close_eye)
			{
				strcpy(temp_warn_type, "CLOSE EYE");
			}
			else if(serial_output_var.DFMS_warn.distraction)
			{
				strcpy(temp_warn_type, "DISTRACTION");
			}
			else if(serial_output_var.DFMS_warn.phoning)
			{
				strcpy(temp_warn_type, "PHONING");
			}
			else if(serial_output_var.DFMS_warn.smoking)
			{
				strcpy(temp_warn_type, "SMOKING");
			}
			else if(serial_output_var.DFMS_warn.yawn)
			{
				strcpy(temp_warn_type, "YAWN");
			}

            #ifdef ADD_WATERMARK
			strcpy((watermark_array+WARN_TYPE_IDX)->content, temp_warn_type);
			sprintf(jpg_watermark, "%d,%d,%s", (watermark_array+WARN_TYPE_IDX)->mPositionX, \
					(watermark_array+WARN_TYPE_IDX)->mPositionY, (watermark_array+WARN_TYPE_IDX)->content);
			addWaterMark(yuv420_image.data, 720, 1280, jpg_watermark, t7_watermark);  //fill warn field of watermark
			usleep(100000);
            #endif

	        get_datetime_according_fmt(dsm_video_record->warn_sys_time);
			get_gps_format_str(serial_input_var.gps_locate_state, serial_input_var.longtitude, \
					serial_input_var.latitude, ',', dsm_video_record->warn_position);

			if((dsm_video_record->file_status != (file_status_t *)(-1)) && \
			   (dsm_video_record->sd_card_status == SD_MOUNT))  //sd card is mounted normally
			{
				if(is_timer_detach(dsm_video_record->file_store_timer))  //if timer is idle, then start 4S timer
				{
					if((dsm_video_record->file_status->file_dir_status == FILE_DIR_NORMAL) && \
						genfilename(dsm_video_record->video_file_name, dsm_video_record->file_status, 0, 4, 10) == 0)
					{
						strcpy(dsm_video_record->warn_type, temp_warn_type);
						init_user_timer(dsm_video_record->file_store_timer, GET_TICKS_TEST+WARN_RECORD_DELAY_5S, notify_save_file, \
								(unsigned long)dsm_video_record);
						add_user_timer(dsm_video_record->file_store_timer);
						dsm_video_record->alert_proof_size = 1280*720*1.5;
						memcpy(dsm_video_record->alert_proof_image, yuv420_image.data, dsm_video_record->alert_proof_size);
					}
				}
			}
			else if(dsm_video_record->sd_card_status == SD_UNMOUNT)
			{
				if(dsm_video_record->file_status != (file_status_t*)(-1)) //object file_status already created
				{
					if(init_file_manager(SD_MOUNT_DIRECTORY, dsm_video_record->file_status, 0, 4, 10) == 0)
					{
						dsm_video_record->sd_card_status = SD_MOUNT;
					}
				}
				else //if object file_status not created yet
				{
					dsm_video_record->file_status = initFileListDir(SD_MOUNT_DIRECTORY, 0, 4, 10);
				}
			}
		}
		else
		{
			memset(temp_warn_type, '\0', sizeof(temp_warn_type));
            #ifdef ADD_WATERMARK
			strcpy((watermark_array+WARN_TYPE_IDX)->content, " ");
            #endif
		}
#endif

		if((last_dfms_alarm != dfms_alarm) || (last_fxp_alarm != temp_fxp_alarm) || (dfms_alarm == 0x40))
		{
			pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, send_buf, &send_buf_len);

//			DEBUG_INFO(put fifo data: );
//			for(i=0; i<send_buf_len; i++)
//			{
//				printf("%02X ", send_buf[i]);
//			}
//
//			printf("\n");

			if(shmfifo_left_size(pSendComFifo) >= send_buf_len)
			{
				shmfifo_put(pSendComFifo, send_buf, send_buf_len);
			}
		}

		last_dfms_alarm = dfms_alarm;
		last_fxp_alarm = fxp_alarm;

		usleep(100000);
	}

	return NULL;
}


void* run_off_wheel_algorithm(void* argc)
{
	unsigned char send_buf[32];
	int send_buf_len = 0;
	unsigned char temp_fxp_alarm = 0;

	FXPAlgo fxp;
	fxp.algoParamFxp.handConf = 0.7;  //org 0.4
	fxp.algoParamFxp.wheelLevel = 5;  //org 2
	fxp.algoParamFxp.wheelDeqConf = 0.9;

	fxp.init();
	Mat framFxpROI;
	float speedFxp =100;
	int demoModeFxp = 0;

	Mat yuv420_image = Mat(720*3/2, 1280, CV_8UC1);
	Mat colorImage;

	char file_name[64];
	unsigned int i = 0;
	char jpg_watermark[128] = "";

	while(true)
	{
		pthread_mutex_lock(&monitor_image_buf_lock);
		memcpy(yuv420_image.data, monitor_image_buf, sizeof(monitor_image_buf));
		pthread_mutex_unlock(&monitor_image_buf_lock);

		if(DFMS_State == ACTIVE)
		{
			cvtColor(yuv420_image, colorImage, CV_YUV2RGB_I420);
			framFxpROI = colorImage(Rect(750, 400, 300,300)).clone();
			temp_fxp_alarm = fxp.detectFXP(framFxpROI,  speedFxp, fxp.algoParamFxp, demoModeFxp);

//			sprintf(file_name, "/extp/origin_image_%04d_%02x.jpg", i, temp_fxp_alarm);
//			cv::imwrite(file_name, colorImage);
//
//			sprintf(file_name, "/extp/detect_image_%04d_%02x.jpg", i, temp_fxp_alarm);
//			cv::imwrite(file_name, framFxpROI);
//			i++;
		}
		else if((DFMS_State == CLOSE) || (DFMS_State == FAULT) || (DFMS_State == STANDBY))
		{
			temp_fxp_alarm = 0;
		}

//		cvtColor(yuv420_image, colorImage, CV_GRAY2BGR);
//		framFxpROI = colorImage(Rect(750, 400, 300,300)).clone();
//		temp_fxp_alarm = fxp.detectFXP(framFxpROI,  speedFxp, fxp.algoParamFxp, demoModeFxp);
//
//	    sprintf(file_name, "/extp/origin_image_%04d_%02x.jpg", i, temp_fxp_alarm);
//	    cv::imwrite(file_name, colorImage);
//
//	    sprintf(file_name, "/extp/detect_image_%04d_%02x.jpg", i, temp_fxp_alarm);
//	    cv::imwrite(file_name, framFxpROI);
//	    i++;

//		framFxpROI = colorImage(Rect(750, 400, 300,300)).clone();
//		temp_fxp_alarm = fxp.detectFXP(framFxpROI,  speedFxp, fxp.algoParamFxp, demoModeFxp);
//		DEBUG_INFO(temp_fxp_alarm: %02X\n, temp_fxp_alarm);

		pthread_mutex_lock(&fxp_alarm_lock);
		fxp_alarm = temp_fxp_alarm;
		pthread_mutex_unlock(&fxp_alarm_lock);
		usleep(100000);

#ifdef SAVE_WARN_VIDEO_FILE
		if((serial_output_var.DFMS_warn_bits) & 0x01) //if off-wheel warning happend
		{
			strcpy(monitor_video_record->warn_type, "OFF WHEEL");

			#ifdef ADD_WATERMARK
			strcpy((monitor_watermark_array+WARN_TYPE_IDX)->content, monitor_video_record->warn_type);
			sprintf(jpg_watermark, "%d,%d,%s", (monitor_watermark_array+WARN_TYPE_IDX)->mPositionX, \
					(monitor_watermark_array+WARN_TYPE_IDX)->mPositionY, (monitor_watermark_array+WARN_TYPE_IDX)->content);
			addWaterMark(yuv420_image.data, 1280, 720, jpg_watermark, t7_watermark);  //fill warn field of watermark
			usleep(200000);
			#endif

			get_datetime_according_fmt(monitor_video_record->warn_sys_time);
			get_gps_format_str(serial_input_var.gps_locate_state, serial_input_var.longtitude, \
					serial_input_var.latitude, ',', monitor_video_record->warn_position);

			if((monitor_video_record->file_status != (file_status_t *)(-1)) && \
			   (monitor_video_record->sd_card_status == SD_MOUNT))  //sd card is mounted normally
			{
				if(is_timer_detach(monitor_video_record->file_store_timer))  //if timer is idle, then start 4S timer
				{
					if((monitor_video_record->file_status->file_dir_status == FILE_DIR_NORMAL) && \
						genfilename(monitor_video_record->video_file_name, monitor_video_record->file_status, 1, 4, 10) == 0)
					{
						init_user_timer(monitor_video_record->file_store_timer, GET_TICKS_TEST+WARN_RECORD_DELAY_5S, notify_save_file, \
								(unsigned long)monitor_video_record);
						add_user_timer(monitor_video_record->file_store_timer);
						monitor_video_record->alert_proof_size = 1280*720*1.5;
						memcpy(monitor_video_record->alert_proof_image, yuv420_image.data, monitor_video_record->alert_proof_size);
					}
				}
			}
			else if(monitor_video_record->sd_card_status == SD_UNMOUNT)
			{
				if(monitor_video_record->file_status != (file_status_t*)(-1)) //object file_status already created
				{
					if(init_file_manager(SD_MOUNT_DIRECTORY, monitor_video_record->file_status, 1, 4, 10) == 0)
					{
						monitor_video_record->sd_card_status = SD_MOUNT;
					}
				}
				else //if object file_status not created yet
				{
					monitor_video_record->file_status = initFileListDir(SD_MOUNT_DIRECTORY, 1, 4, 10);
				}
			}
		}
		else
		{
            #ifdef ADD_WATERMARK
			strcpy((monitor_watermark_array+WARN_TYPE_IDX)->content, " ");
            #endif
		}
#endif //SAVE_WARN_VIDEO_FILE
	}

	return NULL;
}



int init_algorithm()
{
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

	return 0;
}


void* parse_serial_commu(void* argv)
{
	unsigned char recv_buf[256];
	unsigned char send_buf[128];

	int i = 0;
	int rel_recv_length = 0, spec_recv_length = 166;
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

			/* if gps signal is valid, then sync system time */
			if(!serial_input_var.gps_locate_state)
			{
				/* if timing timer is not active, then active it */
				if(is_timer_detach(&sync_systime_timer))
				{
					init_user_timer(&sync_systime_timer, (GET_TICKS_TEST+SYNC_SYS_TIME_PERIOD), \
							sync_systime_with_gps, (unsigned long)(&serial_input_var));
					add_user_timer(&sync_systime_timer);
				}
			}
			else /* stop sync system time */
			{
				detach_user_timer(&sync_systime_timer);
			}

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

			// if vin code in file ini is not equal to the value from MCU
			if(strlen(serial_input_var.vin_code) >= 17 && \
			   strncmp(serial_input_var.vin_code, url_params->vin_code, 17) != 0)
			{
				//update vin code
				strcpy(url_params->vin_code, serial_input_var.vin_code);
				//update vin code in file ini
				change_inifile(INI_CONF_FILE_PATH, PROOF_SERVER_SEC":"VEH_VIN_CODE_KEY, url_params->vin_code);
			}
		}

		usleep(220000);
	}

	return NULL;
}


int run_algorithm()
{
	unsigned long i = 0;
	int result = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for(i=0; i<sizeof(warn_interval_timers)/sizeof(user_timer); i++)
    {
    	init_user_timer(warn_interval_timers+i, GET_TICKS_TEST, warn_delay_process, i);
    }

    init_user_timer((user_timer*)&sync_systime_timer, GET_TICKS_TEST, warn_delay_process, i);

    pthread_t dfms_task, monitor_tast, parse_serial_commu_task;

    result = pthread_create(&parse_serial_commu_task, &attr, parse_serial_commu, NULL);

    result = pthread_create(&dfms_task, &attr, run_DFMS_algorithm, NULL);

    result = pthread_create(&monitor_tast, &attr, run_off_wheel_algorithm, NULL);

    pthread_attr_destroy(&attr);

    return 0;
}





