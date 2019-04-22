/*
 * serial_pack_parse.c
 *
 *  Created on: Nov 21, 2017
 *      Author: tony
 */

#include "serial_pack_parse.h"

/*
 *
 */
SerialInputVar serial_input_var = {
		.vehicle_speed = 0,
		.turn_signal = 0,
		.accel_pedal = 0,
		.brake_switch = 0,
		.driver_door = 0,
		.engine_speed = 0,
		.small_lamp = 0,
	    .DDWS_switch = 1,
	    .OK_switch = 0,  //
	    .IC_DDWS_switch = 0,
	    .MP5_DDWS_switch = 0,
	    .Cruise_switch = 0,
	    .IC_DDWS_switch_2_3 = 1,
	    .MP5_DDWS_switch_2_3 = 0,
};

SerialOutputVar serial_output_var = {0, 0, 0,}, serial_output_var_test;
pthread_mutex_t serial_output_var_mutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * configuration parameters
 */
ConfigParam config_param = {
		.vehicle_speed = 50,
		.freezing_time = 15,
		.led_power_level = 80,
		.motor_pwm_period = 2,
		.motor_pwm_duty = 50,
		.driver_door_time = 15,
		.brake_time = 20,
		.turn_light_time = 20,
		.acceleator_time = 20,
};


/*
 * default value for dws algorithm inputs
 */
DWSAlgInitVal dws_alg_init_val = {
		.dws_warning_enable_config = 127,
		.level1_closing_eye_time = 3,
		.level2_closing_eye_time = 3,
		.yawn_time = 2,
		.distract_time = 3,
		.somking_time = 1,
		.calling_time = 4,
		.covering_time = 10,
};

D6MessageDataType D6_mesg_data;

static unsigned char serial_recv_buf[512] = {0};
unsigned char serial_send_buf[512] = {0};

int serial_commu_recv_state = 0;

KeyValuePair key_value_list[CONFIG_PARAMS_COUNT] = {
		{"vehicle_speed", 30},
		{"freezing_time", 3},
		{"led_power_level", 80},
		{"motor_pwm_period", 2},
		{"motor_pwm_duty", 75},
		{"fun_config", 127},
		{"level1_closing_eye_time", 3},
		{"level2_closing_eye_time", 3},
		{"yawn_time", 2},
		{"distract_time", 3},
		{"somking_time", 1},
		{"calling_time", 4},
		{"covering_time", 10},
		{"driver_door_freeze_time", 3},
		{"brake_freeze_time", 20},
		{"turn_light_freeze_time", 20},
		{"acceleator_freeze_time", 20},
		{"ddws_switch", 3}
};



/*
 * read specified length data from serial port
 */
static int read_spec_len_data(int fd, unsigned char* recv_buf, int spec_len)
{
	int left_bytes = 0;
	int read_bytes = 0;
	int retry_cnt = 0;
	int i = 0;
	unsigned char *buf_begin = recv_buf;
	left_bytes = spec_len;

	while(left_bytes > 0)
	{
		read_bytes = read(fd, recv_buf, left_bytes);

		if(read_bytes > 0)
		{
			retry_cnt = 0;
		}
		else if(read_bytes < 0)
		{
			if(left_bytes == spec_len)
			{
				return -1;
			}
			else
			{
				break;
			}
		}
		else
		{
			if(left_bytes <= 0)
			{
				break;
			}

			if(retry_cnt++ > 3)
			{
				break;
			}
			else
			{
				usleep(30000);
				continue;
			}
		}

		left_bytes -= read_bytes;
		recv_buf += read_bytes;
	}

//	printf("\n %s data: ", __func__);
//
//	for(i=0; i < spec_len - left_bytes; i++)
//	{
//		DEBUG_INFO(%2X, *(buf_begin+i));
//	}
//
//	printf("\n");


	return (spec_len - left_bytes);
}


/*
 * calculate check sum for data buffer
 */
unsigned short calc_check_sum(unsigned char* data_buf, int data_len)
{
	unsigned short check_sum = 0;
	unsigned short i = 0;

	for(i=0; i<data_len; i++)
	{
		check_sum += *(data_buf+i);
	}

	return check_sum;
}


