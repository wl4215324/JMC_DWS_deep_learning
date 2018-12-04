/*
 * algorithm.cpp
 *
 *  Created on: Mar 19, 2018
 *      Author: tony
 */

#include "algorithm.hpp"

ARITH_INPUT algorithm_input = {
		OBJSEC_INITIALIZER, \
		OBJCONF_INITIALIZER, \
		1, 1, 1, 2, 1, 3, 10, 4, 1, 1, 1, 7, 1,};

ARITH_INPUT algorithm_input_for_day = {
		OBJSEC_INITIALIZER_FOR_DAY, \
		OBJCONF_INITIALIZER_FOR_DAY,\
		1, 1, 1, 2, 1, 3, 10, 4, 1, 1, 1, 7, 1,
};

ARITH_OUTPUT algorithm_output;

int gClearBuf = 0;

static void get_Ycompnt_from_YUYV(const unsigned char* YUYV_image, \
		unsigned char* gray_image, unsigned char YUYV_type)
{
	int i = 0;

	for (i = 0; i < IMAGE_HEIGHT*IMAGE_WIDTH*2; i++)
	{
		if (YUYV_type == 0)
		{
			if (i % 2 == 0)
			{
				*(gray_image++) = *(YUYV_image + i);
			}
		}
		else
		{
			if (i % 2 == 1)
			{
				*(gray_image++) = *(YUYV_image + i);
			}
		}
	}
}



