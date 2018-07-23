/*
 * algorithm.cpp
 *
 *  Created on: Mar 19, 2018
 *      Author: tony
 */

#include "algorithm.hpp"

ARITH_INPUT algorithm_input = XIAOMING_ARITH_INPUT_DEFAULT;
ARITH_OUTPUT algorithm_output;

static void get_Ycompnt_from_YUYV(const unsigned char* YUYV_image,
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
	//unsigned char temp_drowsyLevel = 0;
	int send_buf_len = 0;

	/* initialize algorithm */
	InitParams();

	while(true)
	{
		if( (0 == timer_flag.timer_val) && (1 == serial_input_var.DDWS_switch) && (0 == serial_commu_recv_state) && \
		    ((serial_input_var.vehicle_speed>>8) > config_param.vehicle_speed) && (0 == OK_Switch_timer_flag.timer_val))
		{
			pthread_mutex_lock(&uyvy_image_mutex);
			memcpy(local_image, YUYV_image, sizeof(YUYV_image));
			pthread_mutex_unlock(&uyvy_image_mutex);

			/* get Y component from gray picture of YUYV type */
			get_Ycompnt_from_YUYV(local_image, gray_image, 1);
			algorithm_input.clearBuf = 0;

			if( (0 == ImageProcessing(gray_image, &algorithm_input, &algorithm_output)) && \
				(algorithm_output.drowsyLevel != 100) )
			{
				printf("DWS algorithm_output.drowsyLevel: %d, algorithm_output.faceFlag: %d,"
								"algorithm_output.eyeCloseEventTime: %d\n",
						algorithm_output.drowsyLevel, algorithm_output.faceFlag,
						algorithm_output.eyeCloseEventTime);

				temp_drowsyLevel = algorithm_output.drowsyLevel;

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

					break;

				case 2:  //yawn
				{
					/* update warning output variables */
					pthread_mutex_lock(&serial_output_var_mutex);
					serial_output_var.calling_warn = 0;
					serial_output_var.close_eye_one_level_warn = 0;
					serial_output_var.close_eye_two_level_warn = 0;
					serial_output_var.close_eye_time = 0;
					serial_output_var.distract_warn = 0;
					serial_output_var.yawn_warn = 1;
					serial_output_var.warnning_level.somking_warn = 0;
					serial_output_var.warnning_level.warning_state = LEVEL_TWO_WARNING;
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
					/* update warning output variables */
					pthread_mutex_lock(&serial_output_var_mutex);
					serial_output_var.calling_warn = 0;
					serial_output_var.close_eye_one_level_warn = 0;
					serial_output_var.close_eye_two_level_warn = 0;
					serial_output_var.close_eye_time = 0;
					serial_output_var.distract_warn = 1;
					serial_output_var.yawn_warn = 0;
					serial_output_var.warnning_level.somking_warn = 0;
					serial_output_var.warnning_level.warning_state = LEVEL_THREE_WARNING;
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
					serial_output_var.calling_warn = 1;
					serial_output_var.close_eye_one_level_warn = 0;
					serial_output_var.close_eye_two_level_warn = 0;
					serial_output_var.close_eye_time = 0;
					serial_output_var.distract_warn = 0;
					serial_output_var.yawn_warn = 0;
					serial_output_var.warnning_level.somking_warn = 0;
					serial_output_var.warnning_level.warning_state = LEVEL_ONE_WARNING;
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					pthread_mutex_unlock(&serial_output_var_mutex);

					/* send calling phone warning via serial port */
					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
					send_spec_len_data(fd, serial_send_buf, send_buf_len);

					/* clear level 2 close-eye flag and timer */
					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);
					break;

				case 5:  //smoking
					/* update warning output variables */
					pthread_mutex_lock(&serial_output_var_mutex);
					serial_output_var.calling_warn = 0;
					serial_output_var.close_eye_one_level_warn = 0;
					serial_output_var.close_eye_two_level_warn = 0;
					serial_output_var.close_eye_time = 0;
					serial_output_var.distract_warn = 0;
					serial_output_var.yawn_warn = 0;
					serial_output_var.warnning_level.somking_warn = 1;
					serial_output_var.warnning_level.warning_state = LEVEL_ONE_WARNING;
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					pthread_mutex_unlock(&serial_output_var_mutex);

					/* send smoking warning via serial port */
					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
					send_spec_len_data(fd, serial_send_buf, send_buf_len);

					/* clear level 2 close-eye flag and timer */
					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);
					break;

				case 6:  //leaving post or covering warning
					pthread_mutex_lock(&serial_output_var_mutex);
					serial_output_var.calling_warn = 0;
					serial_output_var.close_eye_one_level_warn = 0;
					serial_output_var.close_eye_two_level_warn = 0;
					serial_output_var.close_eye_time = 0;
					serial_output_var.distract_warn = 0;
					serial_output_var.yawn_warn = 0;
					serial_output_var.warnning_level.somking_warn = 0;
					serial_output_var.warnning_level.warning_state = COVER_WARNING;
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
					pthread_mutex_unlock(&serial_output_var_mutex);

					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
					send_spec_len_data(fd, serial_send_buf, send_buf_len);

					level2_closing_eye_timer_flag.timer_val = 0;
					free_spec_type_alarm(level2_closing_eye_timer_1s);
					break;

				case 8:  //level one closed-eye warning
					/* if level 1 close-eye warning happened just now */
					if(0 == level2_closing_eye_timer_flag.timer_val)
					{
						serial_output_var.close_eye_one_level_warn = 1;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.warnning_level.warning_state = LEVEL_TWO_WARNING;

						SetAlarm(&level2_closing_eye_timer_flag, level2_closing_eye_timer_1s, &timeout_execute_activity,\
								S_TO_TIMEVAL(1), 1);
						level2_closing_eye_timer_flag.timer_val = 1;
						serial_output_var.close_eye_time = 2;
				        DEBUG_INFO(level2_closing_eye_timer_flag.timer_val: %d\n, \
				        		level2_closing_eye_timer_flag.timer_val);
					}
					/* if level 1 close-eye warning continued less than 2 seconds */
					else if(1 == level2_closing_eye_timer_flag.timer_val)
					{
						serial_output_var.close_eye_one_level_warn = 1;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.warnning_level.warning_state = LEVEL_TWO_WARNING;
				        DEBUG_INFO(level2_closing_eye_timer_flag.timer_val: %d\n, \
				        		level2_closing_eye_timer_flag.timer_val);
					}
					/* if level 1 close-eye warning continued more than 2 seconds */
					else if(2 == level2_closing_eye_timer_flag.timer_val)
					{
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 1;
						serial_output_var.warnning_level.warning_state = LEVEL_THREE_WARNING;
				        DEBUG_INFO(level2_closing_eye_timer_flag.timer_val: %d\n, \
				        		level2_closing_eye_timer_flag.timer_val);
					}

					//serial_output_var.close_eye_time = 0;
					pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
					send_spec_len_data(fd, serial_send_buf, send_buf_len);
					break;
				}
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
					serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
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
		else
		{
			memset(&algorithm_output, 0, sizeof(algorithm_output));

			pthread_mutex_lock(&uyvy_image_mutex);
			memcpy(local_image, YUYV_image, sizeof(YUYV_image));
			pthread_mutex_unlock(&uyvy_image_mutex);

			/* get Y component from gray picture of YUYV type */
			get_Ycompnt_from_YUYV(local_image, gray_image, 1);
			algorithm_input.clearBuf = 1;
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