/*
 * read one intact data frame from serial port
 */
int read_one_frame(int fd, unsigned char* recv_buff, int* recv_frame_leng)
{
	int i = 0;
	int retry_cnt = 0;
	int retval = 0;
	unsigned char temp_buf[8];
	unsigned char message_type, var_cnt;
	unsigned short temp_var;
	unsigned short message_head, message_len, message_len_complement, check_sum;

	while(true)
	{
		if(i < 8)  /* receive frame header of data package */
		{
			retval = read_spec_len_data(fd, temp_buf+i, 1);
		}
		else  /* receive left length of data package */
		{
			retval = read_spec_len_data(fd, recv_buff+8, (message_len-6));

			if(retval >= 0)
			{
				*recv_frame_leng += retval;
				*(recv_buff+MESSAGE_HEAD_INDEX)   = GET_HIG_BYTE_FROM_WORD(message_head);
				*(recv_buff+MESSAGE_HEAD_INDEX+1) = GET_LOW_BYTE_FROM_WORD(message_head);
				*(recv_buff+MESSAGE_LEN_INDEX)   = GET_HIG_BYTE_FROM_WORD(message_len);
				*(recv_buff+MESSAGE_LEN_INDEX+1) = GET_LOW_BYTE_FROM_WORD(message_len);
				*(recv_buff+MESSAGE_LEN_COMPL_INDEX)   = GET_HIG_BYTE_FROM_WORD(message_len_complement);
				*(recv_buff+MESSAGE_LEN_COMPL_INDEX+1) = GET_LOW_BYTE_FROM_WORD(message_len_complement);
				*(recv_buff+MESSAGE_TYPE_ID) = message_type;
				*(recv_buff+MESSAGE_VAR_NUM) = var_cnt;
				check_sum = calc_check_sum(recv_buff+2, message_len-2);

				//DEBUG_INFO(cal check_sum: %2X\n, check_sum);

				//if((0xd6 == *(recv_buff+6)) && ((0x36 == *(recv_buff+7)) || (0x37 == *(recv_buff+7))))
				{
//					for(j=0; j<*recv_frame_leng; j++ )
//					{
//						printf("%02X", recv_buff[j]);
//					}
//
//					DEBUG_INFO(cal check_sum: %4X\n, check_sum);
				}

				if(check_sum == (((*(recv_buff+ *recv_frame_leng -2) & 0xffff) << 8) |*(recv_buff+ *recv_frame_leng -1)))
				{
					return *recv_frame_leng;
				}
				else
				{
					return -1;
				}
			}
			else  //error
			{
				return -1;
			}
		}

		if(retval < 0)
		{
			return -1;
		}
		else if(retval == 0)
		{
			if(retry_cnt++ > 10)
			{
				return -1;
			}
			else
			{
				continue;
			}
		}
		else
		{
			i += retval;

			if(1 == i)  // check first data of received data
			{
				/* if first byte of received data is not 0xAA, continue to check next byte*/
				if(temp_buf[i-1] != 0xAA)
				{
					i = 0;
					*recv_frame_leng = 0;
					retry_cnt = 0;
				}
			}
			else if(2 == i)  // if bytes of received data is 2
			{
				temp_var = MAKE_WORD(temp_buf[i-2], temp_buf[i-1]);

				/* if fist two bytes is 0xAA55*/
				if(MESSAGE_HEAD == temp_var)
				{
					message_head = MESSAGE_HEAD;
					*recv_frame_leng = 2;
					retry_cnt = 0;
					//DEBUG_INFO(%2X\n, temp_var);
				}
				else
				{
					i = 0;
					retry_cnt += 1;
					message_len = 0;
					*recv_frame_leng = 0;
				}
			}
			else if(4 == i)
			{
				temp_var = MAKE_WORD(temp_buf[i-2], temp_buf[i-1]);
				message_len = temp_var;
				*recv_frame_leng = 4;
				retry_cnt = 0;
				//DEBUG_INFO(%2X\n, temp_var);
			}
			else if(6 == i)
			{
				temp_var = MAKE_WORD(temp_buf[i-2], temp_buf[i-1]);
				message_len_complement = temp_var;

				if(0xFFFF == (message_len + message_len_complement))
				{
					*recv_frame_leng = 6;
					retry_cnt = 0;
				}
				else
				{
					i = 0;
					retry_cnt += 1;
					message_len = 0;
					*recv_frame_leng = 0;
				}

				//DEBUG_INFO(%2X\n, temp_var);
			}
			else if(8 == i)
			{
				message_type = temp_buf[i-2];
				var_cnt = temp_buf[i-1];

				if((message_type == D2_MESSAGE) || (message_type == D3_MESSAGE) || (message_type == D4_MESSAGE) ||\
						(message_type == D5_MESSAGE) || (message_type == D6_MESSAGE) || (message_type == D7_MESSAGE))
				{
					*recv_frame_leng = 8;
					retry_cnt = 0;
				}
				else
				{
					i = 0;
					retry_cnt += 1;
					message_len = 0;
					*recv_frame_leng = 0;
				}
			}
			else
			{
				if(retry_cnt++ >40)
				{
					return -1;
				}
			}
		}
	}

	return -1;
}


