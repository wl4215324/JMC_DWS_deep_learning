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
		1, 1, 1, 2, 1, 3, 10, 4, 1, 1, 1, 7, 0,};


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



void* algorithm_process_task(void* argv)
{
	unsigned char gray_image[IMAGE_HEIGHT*IMAGE_WIDTH];
	unsigned char local_image[IMAGE_HEIGHT*IMAGE_WIDTH*2];
	unsigned char temp_drowsyLevel = 0;
	int send_buf_len = 0;

	/* initialize algorithm */
	InitParams(&algorithm_input);

	while(true)
	{
		if(true)
		{
			pthread_mutex_lock(&uyvy_image_mutex);
			memcpy(local_image, YUYV_image, sizeof(YUYV_image));
			pthread_mutex_unlock(&uyvy_image_mutex);

			/* get Y component from gray picture of YUYV type */
			get_Ycompnt_from_YUYV(local_image, gray_image, 1);
			algorithm_input.clearBuf = 0;

			if ((0 == ImageProcessing(gray_image, &algorithm_input, &algorithm_output)) && \
				(algorithm_output.drowsyLevel != 100) )
			{
				printf("DWS algorithm_output.drowsyLevel: %d, algorithm_output.faceFlag: %d,"
								"algorithm_output.eyeCloseEventTime: %d\n",
						algorithm_output.drowsyLevel, algorithm_output.faceFlag,
						algorithm_output.eyeCloseEventTime);

				switch (algorithm_output.drowsyLevel)
				{
				case 0:  // no warning
				case 7:  // dangerous driving
				case 100: // warning voice
				default:  //else
					memset(&serial_output_var, 0, sizeof(serial_output_var));
					break;

				case 2:  //yawn
					if(1 == vehicle_speed_judge_flag)
					{
						serial_output_var.warning_state = 1;
						serial_output_var.warning_sub_state = 1;

						pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
						send_spec_len_data(fd, serial_send_buf, send_buf_len);
					}
					else
					{
						memset(&serial_output_var, 0, sizeof(serial_output_var));
					}
					break;

				case 3:  //distraction
					if(1 == vehicle_speed_judge_flag)
					{
						serial_output_var.warning_state = 1;
						serial_output_var.warning_sub_state = 2;

						pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
						send_spec_len_data(fd, serial_send_buf, send_buf_len);
					}
					else
					{
						memset(&serial_output_var, 0, sizeof(serial_output_var));
					}
					break;

				case 4:  //calling phone
					if(1 == vehicle_speed_judge_flag)
					{
						serial_output_var.warning_state = 1;
						serial_output_var.warning_sub_state = 4;

						pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
						send_spec_len_data(fd, serial_send_buf, send_buf_len);
					}
					else
					{
						memset(&serial_output_var, 0, sizeof(serial_output_var));
					}

					break;

				case 5:  //smoking
					if(1 == vehicle_speed_judge_flag)
					{
						if(0 == timer_flag.bits.smoking_10min_interval_flag)
						{
							serial_output_var.warning_state = 1;
							serial_output_var.warning_sub_state = 3;

							pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
							send_spec_len_data(fd, serial_send_buf, send_buf_len);

							timer_flag.bits.smoking_10min_interval_flag = 1;
							timer_flag.bits.smoking_10min_interval_stat = 1;
							SetAlarm(&timer_flag, smoking_10min_interval, &timeout_execute_activity, MIN_TO_TIMEVAL(10), 0);
						}
						else
						{
							memset(&serial_output_var, 0, sizeof(serial_output_var));
						}
					}
					else
					{
						memset(&serial_output_var, 0, sizeof(serial_output_var));
					}
					break;

				case 6:
					if(1 == vehicle_speed_judge_flag)
					{
						serial_output_var.warning_state = 1;
						serial_output_var.warning_sub_state = 5;

						pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
						send_spec_len_data(fd, serial_send_buf, send_buf_len);
					}
					else
					{
						memset(&serial_output_var, 0, sizeof(serial_output_var));
					}

					break;

				case 8:  // closing eye
					if(1 == vehicle_speed_judge_flag)
					{
						serial_output_var.warning_state = 1;
						serial_output_var.warning_sub_state = 6;

						pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
						send_spec_len_data(fd, serial_send_buf, send_buf_len);
					}
					else
					{
						memset(&serial_output_var, 0, sizeof(serial_output_var));
					}
					break;
				}
			}
			else
			{
				;
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

			memset(&serial_output_var, 0, sizeof(serial_output_var));
			usleep(10000);
		}
	}

	pthread_exit(NULL);
}