void *algorithm_process(void *argv)
{
	unsigned char gray_image[IMAGE_HEIGHT*IMAGE_WIDTH];
	unsigned char local_image[IMAGE_HEIGHT*IMAGE_WIDTH*2];
	unsigned char last_drowsyLevel = 0;
	int send_buf_len = 0;

	/* initialize algorithm */
	algorithm_input = algorithm_input_for_day;
	algorithm_input.clearBuf = 1;
	InitParams(&algorithm_input);

	while(JMC_bootloader_logic.bootloader_subseq < DownloadDriver)
	{
		if( ((0 == timer_flag.timer_val) && (0 < serial_input_var.DDWS_switch) && (0 == serial_commu_recv_state) && \
		    ((serial_input_var.vehicle_speed>>8) > config_param.vehicle_speed) && (0 == OK_Switch_timer_flag.timer_val)) ||\
		    (1 == rs485_test_flag) )
//		if( ((0 == timer_flag.timer_val) && (0 < serial_input_var.DDWS_switch) && (0 == serial_commu_recv_state) && \
//		    ((serial_input_var.vehicle_speed>>8) > config_param.vehicle_speed) && (0 == OK_Switch_timer_flag.timer_val)) )
		{
			pthread_mutex_lock(&uyvy_image_mutex);
			memcpy(local_image, YUYV_image, sizeof(YUYV_image));

			/* get Y component from gray picture of YUYV type */
			get_Ycompnt_from_YUYV(local_image, gray_image, 1);
			algorithm_input.clearBuf = 0;
			gClearBuf = 0;
			pthread_mutex_unlock(&uyvy_image_mutex);

			/* The algorithm is executed for every second frame */
			if( (0 == ImageProcessing(gray_image, &algorithm_input, &algorithm_output)) && \
				(algorithm_output.drowsyLevel != 100) )
			{
				printf("DWS algorithm_output.drowsyLevel: %d, algorithm_output.faceFlag: %d," \
								"algorithm_output.eyeCloseEventTime: %d\n", \
						algorithm_output.drowsyLevel, algorithm_output.faceFlag, \
						algorithm_output.eyeCloseEventTime);

				temp_drowsyLevel = algorithm_output.drowsyLevel;

				/* if soft-switch is off or timer of driving behavior is not up, DDWS send no warning message */
				if((timer_flag.timer_val > 0) || (0 == serial_input_var.DDWS_switch))
				{
					memset(&algorithm_output, 0, sizeof(algorithm_output));

					pthread_mutex_lock(&serial_output_var_mutex);
					serial_output_var.calling_warn = 0;
					serial_output_var.close_eye_one_level_warn = 0;
					serial_output_var.close_eye_two_level_warn = 0;
					serial_output_var.close_eye_time = 0;
					serial_output_var.distract_warn = 0;
					serial_output_var.yawn_warn = 0;
					serial_output_var.warnning_level.somking_warn = 0;
					serial_output_var.warnning_level.warning_state = 0;
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					pthread_mutex_unlock(&serial_output_var_mutex);

					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
					send_spec_len_data(fd, serial_send_buf, send_buf_len);

					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);

					goto exit_detect;
				}

				switch (algorithm_output.drowsyLevel)
				{
				case 0:  //no warning
				case 7:  //dangerous driving
				case 100: //warning voice
				default:
					/* update warning output variables */
					pthread_mutex_lock(&serial_output_var_mutex);
					serial_output_var.calling_warn = 0;
					serial_output_var.close_eye_one_level_warn = 0;
					serial_output_var.close_eye_two_level_warn = 0;
					serial_output_var.close_eye_time = 0;
					serial_output_var.distract_warn = 0;
					serial_output_var.yawn_warn = 0;
					serial_output_var.warnning_level.somking_warn = 0;
					serial_output_var.warnning_level.warning_state = 0;
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					pthread_mutex_unlock(&serial_output_var_mutex);

					/* clear level 2 close-eye flag and timer */
					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);

					/* if warning exits, then output variables are sent actively via serial port in
					 * order to response quickly.
					*/
					if((2 == last_drowsyLevel) || (3 == last_drowsyLevel) || (4 == last_drowsyLevel) ||\
					   (5 == last_drowsyLevel) || (6 == last_drowsyLevel) || (8 == last_drowsyLevel))
					{
						pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
						send_spec_len_data(fd, serial_send_buf, send_buf_len);
					}
					break;

				case 2:  //yawn
				{
					/* clear cache historical warning */
					kfifo_reset(dws_warn_fifo);

					/* update warning output variables */
					pthread_mutex_lock(&serial_output_var_mutex);

					//if(serial_input_var.DDWS_switch&0x02) //if level 2~3 warning is enabled
					if((serial_input_var.DDWS_switch&0x02) && \
						dws_alg_init_val.dws_warning_enable_config.bits.yawn_enable)
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 1;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = LEVEL_TWO_WARNING;
					}
					else  //if level 2~3 warning is disabled
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}

					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					pthread_mutex_unlock(&serial_output_var_mutex);

					/* send yawn warning message via serial port */
					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
					send_spec_len_data(fd, serial_send_buf, send_buf_len);

					/* clear level 2 close-eye flag and timer */
					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);
				}
				break;

				case 3:  //distraction
					/* clear cache historical warning */
					kfifo_reset(dws_warn_fifo);

					/* update warning output variables */
					pthread_mutex_lock(&serial_output_var_mutex);

					//if(serial_input_var.DDWS_switch&0x02) //if level 2~3 warning is enabled
					if((serial_input_var.DDWS_switch&0x02) && \
						dws_alg_init_val.dws_warning_enable_config.bits.distract_enable)
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 1;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = LEVEL_THREE_WARNING;
					}
					else //if level 2~3 warning is disabled
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}

					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					pthread_mutex_unlock(&serial_output_var_mutex);

					/* send distraction warning via serial port */
					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
					send_spec_len_data(fd, serial_send_buf, send_buf_len);

					/* clear level 2 close-eye flag and timer */
					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);
					break;

				case 4:  //calling phone
					/* update warning output variables */
					pthread_mutex_lock(&serial_output_var_mutex);
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;

					//if(serial_input_var.DDWS_switch&0x01) //if level 1 warning is enabled
					if((serial_input_var.DDWS_switch&0x01) && \
						dws_alg_init_val.dws_warning_enable_config.bits.calling_enable)
					{
						/* if phoning warning is unfrozen, firstly warn then freeze warning */
						if(phoning_freezing_5min_flag.timer_val == WARNING_UNFREEZE)
						{
							serial_output_var.calling_warn = 1;
							serial_output_var.close_eye_one_level_warn = 0;
							serial_output_var.close_eye_two_level_warn = 0;
							serial_output_var.close_eye_time = 0;
							serial_output_var.distract_warn = 0;
							serial_output_var.yawn_warn = 0;
							serial_output_var.warnning_level.somking_warn = 0;
							serial_output_var.warnning_level.warning_state = LEVEL_ONE_WARNING;

							/* freeze phoning warning for 5 minutes */
							phoning_freezing_5min_flag.timer_val = WARNING_FREEZE;
							SetAlarm(&phoning_freezing_5min_flag, phoning_warning_freezing_5min, &timeout_execute_activity,\
									MIN_TO_TIMEVAL(5), 0);

							kfifo_put(dws_warn_fifo, (unsigned char*)&serial_output_var, 8);
//							kfifo_put(dws_warn_fifo, (unsigned char*)&serial_output_var, 8);
//							kfifo_put(dws_warn_fifo, (unsigned char*)&serial_output_var, 8);

							/* send calling phone warning via serial port */
//							pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
//							send_spec_len_data(fd, serial_send_buf, send_buf_len);
						}
						else
						{
							serial_output_var.calling_warn = 0;
							serial_output_var.close_eye_one_level_warn = 0;
							serial_output_var.close_eye_two_level_warn = 0;
							serial_output_var.close_eye_time = 0;
							serial_output_var.distract_warn = 0;
							serial_output_var.yawn_warn = 0;
							serial_output_var.warnning_level.somking_warn = 0;
							serial_output_var.warnning_level.warning_state = NO_WARNING;
						}
					}
					else
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}

					pthread_mutex_unlock(&serial_output_var_mutex);

					/* clear level 2 close-eye flag and timer */
					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);
					break;

				case 5:  //smoking
					/* update warning output variables */
					pthread_mutex_lock(&serial_output_var_mutex);
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					//if(serial_input_var.DDWS_switch&0x01) //if level 1 warning is enabled
					if((serial_input_var.DDWS_switch&0x01) && \
					    dws_alg_init_val.dws_warning_enable_config.bits.smoking_enable)
					{
						/* if smoking warning is unfreeze, firstly warn then freeze warning. added on Nov. 27th*/
						if(somking_freezing_5min_flag.timer_val == WARNING_UNFREEZE)
						{
							serial_output_var.calling_warn = 0;
							serial_output_var.close_eye_one_level_warn = 0;
							serial_output_var.close_eye_two_level_warn = 0;
							serial_output_var.close_eye_time = 0;
							serial_output_var.distract_warn = 0;
							serial_output_var.yawn_warn = 0;
							serial_output_var.warnning_level.somking_warn = 1;
							serial_output_var.warnning_level.warning_state = LEVEL_ONE_WARNING;

							/* freeze smoking warning for 5 minutes */
							somking_freezing_5min_flag.timer_val = WARNING_FREEZE;
							SetAlarm(&somking_freezing_5min_flag, smoking_warning_freezing_5min, &timeout_execute_activity,\
									MIN_TO_TIMEVAL(5), 0);

							kfifo_put(dws_warn_fifo, (unsigned char*)&serial_output_var, 8);
//							kfifo_put(dws_warn_fifo, (unsigned char*)&serial_output_var, 8);
						}
						else
						{
							serial_output_var.calling_warn = 0;
							serial_output_var.close_eye_one_level_warn = 0;
							serial_output_var.close_eye_two_level_warn = 0;
							serial_output_var.close_eye_time = 0;
							serial_output_var.distract_warn = 0;
							serial_output_var.yawn_warn = 0;
							serial_output_var.warnning_level.somking_warn = 0;
							serial_output_var.warnning_level.warning_state = NO_WARNING;
						}
					}
					else  //if level 1 warning is disabled
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}


					pthread_mutex_unlock(&serial_output_var_mutex);

					/* send smoking warning via serial port */