/*
 * package data frame sent by serial port
 */
int pack_serial_send_message(unsigned char message_type, void* send_data, unsigned char* send_buf, int* send_buf_len)
{
	unsigned char message_type_id, message_var_cnt;
	unsigned short message_head, message_len, message_len_compl, check_sum;

	message_head = MESSAGE_HEAD;
	*send_buf = GET_HIG_BYTE_FROM_WORD(message_head);
	*(send_buf+1) = GET_LOW_BYTE_FROM_WORD(message_head);

	switch(message_type)
	{
	case D2_MESSAGE:
		message_len = 14;
		message_len_compl = ~(14);
		message_type_id = D2_MESSAGE;
		/*
		memcpy(send_buf+7, (unsigned char*)send_data, 5);
		*send_buf_len = 7 + 5;
		*(send_buf+*send_buf_len) = *(unsigned char*)(send_data+6) & 0x0F;
		*(send_buf+*send_buf_len) = *(send_buf+*send_buf_len) << 2;
		*(send_buf+*send_buf_len) = *(send_buf+*send_buf_len) | (*(unsigned char*)(send_data+5) & 0x03);
		*send_buf_len += 1;
		*(send_buf+*send_buf_len) = *(unsigned char*)(send_data+7);
		*send_buf_len += 1;
		*/
		memcpy(send_buf+7, (unsigned char*)send_data, 6);
		*send_buf_len = 7 + 6;
		*(send_buf+*send_buf_len) = *(unsigned char*)(send_data+6);
		*send_buf_len += 1;
		break;

	case D3_MESSAGE:
		message_type_id = D3_MESSAGE;
		*(send_buf+7) = (*(D6MessageDataType *)send_data).threshold_value.vehicle_speed;
		*(send_buf+8) = (*(D6MessageDataType *)send_data).threshold_value.led_power_level;
		*(send_buf+9) = (*(D6MessageDataType *)send_data).threshold_value.freezing_time;
		*(send_buf+10) = (*(D6MessageDataType *)send_data).dws_init_val.dws_warning_enable_config.byte_val;
		*(send_buf+11) = (*(D6MessageDataType *)send_data).threshold_value.motor_pwm_period;
		*(send_buf+12) = (*(D6MessageDataType *)send_data).threshold_value.motor_pwm_duty;
		*(send_buf+13) = (*(D6MessageDataType *)send_data).dws_init_val.level1_closing_eye_time;
		*(send_buf+14) = (*(D6MessageDataType *)send_data).dws_init_val.level2_closing_eye_time;
		*(send_buf+15) = (*(D6MessageDataType *)send_data).dws_init_val.yawn_time;
		*(send_buf+16) = (*(D6MessageDataType *)send_data).dws_init_val.distract_time;
		*(send_buf+17) = (*(D6MessageDataType *)send_data).dws_init_val.somking_time;
		*(send_buf+18) = (*(D6MessageDataType *)send_data).dws_init_val.calling_time;
		*(send_buf+19) = (*(D6MessageDataType *)send_data).dws_init_val.covering_time;
		*(send_buf+20) = (*(D6MessageDataType *)send_data).threshold_value.driver_door_time;
		*(send_buf+21) = (*(D6MessageDataType *)send_data).threshold_value.brake_time;
		*(send_buf+22) = (*(D6MessageDataType *)send_data).threshold_value.turn_light_time;
		*(send_buf+23) = (*(D6MessageDataType *)send_data).threshold_value.acceleator_time;
		//*(send_buf+24) = (unsigned char)(((ARM_APP_SOFTWARE_VER_MAJ&0xff)<<4)|ARM_APP_SOFTWARE_VER_MIN);
		//*send_buf_len = 7 + 18;
		*send_buf_len = 7 + 17;
		message_len = *send_buf_len;
		message_len_compl = ~(message_len);
		break;

	case D4_MESSAGE:
		break;

	case D5_MESSAGE:
		message_len = 8;
		message_len_compl = ~(8);
		message_type_id = D5_MESSAGE;
		*(send_buf+7) = 1;  //result of configuring parameters, 1: success, 0: failed
		*send_buf_len = 7 + 1;
		break;

	case D6_MESSAGE:
		break;

	case D7_MESSAGE:
		break;

	default:
		goto error_return;
		break;
	}

	*(send_buf+2) = GET_HIG_BYTE_FROM_WORD(message_len);
	*(send_buf+3) = GET_LOW_BYTE_FROM_WORD(message_len);
	*(send_buf+4) = GET_HIG_BYTE_FROM_WORD(message_len_compl);
	*(send_buf+5) = GET_LOW_BYTE_FROM_WORD(message_len_compl);
	*(send_buf+6) = message_type_id;
	check_sum = calc_check_sum(send_buf+2, *send_buf_len-2);
	*send_buf_len += 2;
	*(send_buf + *send_buf_len-2) = GET_HIG_BYTE_FROM_WORD(check_sum);
	*(send_buf + *send_buf_len-1) = GET_LOW_BYTE_FROM_WORD(check_sum);
	return *send_buf_len;

error_return:
	return -1;
}




/*
 * type D2 message processing, mainly parsing input variables for DWS algorithm
 */
static int D2_message_process(unsigned char* recv_buf, int recv_buf_len,\
		unsigned char* send_buf, int* send_buf_len)
{
	if(( D2_MESSAGE != *(recv_buf+MESSAGE_TYPE_ID) ) || \
			( (recv_buf_len-2) != MAKE_WORD(*(recv_buf+MESSAGE_LEN_INDEX), *(recv_buf+MESSAGE_LEN_INDEX+1)) ))
	{
		return -1;
	}

//	if(parse_serial_input_var(recv_buf, recv_buf_len) < 0)
//	{
//		return -1;
//	}

	//serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;

	//pthread_mutex_lock(&serial_output_var_mutex);

	/* added on Nov. 27th*/
//	if(somking_freezing_5min_flag.timer_val == WARNING_FREEZE)
//	{
//		serial_output_var.warnning_level.somking_warn = 0;
//	}
//
//	if(phoning_freezing_5min_flag.timer_val == WARNING_FREEZE)
//	{
//		serial_output_var.calling_warn = 0;
//	}
//
//	if(covering_freezing_5min_flag.timer_val == WARNING_FREEZE)
//	{
//		serial_output_var.warnning_level.warning_state = NO_WARNING;
//	}

	//serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
	//pthread_mutex_unlock(&serial_output_var_mutex);

	return pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, send_buf, send_buf_len);
}



/*
 * type D3 message processing, mainly receiving D3 message and replying the values of configuration parameters
 */
static int D3_message_process(unsigned char* recv_buf, int recv_buf_len,\
		unsigned char* send_buf, int* send_buf_len)
{
	if((D3_MESSAGE != *(recv_buf + MESSAGE_TYPE_ID)) || \
			( (recv_buf_len - 2) != MAKE_WORD(*(recv_buf + MESSAGE_LEN_INDEX), *(recv_buf+MESSAGE_LEN_INDEX + 1))))
	{
		return -1;
	}

	D6_mesg_data.threshold_value = config_param;
	D6_mesg_data.dws_init_val = dws_alg_init_val;

	return pack_serial_send_message(D3_MESSAGE, (void*)&D6_mesg_data, send_buf, send_buf_len);
}