//					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
//					send_spec_len_data(fd, serial_send_buf, send_buf_len);

					/* clear level 2 close-eye flag and timer */
					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);
					break;

				case 6:  //leaving post or covering warning
					pthread_mutex_lock(&serial_output_var_mutex);
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;

					/* if covering waring is unfrozen, firstly warn then freezing warning. added on Nov. 27th */
					if((covering_freezing_5min_flag.timer_val == WARNING_UNFREEZE) && \
							dws_alg_init_val.dws_warning_enable_config.bits.covering_enable)
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = COVER_WARNING;

						/* freeze covering warning for 5 mins */
						covering_freezing_5min_flag.timer_val = WARNING_FREEZE;
						SetAlarm(&covering_freezing_5min_flag, covering_warning_freezing_5min, &timeout_execute_activity,\
								MIN_TO_TIMEVAL(5), 0);

//						kfifo_put(dws_warn_fifo, (unsigned char*)&serial_output_var, 8);
					}
					else
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}

					pthread_mutex_unlock(&serial_output_var_mutex);

//					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
//					send_spec_len_data(fd, serial_send_buf, send_buf_len);

					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);
					break;

				case 8:  //level one closed-eye warning
					/* clear cache historical warning */
					kfifo_reset(dws_warn_fifo);

					if(serial_input_var.DDWS_switch&0x02) //if level 2~3 warning is enabled
					{
						/* if level 1 close-eye warning happened just now */
						if(0 == level2_closing_eye_timer_flag.timer_val)
						{
							if(dws_alg_init_val.dws_warning_enable_config.bits.level1_closing_eye_enable)
							{
								serial_output_var.close_eye_one_level_warn = 1;
								serial_output_var.close_eye_two_level_warn = 0;
								serial_output_var.warnning_level.warning_state = LEVEL_TWO_WARNING;
							}
							else
							{
								serial_output_var.close_eye_one_level_warn = 0;
								serial_output_var.close_eye_two_level_warn = 0;
								serial_output_var.warnning_level.warning_state = NO_WARNING;
							}

							SetAlarm(&level2_closing_eye_timer_flag, level2_closing_eye_timer_1s, &timeout_execute_activity,\
									S_TO_TIMEVAL(1), S_TO_TIMEVAL(1));
							level2_closing_eye_timer_flag.timer_val = 1;
							serial_output_var.close_eye_time = 2;
					        DEBUG_INFO(level2_closing_eye_timer_flag.timer_val: %d\n, \
					        		level2_closing_eye_timer_flag.timer_val);
						}
						/* if level 1 close-eye warning continued less than 2 seconds */
						else if(1 == level2_closing_eye_timer_flag.timer_val)
						{
							if(dws_alg_init_val.dws_warning_enable_config.bits.level1_closing_eye_enable)
							{
								serial_output_var.close_eye_one_level_warn = 1;
								serial_output_var.close_eye_two_level_warn = 0;
								serial_output_var.warnning_level.warning_state = LEVEL_TWO_WARNING;
							}
							else
							{
								serial_output_var.close_eye_one_level_warn = 0;
								serial_output_var.close_eye_two_level_warn = 0;
								serial_output_var.warnning_level.warning_state = NO_WARNING;
							}

					        DEBUG_INFO(level2_closing_eye_timer_flag.timer_val: %d\n, \
					        		level2_closing_eye_timer_flag.timer_val);
						}
						/* if level 1 close-eye warning continued more than 2 seconds */
						else if(2 == level2_closing_eye_timer_flag.timer_val)
						{
							if(dws_alg_init_val.dws_warning_enable_config.bits.level2_closing_eye_enable)
							{
								serial_output_var.close_eye_one_level_warn = 0;
								serial_output_var.close_eye_two_level_warn = 1;
								serial_output_var.warnning_level.warning_state = LEVEL_THREE_WARNING;
							}
							else
							{
								serial_output_var.close_eye_one_level_warn = 0;
								serial_output_var.close_eye_two_level_warn = 0;
								serial_output_var.warnning_level.warning_state = NO_WARNING;
							}

					        DEBUG_INFO(level2_closing_eye_timer_flag.timer_val: %d\n, \
					        		level2_closing_eye_timer_flag.timer_val);
					        DEBUG_INFO(serial_output_var.close_eye_time: %d\n, \
					        		serial_output_var.close_eye_time);
						}
					}
					else  //if level 2~3 warning is disabled
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
						level2_closing_eye_timer_flag.timer_val = 0;
						free_spec_type_alarm(level2_closing_eye_timer_1s);
					}

					//serial_output_var.close_eye_time = 0;
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
					send_spec_len_data(fd, serial_send_buf, send_buf_len);
					break;
				}

				last_drowsyLevel = algorithm_output.drowsyLevel;
			}
			else if(1 == OK_Switch_timer_flag.timer_val)
			{
				pthread_mutex_lock(&serial_output_var_mutex);
				serial_output_var.calling_warn = 0;
				serial_output_var.close_eye_one_level_warn = 0;
				serial_output_var.close_eye_two_level_warn = 0;
				serial_output_var.close_eye_time = 0;
				serial_output_var.distract_warn = 0;
				serial_output_var.yawn_warn = 0;
				serial_output_var.warnning_level.somking_warn = 0;
				serial_output_var.warnning_level.warning_state = 0;
				serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
				pthread_mutex_unlock(&serial_output_var_mutex);

				//level2_closing_eye_timer_flag.timer_val = 0;
				//free_spec_type_alarm(level2_closing_eye_timer_1s);
			}
			else
			{
				/* if driving behavior is not timeout or DDWS is closed, send no warning message immediately*/
				if((timer_flag.timer_val > 0) || (0 == serial_input_var.DDWS_switch))
				{
					memset(&algorithm_output, 0, sizeof(algorithm_output));

					pthread_mutex_lock(&serial_output_var_mutex);
					serial_output_var.calling_warn = 0;
					serial_output_var.close_eye_one_level_warn = 0;
					serial_output_var.close_eye_two_level_warn = 0;
					serial_output_var.close_eye_time = 0;
					serial_output_var.distract_warn = 0;
					serial_output_var.yawn_warn = 0;
					serial_output_var.warnning_level.somking_warn = 0;
					serial_output_var.warnning_level.warning_state = 0;
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					pthread_mutex_unlock(&serial_output_var_mutex);

					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
					send_spec_len_data(fd, serial_send_buf, send_buf_len);

					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);

					goto exit_detect;
				}
				else
				{
					pthread_mutex_lock(&serial_output_var_mutex);

					/* added on Nov. 27th*/
					if((somking_freezing_5min_flag.timer_val == WARNING_FREEZE) && \
					   (temp_drowsyLevel == 5))
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}

					if((phoning_freezing_5min_flag.timer_val == WARNING_FREEZE) && \
					   (temp_drowsyLevel == 4))
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}

					if((covering_freezing_5min_flag.timer_val == WARNING_FREEZE) && \
					   (temp_drowsyLevel == 6))
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}

					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					pthread_mutex_unlock(&serial_output_var_mutex);
				}