/*
 * type D5 message processing, mainly receiving D5 message and configuring specified variables
 */
static int D5_message_process(unsigned char* recv_buf, int recv_buf_len,\
		unsigned char* send_buf, int* send_buf_len)
{
	if(( D5_MESSAGE != *(recv_buf+MESSAGE_TYPE_ID) ) || \
			( (recv_buf_len-2) != MAKE_WORD(*(recv_buf+MESSAGE_LEN_INDEX), *(recv_buf+MESSAGE_LEN_INDEX+1))))
	{
		return -1;
	}

//	if(parse_config_parm(recv_buf, recv_buf_len) < 0)
//	{
//		return -1;
//	}

	return pack_serial_send_message(D5_MESSAGE, (void*)&D6_mesg_data, send_buf, send_buf_len);
}


/*
 * type D6 message processing, mainly receiving bootloader message and
 * replying to MCU
 */
static int D6_message_process(unsigned char* recv_buf, int recv_buf_len,\
		unsigned char* send_buf, int* send_buf_len)
{
	int ret = -1;
	unsigned short send_mesg_len = 0, check_sum = 0;

	if((D6_MESSAGE != *(recv_buf + MESSAGE_TYPE_ID) ) || \
			( (recv_buf_len-2) != MAKE_WORD(*(recv_buf+MESSAGE_LEN_INDEX), *(recv_buf+MESSAGE_LEN_INDEX+1))))
	{
		memcpy(send_buf, recv_buf, recv_buf_len);
		*send_buf_len = recv_buf_len;
		ret = -1;
	}
	else
	{
//		bootloader_main_process(&JMC_bootloader_logic, serial_recv_buf+MESSAGE_VAR_NUM, \
//					recv_buf_len, (send_buf+MESSAGE_VAR_NUM), send_buf_len);
		ret = 0;
	}

	send_mesg_len = *send_buf_len + 7;
	*(send_buf+0) = 0xAA;
	*(send_buf+1) = 0x55;
	*(send_buf+2) = GET_HIG_BYTE_FROM_WORD(send_mesg_len);
	*(send_buf+3) = GET_LOW_BYTE_FROM_WORD(send_mesg_len);
	*(send_buf+4) = GET_HIG_BYTE_FROM_WORD(~send_mesg_len);
	*(send_buf+5) = GET_LOW_BYTE_FROM_WORD(~send_mesg_len);
	*(send_buf+6) = D6_MESSAGE;
	check_sum = calc_check_sum(send_buf+2, send_mesg_len-2);
	*(send_buf+send_mesg_len) = GET_HIG_BYTE_FROM_WORD(check_sum);
	*(send_buf+send_mesg_len+1) = GET_LOW_BYTE_FROM_WORD(check_sum);
	*send_buf_len = send_mesg_len + 2;

	return ret;
}



int parse_recv_pack_send(unsigned char* recv_buf, int recv_buf_len,\
		unsigned char* send_buf, int* send_buf_len)
{
	switch(*(recv_buf+MESSAGE_TYPE_ID))
	{
	case D2_MESSAGE:
		return D2_message_process(recv_buf, recv_buf_len, send_buf, send_buf_len);
		break;

	case D3_MESSAGE:  //read parameters' value
		return D3_message_process(recv_buf, recv_buf_len, send_buf, send_buf_len);
		break;

	case D4_MESSAGE:
		return -1;
		break;

	case D5_MESSAGE:  //write parameters' value
		return D5_message_process(recv_buf, recv_buf_len, send_buf, send_buf_len);
		break;

	case D6_MESSAGE:
		return D6_message_process(recv_buf, recv_buf_len, send_buf, send_buf_len);
		break;

	case D7_MESSAGE:
		return -1;
		break;

	default:
		return -1;
		break;
	}

	return -1;
}



/*
 * send specified length data to serial port
 */