//				serial_output_var.calling_warn = 0;
//				serial_output_var.close_eye_one_level_warn = 0;
//				serial_output_var.close_eye_two_level_warn = 0;
//				serial_output_var.close_eye_time = 0;
//				serial_output_var.distract_warn = 0;
//				serial_output_var.yawn_warn = 0;
//				serial_output_var.warnning_level.somking_warn = 0;
//				serial_output_var.warnning_level.warning_state = 0;

				//level2_closing_eye_timer_flag.timer_val = 0;
				//free_spec_type_alarm(level2_closing_eye_timer_1s);
			}

			usleep(10000);
		}
		else  // don't execute DDWS algorithm
		{
			memset(&algorithm_output, 0, sizeof(algorithm_output));

			pthread_mutex_lock(&uyvy_image_mutex);
			memcpy(local_image, YUYV_image, sizeof(YUYV_image));
			pthread_mutex_unlock(&uyvy_image_mutex);

			/* get Y component from gray picture of YUYV type */
			get_Ycompnt_from_YUYV(local_image, gray_image, 1);
			algorithm_input.clearBuf = 1;
			gClearBuf = 1;
			ImageProcessing(gray_image, &algorithm_input, &algorithm_output);

			pthread_mutex_lock(&serial_output_var_mutex);
			serial_output_var.calling_warn = 0;
			serial_output_var.close_eye_one_level_warn = 0;
			serial_output_var.close_eye_two_level_warn = 0;
			serial_output_var.close_eye_time = 0;
			serial_output_var.distract_warn = 0;
			serial_output_var.yawn_warn = 0;
			serial_output_var.warnning_level.somking_warn = 0;
			serial_output_var.warnning_level.warning_state = 0;
			serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
			pthread_mutex_unlock(&serial_output_var_mutex);

			level2_closing_eye_timer_flag.timer_val = 0;
			free_spec_type_alarm(level2_closing_eye_timer_1s);

			temp_drowsyLevel = 0;

exit_detect:
			usleep(10000);
		}
	}

	pthread_exit(NULL);
}