int send_spec_len_data(int fd, unsigned char* send_buf, unsigned short spec_send_data_len)
{
	int nwritten, retry_cnt = 0;
	int nleft = spec_send_data_len;
	struct timeval tp;
	unsigned short i = 0;
	unsigned char *temp_send_buf = send_buf;

	while(nleft > 0)
	{
		if((nwritten = write(fd, send_buf, nleft)) < 0)
		{
			if(nleft == spec_send_data_len)
				return -1;  /* error occur, return -1*/
			else
				break;  /* error, return amount written so far */
		}
		else if(nwritten == 0)
		{
			if(retry_cnt++ > 3)
			{
				break;
			}
			else
			{
				usleep(50000);
			}
		}

		nleft -= nwritten;
		send_buf += nwritten;
	}

	/*
	gettimeofday(&tp, NULL);
	DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);

	puts("serial send data: ");
	for(i=0; i< (spec_send_data_len- nleft); i++)
	{
		printf("%02X ", *(temp_send_buf+i));
	}

	puts("\n");
	*/

	return (spec_send_data_len- nleft);
}



/* rs232 communication task */
//void* serial_commu_app(void* argv)
//int serial_commu_app()
//{
//	int i  = 0, retry_cnt = 0;
//	int fd = 0;
//	fd_set rfds;
//	int recv_length, spec_recv_len;
//	int send_buf_len;
//	unsigned char dws_mesg_array[8] = {0};
//	struct timeval tp;
//	unsigned char work_mode_poll_cnt = 0;
//
//	struct timeval tv = {
//			.tv_sec = 1,
//			.tv_usec = 300000,
//	};
//
//	if((fd = open_set_serial_port()) < 0)
//	{
//		DEBUG_INFO("open internal serial com error!\n");
//		return -1;
//	}
//
//	tcflush(fd, TCIOFLUSH);
//
//	while(true)
//	{
//		FD_ZERO(&rfds);
//		FD_SET(fd, &rfds);
//
//		if(select(fd+1, &rfds, NULL, NULL, &tv) > 0)
//		{
//			/*read serial data from rs232 */
//			if((recv_length = read_one_frame(fd, serial_recv_buf, &spec_recv_len)) > 0)
//			{
//				retry_cnt = 0;
//				serial_commu_recv_state = 0;
//
//				gettimeofday(&tp, NULL);
//				printf("%ld ms recv_length is: %d data:", (tp.tv_sec*1000+tp.tv_usec/1000), recv_length);
//
//				for(i=0; i<recv_length; i++ )
//				{
//					printf("%02X", serial_recv_buf[i]);
//				}
//
//			    printf("\n");
//
//				if(D6_MESSAGE == *(serial_recv_buf+MESSAGE_TYPE_ID))
//				{
//
//				}
//				else
//				{
//
//				}
//
//
//			    /* parse receiving serial data */
//			    if(parse_recv_pack_send(serial_recv_buf, spec_recv_len, serial_send_buf, &send_buf_len) > 0)
//			    {
//			    	if(send_buf_len > 0)
//				    {
//						if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
//						{
//							//printf("periodic serial port send_length is: %d, data: ", send_buf_len);
//
//							if(0xd6 == *(serial_send_buf+6))
//							{
//								gettimeofday(&tp, NULL);
//								printf("%d ms periodic serial port send_length is: %d, data: ", (tp.tv_sec*1000+tp.tv_usec/1000), \
//										send_buf_len);
//
//								for(i=0; i<send_buf_len; i++)
//								{
//									printf("%02X ", serial_send_buf[i]);
//								}
//
//								puts("\n");
//							}
//						}
//						else
//						{
//							printf("send data failed!\n");
//						}
//				    }
//			    }
//			}
//			else
//			{
//				goto recv_error;
//			}
//		}
//		else
//		{
//recv_error:
//			if(retry_cnt++ > 1000)
//			{
//				serial_commu_recv_state = -1;
//				tcflush(fd, TCIFLUSH);
//				retry_cnt = 0;
//			}
//			else
//			{
//				usleep(10000);
//			}
//		}
//	}
//
//	pthread_exit(NULL);
//}
