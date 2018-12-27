/*
 * serial_pack_parse.c
 *
 *  Created on: Nov 21, 2017
 *      Author: tony
 */

#include "serial_pack_parse.h"
#include "user_timer.h"
#include "xml_operation.h"
#include "production_test.h"
#include "v4l2_tvin.h"


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
int fd = 0;
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

		if(read_bytes < 0)
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
		else if(0 == read_bytes)
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
				usleep(50000);
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
static unsigned short calc_check_sum(unsigned char* data_buf, int data_len)
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
static int read_one_frame(int fd, unsigned char* recv_buff, int* recv_frame_leng)
{
	int i = 0, j = 0;
	int retry_cnt = 0;
	int retval = 0;
	unsigned char temp_buf[8];
	unsigned char message_type, var_cnt;
	unsigned short temp_var;
	unsigned short message_head, message_len, message_len_complement, check_sum;

//	struct timeval tp;
//	gettimeofday(&tp, NULL);
	//DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);

	/*
	struct timeval tp;
	gettimeofday(&tp, NULL);
	DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);
	*/

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
					DEBUG_INFO(%2X\n, temp_var);
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
				DEBUG_INFO(%2X\n, temp_var);
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

				DEBUG_INFO(%2X\n, temp_var);
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
 * parse receiving frame and get input variables for DWS algorithm
 */
static int parse_serial_input_var(unsigned char* recv_buf, int recv_buf_len)
{
#if (TRIGGER_STYLE == RISING_EDGE)
	static unsigned short DDWS_switch_temp = 0;
	static unsigned short OK_switch_temp = 0;
#endif

	unsigned int can_frame;
	char value_buf[8] = "";
	int send_buf_len = 0;

	static unsigned short last_vehicle_speed = 0;
    static unsigned short last_turn_signal = 0;
	static unsigned short last_brake_switch = 0;
	static unsigned short last_driver_door = 0;
	static unsigned short last_reverse_gear = 0;
	unsigned short DDWS_switch_temp = 0;
	struct timeval tp;

	/*receiving message length error*/
	if((NULL == recv_buf) || (recv_buf_len < 128))  //128 = 8+12*10
		return -1;

	/* get variable vehicle_speed from receiving data */
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_VEHICLE_SPEED_INDEX), *(recv_buf+MESSAGE_ID_OF_VEHICLE_SPEED_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_VEHICLE_SPEED_INDEX+2), *(recv_buf+MESSAGE_ID_OF_VEHICLE_SPEED_INDEX+3)) == \
			MESSAGE_ID_OF_VEHICLE_SPEED)
	{
		serial_input_var.vehicle_speed = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_VEHICLE_SPEED_INDEX+4, 48, 16);
		//serial_input_var.vehicle_speed = 0x4040;  /* for test */
		DEBUG_INFO(vehicle_speed is: %4X\n, serial_input_var.vehicle_speed);
        
		/* if vehicle is going to stop, device DDWS sends no warning message */
		if( (last_vehicle_speed > 0) && (serial_input_var.vehicle_speed <= 0) )
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
			pthread_mutex_unlock(&serial_output_var_mutex);

			pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);

			if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
			{
				gettimeofday(&tp, NULL);
				DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);
			}
		}

		last_vehicle_speed = serial_input_var.vehicle_speed;
	}
	else
	{
		return -1;
	}

	/* get variable turn_signal from receiving data */
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_TURN_SIGNAL_INDEX), *(recv_buf+MESSAGE_ID_OF_TURN_SIGNAL_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_TURN_SIGNAL_INDEX+2), *(recv_buf+MESSAGE_ID_OF_TURN_SIGNAL_INDEX+3)) == \
			MESSAGE_ID_OF_TURN_SIGNAL)
	{
		serial_input_var.turn_signal = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_TURN_SIGNAL_INDEX+4, 8, 4);
		DEBUG_INFO(turn_signal is: %4X\n, serial_input_var.turn_signal);

		/* if left or right turnning light is going to flash, device DDWS sends no warning message */
		if( (last_turn_signal == 0) && (serial_input_var.turn_signal > 0) )
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
			pthread_mutex_unlock(&serial_output_var_mutex);

			pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);

			if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
			{
				gettimeofday(&tp, NULL);
				DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);
			}
		}

		last_turn_signal = serial_input_var.turn_signal;
	}
	else
	{
		return -1;
	}

	/* get variable accel_pedal from receiving data */
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_ACCEL_PEDAL_INDEX), *(recv_buf+MESSAGE_ID_OF_ACCEL_PEDAL_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_ACCEL_PEDAL_INDEX+2), *(recv_buf+MESSAGE_ID_OF_ACCEL_PEDAL_INDEX+3)) == \
			MESSAGE_ID_OF_ACCEL_PEDAL)
	{
		serial_input_var.accel_pedal = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_ACCEL_PEDAL_INDEX+4, 8, 8);
		DEBUG_INFO(accel_pedal is: %4X\n, serial_input_var.accel_pedal);
	}
	else
	{
		return -1;
	}

	/* get variable brake_switch from receiving data */
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX), *(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX+2), *(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX+3)) == \
			MESSAGE_ID_OF_BRAKE_SWITCH)
	{
		serial_input_var.Cruise_switch = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX+4, 24, 2);
		serial_input_var.brake_switch = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX+4, 28, 2);

		//DEBUG_INFO(Cruise_switch is: %4X\n, serial_input_var.Cruise_switch);
		//DEBUG_INFO(brake_switch is: %4X\n, serial_input_var.brake_switch);
		
		/* if brake switch is going to be actualized, device DDWS sends no warning message*/
        if( (last_brake_switch == 0) && (serial_input_var.brake_switch > 0) )
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
			pthread_mutex_unlock(&serial_output_var_mutex);

			pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);

			if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
			{
				gettimeofday(&tp, NULL);
				DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);
			}
		}

		last_brake_switch = serial_input_var.brake_switch;
	}
	else
	{
		return -1;
	}

	/*get variable driver_door from receiving data*/
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_DRIVER_DOOR_INDEX), *(recv_buf+MESSAGE_ID_OF_DRIVER_DOOR_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_DRIVER_DOOR_INDEX+2), *(recv_buf+MESSAGE_ID_OF_DRIVER_DOOR_INDEX+3)) == \
			MESSAGE_ID_OF_DRIVER_DOOR)
	{
		serial_input_var.driver_door = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DRIVER_DOOR_INDEX+4, 2, 2);
		//DEBUG_INFO(driver_door is: %4X\n, serial_input_var.driver_door);
		
		/* if driver-side door is going to be opened, device DDWS sends no warning message */
		if( (last_driver_door == 0) && (serial_input_var.driver_door > 0) )
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
			pthread_mutex_unlock(&serial_output_var_mutex);

			pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
			if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
			{
				gettimeofday(&tp, NULL);
				DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);
			}
		}

		last_driver_door = serial_input_var.driver_door;
	}
	else
	{
		return -1;
	}

	/*get variable small_lamp from receiving data*/
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_SMALL_LAMP_INDEX), *(recv_buf+MESSAGE_ID_OF_SMALL_LAMP_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_SMALL_LAMP_INDEX+2), *(recv_buf+MESSAGE_ID_OF_SMALL_LAMP_INDEX+3)) == \
			MESSAGE_ID_OF_SMALL_LAMP)
	{
		serial_input_var.small_lamp = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_SMALL_LAMP_INDEX+4, 30, 2);
		//DEBUG_INFO(small_lamp is: %4X\n, serial_input_var.small_lamp);
	}
	else
	{
		return -1;
	}

	/*get variables DDWS_switch and OK_switch from receiving data*/
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX), *(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+2), *(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+3)) == \
			MESSAGE_ID_OF_DDWS_SWITCH)
	{
#if (TRIGGER_STYLE == RISING_EDGE)
		if((0 == DDWS_switch_temp) && (1 == get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+4, 20, 2)))
		{
			serial_input_var.DDWS_switch = serial_input_var.DDWS_switch^1;
		}

		DDWS_switch_temp = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+4, 20, 2);

		if((0 == OK_switch_temp) && (1 == get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+4, 22, 2)))
		{
			serial_input_var.OK_switch = serial_input_var.OK_switch^1;
		}

		OK_switch_temp = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+4, 22, 2);
#else
#if 0
		memcpy(&can_frame, recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+4, sizeof(can_frame));

		if(0xFFFFFFFF != can_frame)
		{
			serial_input_var.IC_DDWS_switch = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+4, 20, 2);
			serial_input_var.OK_switch = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+4, 22, 2);

			if(1 == serial_input_var.DDWS_switch^serial_input_var.IC_DDWS_switch)
			{
				serial_input_var.DDWS_switch = serial_input_var.IC_DDWS_switch;
				/* save DDWS_switch status into file-system */
				sprintf(value_buf, "%d", serial_input_var.DDWS_switch);
				update_node_value(PARAM_CONFIG_XML_PATH, "ddws_switch", value_buf);
			}
		}
#endif
#endif

		if(0xFF == get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX+4, 0, 8))
		{
			serial_input_var.IC_DDWS_switch = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+4, 20, 2);
			serial_input_var.OK_switch = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+4, 22, 2);
			serial_input_var.IC_DDWS_switch_2_3 = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_INDEX+4, 24, 2);

			/* if serial_input_var.OK_switch's value is error or invalid, no response to clicking button */
			if(serial_input_var.OK_switch != 1)
			{
				serial_input_var.OK_switch = 0;
			}

			/* if serial_input_var.IC_DDWS_switch's value is error or invalid, keep previous state */
			if(serial_input_var.IC_DDWS_switch > 1 )
			{
				//serial_input_var.IC_DDWS_switch = 0;
				serial_input_var.IC_DDWS_switch = serial_input_var.DDWS_switch&0x01;
			}

			/* if serial_input_var.IC_DDWS_switch_2_3's value is error or invalid, keep previous state */
			if(serial_input_var.IC_DDWS_switch_2_3 > 1)
			{
				//serial_input_var.IC_DDWS_switch_2_3 = 0;
				serial_input_var.IC_DDWS_switch_2_3 = (serial_input_var.DDWS_switch&0x02)>>1;
			}

			DDWS_switch_temp = serial_input_var.IC_DDWS_switch_2_3<<1;
			DDWS_switch_temp += serial_input_var.IC_DDWS_switch;

			/* if current DDWS switch of IC is different from previous sate */
			if(DDWS_switch_temp != serial_input_var.DDWS_switch)
			{
				serial_input_var.DDWS_switch = DDWS_switch_temp;
				pthread_mutex_lock(&serial_output_var_mutex);

				switch(serial_input_var.DDWS_switch)
				{
				case 0:  //all levels warnings are closed
					serial_output_var.warnning_level.working_state = 0;
				    memset(&serial_output_var, 0, sizeof(serial_output_var));
					break;

				case 1:  //level 1 warning is enabled
					serial_output_var.warnning_level.working_state = 1;
					if( (LEVEL_TWO_WARNING == serial_output_var.warnning_level.warning_state) && \
					    (LEVEL_THREE_WARNING == serial_output_var.warnning_level.warning_state) )
					{
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}
					break;

				case 2:  //level 2 and 3 waring are enabled
					serial_output_var.warnning_level.working_state = 2;
					if(LEVEL_ONE_WARNING == serial_output_var.warnning_level.warning_state)
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}
					break;

				case 3:  //all levels warning are enabled
					serial_output_var.warnning_level.working_state = 3;
					break;
				}
				pthread_mutex_unlock(&serial_output_var_mutex);

				pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
				if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
				{
					gettimeofday(&tp, NULL);
					DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);
				}

				/* save DDWS_switch status into file-system */
				sprintf(value_buf, "%d", serial_input_var.DDWS_switch);
				update_node_value(PARAM_CONFIG_XML_PATH, "ddws_switch", value_buf);
			}
			else //if current DDWS switch of IC is same with previous sate
			{
				;//do nothing
			}


//			if(serial_input_var.IC_DDWS_switch != serial_input_var.DDWS_switch)
//			{
//				serial_input_var.DDWS_switch = serial_input_var.IC_DDWS_switch;
//
//				pthread_mutex_lock(&serial_output_var_mutex);
//				if(0 == serial_input_var.DDWS_switch)  //IC turn off ddws
//				{
//					serial_output_var.warnning_level.working_state = 0;
//					memset(&serial_output_var, 0, sizeof(serial_output_var));
//				}
//				else if(1 == serial_input_var.DDWS_switch)  //IC turn on ddws
//				{
//					serial_output_var.warnning_level.working_state = 1;
//				}
//				pthread_mutex_unlock(&serial_output_var_mutex);
//
//				pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
//				if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
//				{
//					gettimeofday(&tp, NULL);
//					DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);
//				}
//
//				/* save DDWS_switch status into file-system */
//				sprintf(value_buf, "%d", serial_input_var.DDWS_switch);
//				update_node_value(PARAM_CONFIG_XML_PATH, "ddws_switch", value_buf);
//			}
		}
		else
		{
			serial_input_var.OK_switch = 0;
		}

		//DEBUG_INFO(serial_input_var.IC_DDWS_switch: %4X\n, serial_input_var.IC_DDWS_switch);
		//DEBUG_INFO(OK_switch is: %4X\n, serial_input_var.OK_switch);
	}
	else
	{
		return -1;
	}

	/* get variable engine_speed from receiving data */
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX), *(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX+2), *(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX+3)) == \
			MESSAGE_ID_OF_ENGINE_SPEED)
	{
		serial_input_var.engine_speed = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX+4, 24, 16);
		//DEBUG_INFO(engine_speed is: %4X\n, serial_input_var.engine_speed);
	}
	else
	{
		return -1;
	}

	/* get variable MP5_DDWS_switch from receiving data */
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_MP5_INDEX), *(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_MP5_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_MP5_INDEX+2), *(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_MP5_INDEX+3)) == \
			MESSAGE_ID_OF_DDWS_SWITCH_MP5)
	{
#if 0
		memcpy(&can_frame, recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_MP5_INDEX+4, sizeof(can_frame));

		if(0xFFFFFFFF != can_frame)
		{
			serial_input_var.MP5_DDWS_switch = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_MP5_INDEX+4, 20, 2);

			if(1 == serial_input_var.DDWS_switch^serial_input_var.MP5_DDWS_switch)
			{
				serial_input_var.DDWS_switch = serial_input_var.MP5_DDWS_switch;
				/* save DDWS_switch status into file-system */
				sprintf(value_buf, "%d", serial_input_var.DDWS_switch);
				update_node_value(PARAM_CONFIG_XML_PATH, "ddws_switch", value_buf);
			}
		}
#endif
		if(0xFF == get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX+4, 8, 8))
		{
			serial_input_var.MP5_DDWS_switch = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_MP5_INDEX+4, 20, 2);
			serial_input_var.MP5_DDWS_switch_2_3 = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DDWS_SWITCH_MP5_INDEX+4, 22, 2);
			DEBUG_INFO(serial_input_var.MP5_DDWS_switch: %4X serial_input_var.MP5_DDWS_switch_2_3: %4X\
					serial_input_var.DDWS_switch: %4X\n, \
					serial_input_var.MP5_DDWS_switch, serial_input_var.MP5_DDWS_switch_2_3, serial_input_var.DDWS_switch);

			/* if serial_input_var.MP5_DDWS_switch's value is error or invalid, keep previous state */
			if(serial_input_var.MP5_DDWS_switch > 1)
			{
				//serial_input_var.MP5_DDWS_switch = 0;
				serial_input_var.MP5_DDWS_switch = serial_input_var.DDWS_switch&0x01;
			}

			/* if serial_input_var.MP5_DDWS_switch_2_3's value is error or invalid, keep previous state */
			if(serial_input_var.MP5_DDWS_switch_2_3 > 1)
			{
				//serial_input_var.MP5_DDWS_switch_2_3 = 0;
				serial_input_var.MP5_DDWS_switch_2_3 = (serial_input_var.DDWS_switch&0x02)>>1;
			}

			DDWS_switch_temp = serial_input_var.MP5_DDWS_switch_2_3<<1;
			DDWS_switch_temp += serial_input_var.MP5_DDWS_switch;

			/* if current DDWS switch state of MP5 is different from previous sate */
			if(DDWS_switch_temp != serial_input_var.DDWS_switch)
			{
				serial_input_var.DDWS_switch = DDWS_switch_temp;
				pthread_mutex_lock(&serial_output_var_mutex);

				switch(serial_input_var.DDWS_switch)
				{
				case 0:  //all levels warnings are closed
					serial_output_var.warnning_level.working_state = 0;
				    memset(&serial_output_var, 0, sizeof(serial_output_var));
					break;

				case 1:  //level 1 warning is enabled
					serial_output_var.warnning_level.working_state = 1;
					if( (LEVEL_TWO_WARNING == serial_output_var.warnning_level.warning_state) && \
					    (LEVEL_THREE_WARNING == serial_output_var.warnning_level.warning_state) )
					{
						serial_output_var.close_eye_one_level_warn = 0;
						serial_output_var.close_eye_two_level_warn = 0;
						serial_output_var.close_eye_time = 0;
						serial_output_var.distract_warn = 0;
						serial_output_var.yawn_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}
					break;

				case 2:  //level 2 and 3 waring are enabled
					serial_output_var.warnning_level.working_state = 2;
					if(LEVEL_ONE_WARNING == serial_output_var.warnning_level.warning_state)
					{
						serial_output_var.calling_warn = 0;
						serial_output_var.warnning_level.somking_warn = 0;
						serial_output_var.warnning_level.warning_state = NO_WARNING;
					}
					break;

				case 3:  //all levels warning are enabled
					serial_output_var.warnning_level.working_state = 3;
					break;
				}
				pthread_mutex_unlock(&serial_output_var_mutex);

				pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
				if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
				{
					gettimeofday(&tp, NULL);
					DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);
				}

				/* save DDWS_switch status into file-system */
				sprintf(value_buf, "%d", serial_input_var.DDWS_switch);
				update_node_value(PARAM_CONFIG_XML_PATH, "ddws_switch", value_buf);
			}
			else //if current DDWS switch state of MP5 is same with previous sate
			{
				;  //do nothing
			}



//			if(serial_input_var.MP5_DDWS_switch != serial_input_var.DDWS_switch)
//			{
//				serial_input_var.DDWS_switch = serial_input_var.MP5_DDWS_switch;
//
//				pthread_mutex_lock(&serial_output_var_mutex);
//				if(0 == serial_input_var.DDWS_switch)  //mp5 turn off ddws
//				{
//					memset(&serial_output_var, 0, sizeof(serial_output_var));
//					serial_output_var.warnning_level.working_state = 0;
//				}
//				else if(1 == serial_input_var.DDWS_switch)  //mp5 turn on ddws
//				{
//					serial_output_var.warnning_level.working_state = 1;
//				}
//				pthread_mutex_unlock(&serial_output_var_mutex);
//
//				pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);
//				if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
//				{
//					gettimeofday(&tp, NULL);
//					DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);
//				}
//
//				/* save DDWS_switch status into file-system */
//				sprintf(value_buf, "%d", serial_input_var.MP5_DDWS_switch);
//				update_node_value(PARAM_CONFIG_XML_PATH, "ddws_switch", value_buf);
//			}
		}

		//DEBUG_INFO(serial_input_var.MP5_DDWS_switch: %4X\n, serial_input_var.MP5_DDWS_switch);
		//DEBUG_INFO(serial_input_var.DDWS_switch: %4X\n, serial_input_var.DDWS_switch);
	}
	else
	{
		return -1;
	}

	/* get variable reverse_gear from receiving data */
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_REVERSE_GEAR_INDEX), *(recv_buf+MESSAGE_ID_OF_REVERSE_GEAR_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_REVERSE_GEAR_INDEX+2), *(recv_buf+MESSAGE_ID_OF_REVERSE_GEAR_INDEX+3)) == \
			MESSAGE_ID_OF_REVERSE_GEAR)
	{
		serial_input_var.reverse_gear = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_REVERSE_GEAR_INDEX+4, 16, 8);
		DEBUG_INFO(serial_input_var.reverse_gear: %X\n, serial_input_var.reverse_gear);

		/* if reverse_gear is in reverse position or error now, send no warning message */
		if((0 == last_reverse_gear) && (serial_input_var.reverse_gear > 0))
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
			pthread_mutex_unlock(&serial_output_var_mutex);

			pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, serial_send_buf, &send_buf_len);

			if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
			{
				gettimeofday(&tp, NULL);
				DEBUG_INFO(now ms: %d\n, tp.tv_sec*1000 + tp.tv_usec/1000);
			}
		}

		last_reverse_gear = serial_input_var.reverse_gear;
	}
	else
	{
		return -1;
	}


	//DEBUG_INFO(serial_input_var.DDWS_switch: %X\n, serial_input_var.DDWS_switch);
	return 0;
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

	if(parse_serial_input_var(recv_buf, recv_buf_len) < 0)
	{
		return -1;
	}

	//serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;

	pthread_mutex_lock(&serial_output_var_mutex);

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

	serial_output_var.warnning_level.working_state = serial_input_var.DDWS_switch;
	pthread_mutex_unlock(&serial_output_var_mutex);

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
 * update value of configuration parameter into xml file
 */
static int parse_config_parm(unsigned char* recv_buf, int recv_buf_len)
{
	unsigned char var_cnt = 0;
	unsigned char i = 0;
	unsigned short param_index = 0;
	char value_buf[8] = "";

	var_cnt = *(recv_buf+MESSAGE_VAR_NUM);

	if(var_cnt <= 0)
	{
		return -1;
	}

	for(i=0; i<var_cnt; i++)
	{
		param_index = MAKE_WORD(*(recv_buf+8+3*i), *(recv_buf+9+3*i));

		switch(param_index)
		{
		case MESSAGE_ID_OF_TRIGGERING_SPEED:
			config_param.vehicle_speed = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", config_param.vehicle_speed);
			update_node_value(PARAM_CONFIG_XML_PATH, "vehicle_speed", value_buf);
			break;

		case MESSAGE_ID_OF_LED_POWER_LEVEL:
			config_param.led_power_level = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", config_param.led_power_level);
			update_node_value(PARAM_CONFIG_XML_PATH, "led_power_level", value_buf);
			break;

		case MESSAGE_ID_OF_FREEZING_TIME:
			config_param.freezing_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", config_param.freezing_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "freezing_time", value_buf);
			break;

		case MESSAGE_ID_OF_DWS_WARNING_ENABLE_CONFIG:
			dws_alg_init_val.dws_warning_enable_config.byte_val = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", dws_alg_init_val.dws_warning_enable_config.byte_val);
			update_node_value(PARAM_CONFIG_XML_PATH, "fun_config", value_buf);
			break;

		case MESSAGE_ID_OF_MOTOR_PWM_PERIOD:
			config_param.motor_pwm_period = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", config_param.motor_pwm_period);
			update_node_value(PARAM_CONFIG_XML_PATH, "motor_pwm_period", value_buf);
			break;

		case MESSAGE_ID_OF_MOTOR_PWM_DUTY:
			config_param.motor_pwm_duty = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", config_param.motor_pwm_duty);
			update_node_value(PARAM_CONFIG_XML_PATH, "motor_pwm_duty", value_buf);
			break;

		case MESSAGE_ID_OF_LEVEL1_CLOSING_EYE_TIME:
			dws_alg_init_val.level1_closing_eye_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", dws_alg_init_val.level1_closing_eye_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "level1_closing_eye_time", value_buf);
			break;

		case MESSAGE_ID_OF_LEVEL2_CLOSING_EYE_TIME:
			dws_alg_init_val.level2_closing_eye_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", dws_alg_init_val.level2_closing_eye_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "level2_closing_eye_time", value_buf);
			break;

		case MESSAGE_ID_OF_YAWN_TIME:
			dws_alg_init_val.yawn_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", dws_alg_init_val.yawn_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "yawn_time", value_buf);
			break;

		case MESSAGE_ID_OF_DISTRACT_TIME:
			dws_alg_init_val.distract_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", dws_alg_init_val.distract_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "distract_time", value_buf);
			break;

		case MESSAGE_ID_OF_SOMKING_TIME:
			dws_alg_init_val.somking_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", dws_alg_init_val.somking_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "somking_time", value_buf);
			break;

		case MESSAGE_ID_OF_CALLING_TIME:
			dws_alg_init_val.calling_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", dws_alg_init_val.calling_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "calling_time", value_buf);
			break;

		case MESSAGE_ID_OF_COVERING_TIME:
			dws_alg_init_val.covering_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", dws_alg_init_val.covering_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "covering_time", value_buf);
			break;

		case MESSAGE_ID_OF_DRIVERDOOR_TIME:
			config_param.driver_door_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", config_param.driver_door_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "driver_door_freeze_time", value_buf);
			break;

		case MESSAGE_ID_OF_BRAKE_TIME:
			config_param.brake_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", config_param.brake_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "brake_freeze_time", value_buf);
			break;

		case MESSAGE_ID_OF_TURNLIGHT_TIME:
			config_param.turn_light_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", config_param.turn_light_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "turn_light_freeze_time", value_buf);
			break;

		case MESSAGE_ID_OF_ACCELERATOR_TIME:
			config_param.acceleator_time = *(recv_buf+10+3*i);
			sprintf(value_buf, "%d", config_param.acceleator_time);
			update_node_value(PARAM_CONFIG_XML_PATH, "acceleator_freeze_time", value_buf);
			break;

		default:
			return -1;
		}
	}

	return 1;
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

	if(parse_config_parm(recv_buf, recv_buf_len) < 0)
	{
		return -1;
	}

	return pack_serial_send_message(D5_MESSAGE, (void*)&D6_mesg_data, send_buf, send_buf_len);
}


/*
 * type D6 message processing, mainly receiving bootloader message and
 * replying to MCU
 */
static int D6_message_process(unsigned char* recv_buf, int recv_buf_len,\
		unsigned char* send_buf, int* send_buf_len)
{
	unsigned short send_mesg_len = 0, check_sum = 0;

	if(( D6_MESSAGE != *(recv_buf + MESSAGE_TYPE_ID) ) || \
			( (recv_buf_len-2) != MAKE_WORD(*(recv_buf+MESSAGE_LEN_INDEX), *(recv_buf+MESSAGE_LEN_INDEX+1))))
	{
		return -1;
	}

	if (bootloader_main_process(&JMC_bootloader_logic, recv_buf+MESSAGE_TYPE_ID+1, \
			recv_buf_len, (send_buf+7), (unsigned short*)send_buf_len) < 0)
	{
		return -1;
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

	return *send_buf_len;
}



static int parse_recv_pack_send(unsigned char* recv_buf, int recv_buf_len,\
		unsigned char* send_buf, int* send_buf_len)
{
	switch(*(recv_buf+MESSAGE_TYPE_ID))
	{
	case D2_MESSAGE:
		return D2_message_process(recv_buf, recv_buf_len, send_buf, send_buf_len);
		break;

	case D3_MESSAGE:
		return D3_message_process(recv_buf, recv_buf_len, send_buf, send_buf_len);
		break;

	case D4_MESSAGE:
		return -1;
		break;

	case D5_MESSAGE:
		return D5_message_process(recv_buf, recv_buf_len, send_buf, send_buf_len);
		break;

	case D6_MESSAGE:
		DEBUG_INFO(enter D6 message deal\n);
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


void serial_input_var_judge_2(SerialInputVar serial_input_var_temp)
{
	/* if vehicle speed more than 0 */
	if((serial_input_var_temp.vehicle_speed>>8) > 0)
	{
		/* if 15mins timer for engine start for 15mins*/
		if((timer_flag.bits.engine_start_timer_stat == 0) && (timer_flag.bits.engine_start_afer_15min_flag == 1))
		{
			timer_flag.bits.engine_start_timer_stat = 1;
			SetAlarm(&timer_flag, engine_start_after_15min, &timeout_execute_activity,\
					MIN_TO_TIMEVAL(config_param.freezing_time), 0);
					//MIN_TO_TIMEVAL(15), 0);
			printf("engine start, config_param.freezing_time: %d\n", config_param.freezing_time);
		}
	}
	else
	{   /*if vehicle stop*/
#ifdef ENABLE_ACCELERATOR
		timer_flag.timer_val = 0x1f;
#else
		timer_flag.timer_val = 0x0f;
#endif
		free_all_alarm();
		//DEBUG_INFO(engine stop!\n);
	}

	/*if driver door closed*/
	if(serial_input_var_temp.driver_door == 0)
	{
		/*launch 15mins-timer for closing driver side door*/
		if((timer_flag.bits.driver_door_close_timer_stat == 0) && (timer_flag.bits.driver_door_close_after_15min_flag == 1))
		{
			timer_flag.bits.driver_door_close_timer_stat = 1;
			SetAlarm(&timer_flag, driver_door_close_after_15min, &timeout_execute_activity, \
					MIN_TO_TIMEVAL(config_param.driver_door_time), 0);
					//MIN_TO_TIMEVAL(15), 0);
			//printf("driver door close, config_param.driver_door_time: %d\n", config_param.driver_door_time);
		}
	}
	else
	{
		/*if driver door opened now */
#ifdef ENABLE_ACCELERATOR
		timer_flag.timer_val = 0x1f;
#else
		timer_flag.timer_val = 0x0f;
#endif
		free_all_alarm();
		//printf("driver door open!\n");
	}

	/*if turning signal light didn't flash*/
	if(serial_input_var_temp.turn_signal == 0)
	{
		if((timer_flag.bits.turning_light_active_timer_stat == 0) && (timer_flag.bits.turning_light_active_after_20s_flag == 1))
		{
			timer_flag.bits.turning_light_active_timer_stat = 1;
			SetAlarm(&timer_flag, turning_light_active_after_20s, &timeout_execute_activity, \
					S_TO_TIMEVAL(config_param.turn_light_time), 0);
		}
	}/*if turning signal light begin to flash*/
	//else if((serial_input_var_temp.turn_signal == 1)||(serial_input_var_temp.turn_signal == 2))
	else
	{
		timer_flag.bits.turning_light_active_timer_stat = 0;
		timer_flag.bits.turning_light_active_after_20s_flag = 1;
		free_spec_type_alarm(turning_light_active_after_20s);
	}

	/* if brake is freed */
	if(serial_input_var_temp.brake_switch == 0)
	{
		if((timer_flag.bits.brake_active_timer_stat == 0) && (timer_flag.bits.brake_active_after_20s_flag == 1))
		{
			timer_flag.bits.brake_active_timer_stat = 1;
			SetAlarm(&timer_flag, brake_active_after_20s, &timeout_execute_activity, \
					S_TO_TIMEVAL(config_param.brake_time), 0);
		}
	}
	else
	{
		/*if brake is actuated*/
		timer_flag.bits.brake_active_timer_stat = 0;
		timer_flag.bits.brake_active_after_20s_flag = 1;
		free_spec_type_alarm(brake_active_after_20s);
	}

#ifdef ENABLE_ACCELERATOR
	/* if accelerator pedal is kicked down or cruise mode is active */
	if(((0 < serial_input_var_temp.accel_pedal) && (serial_input_var_temp.accel_pedal <= 0xFA)) ||\
		(1 == serial_input_var_temp.Cruise_switch))
	{
		if((timer_flag.bits.accelerator_active_timer_stat == 0) && (timer_flag.bits.accelerator_active_after_20s_flag == 1))
		{
			timer_flag.bits.accelerator_active_timer_stat = 1;
			SetAlarm(&timer_flag, accelerator_active_after_20s, &timeout_execute_activity, \
					S_TO_TIMEVAL(config_param.acceleator_time), 0);
		}
	}
	/*if accelerator pedal is released as well as cruise mode is turned off*/
	//else if((0 ==serial_input_var_temp.accel_pedal) && (0 == serial_input_var_temp.Cruise_switch))
	else if((((0 >= serial_input_var_temp.accel_pedal) || (0xFA < serial_input_var_temp.accel_pedal)) \
			&& (0 == serial_input_var_temp.Cruise_switch)) || \
			(((0 >= serial_input_var_temp.accel_pedal) || (0xFA < serial_input_var_temp.accel_pedal)) \
			&& (1 < serial_input_var_temp.Cruise_switch)))
	{
		/*if pendal is not kicked down*/
		timer_flag.bits.accelerator_active_timer_stat = 0;
		timer_flag.bits.accelerator_active_after_20s_flag = 1;
		free_spec_type_alarm(accelerator_active_after_20s);
	}
#else
	timer_flag.bits.accelerator_active_timer_stat = 0;
	timer_flag.bits.accelerator_active_after_20s_flag = 0;
#endif

	if(0 == serial_input_var_temp.reverse_gear)
	{
		timer_flag.bits.reverse_gear_flag = 0;
		timer_flag.bits.reverse_gear_stat = 0;
	}
	else
	{
		timer_flag.bits.reverse_gear_flag = 1;
		timer_flag.bits.reverse_gear_stat = 1;
	}
}



void serial_input_var_judge(SerialInputVar serial_input_var_temp)
{
	static SerialInputVar serial_input_var_old =
	{
		.driver_door = 1,   /*default door open*/
		.engine_speed = 0,  /*default engine speed 0*/
		.turn_signal = 1,   /**/
		.brake_switch = 1,
		.accel_pedal = 0   /**/
	};
	static i = 0;
	static accelerator_cnt = 0, accelerator_steady = 0;

	if(i == 0)
	{
		i++;
	}
	else
	{
		/* if engine start */
		if(((serial_input_var_temp.engine_speed >> 3) > 0) && ((serial_input_var_old.engine_speed>>3) <= 0) )
		{
			/* if 15mins timer for engine start for 15mins*/
			if(timer_flag.bits.engine_start_afer_15min_flag == 0)
			{
				timer_flag.bits.engine_start_afer_15min_flag = 1;
			}

			SetAlarm(&timer_flag, engine_start_after_15min, &timeout_execute_activity, MIN_TO_TIMEVAL(2), 0);
			printf("engine start!\n");
		}
		else if(((serial_input_var_temp.engine_speed >> 3) <=0 ) && ((serial_input_var_old.engine_speed>>3) >= 0))
		{   /*if engine stop*/
		    timer_flag.bits.engine_start_afer_15min_flag = 1;
			timer_flag.bits.turning_light_active_after_20s_flag = 1;
			timer_flag.bits.accelerator_active_after_20s_flag = 1;
			timer_flag.bits.brake_active_after_20s_flag = 1;
			timer_flag.bits.driver_door_close_after_15min_flag = 1;
			free_all_alarm();
			printf("engine stop!\n");
		}

		/*if driver door closed*/
		if((serial_input_var_old.driver_door == 1) && (serial_input_var_temp.driver_door == 0))
		{
			if(timer_flag.bits.driver_door_close_after_15min_flag == 0)
			{
				timer_flag.bits.driver_door_close_after_15min_flag = 1;
			}

			SetAlarm(&timer_flag, driver_door_close_after_15min, &timeout_execute_activity, MIN_TO_TIMEVAL(2), 0);
			printf("driver door close!\n");
		}
		else if(serial_input_var_temp.driver_door == 1) /*if driver door opened now */
		{
		    timer_flag.bits.engine_start_afer_15min_flag = 1;
			timer_flag.bits.turning_light_active_after_20s_flag = 1;
			timer_flag.bits.accelerator_active_after_20s_flag = 1;
			timer_flag.bits.brake_active_after_20s_flag = 1;
			timer_flag.bits.driver_door_close_after_15min_flag = 1;
			free_all_alarm();
			printf("driver door open!\n");
		}

		/*if turning signal light didn't flash*/
		if(((serial_input_var_old.turn_signal == 1)||(serial_input_var_old.turn_signal == 2)) && \
				(serial_input_var_temp.turn_signal == 0))
		{
			if(timer_flag.bits.turning_light_active_after_20s_flag == 0)
			{
				timer_flag.bits.turning_light_active_after_20s_flag = 1;
			}

			SetAlarm(&timer_flag, turning_light_active_after_20s, &timeout_execute_activity, S_TO_TIMEVAL(20), 0);
		}/*if turning signal light begin to flash*/
		else if((serial_input_var_old.turn_signal == 0) && ((serial_input_var_temp.turn_signal == 1)||\
				(serial_input_var_temp.turn_signal == 2)))
		{
			timer_flag.bits.turning_light_active_after_20s_flag = 1;
			free_spec_type_alarm(turning_light_active_after_20s);
		}

		/* if brake is freed */
		if((serial_input_var_old.brake_switch == 1) && (serial_input_var_temp.brake_switch == 0))
		{
			if(timer_flag.bits.brake_active_after_20s_flag == 0)
			{
				timer_flag.bits.brake_active_after_20s_flag = 1;
			}

			SetAlarm(&timer_flag, brake_active_after_20s, &timeout_execute_activity, S_TO_TIMEVAL(20), 0);

		}
		else if(serial_input_var_temp.brake_switch == 1)
		{
			/*if brake is actuated*/
			timer_flag.bits.brake_active_after_20s_flag = 1;
			free_spec_type_alarm(brake_active_after_20s);
		}

		/**/
		if(((serial_input_var_old.accel_pedal == 0) && (serial_input_var_temp.accel_pedal == 0)) ||\
				((serial_input_var_old.accel_pedal == 1) && (serial_input_var_temp.accel_pedal == 1)))
		{
			if(timer_flag.bits.accelerator_active_after_20s_flag == 0)
			{
				if(accelerator_cnt++ < 1)
				{
					timer_flag.bits.accelerator_active_after_20s_flag = 1;
					SetAlarm(&timer_flag, accelerator_active_after_20s, &timeout_execute_activity, S_TO_TIMEVAL(20), 0);
				}
				else
				{
					accelerator_cnt = 1;
				}
			}
			else if(accelerator_steady == 1)
			{
				accelerator_steady = 0;
				SetAlarm(&timer_flag, accelerator_active_after_20s, &timeout_execute_activity, S_TO_TIMEVAL(20), 0);
			}
		}
		else if(((serial_input_var_old.accel_pedal == 0) && (serial_input_var_temp.accel_pedal == 1)) ||\
				((serial_input_var_old.accel_pedal == 1) && (serial_input_var_temp.accel_pedal == 0)))
		{
			/*if pedal is actuated*/
			timer_flag.bits.accelerator_active_after_20s_flag = 1;
			free_spec_type_alarm(accelerator_active_after_20s);
			accelerator_cnt = 0;
			accelerator_steady = 1;
		}

		serial_input_var_old = serial_input_var_temp;
	}
}


/* rs232 communication task */
void* serial_commu_app(void* argv)
{
	int i  = 0, retry_cnt = 0;
	fd_set rfds;
	int recv_length, spec_recv_len;
	int send_buf_len;
	unsigned char dws_mesg_array[8] = {0};
	struct timeval tp;
	unsigned char work_mode_poll_cnt = 0;

	struct timeval tv = {
			.tv_sec = 1,
			.tv_usec = 300000,
	};

	tcflush(fd, TCIOFLUSH);

	while(true)
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		if(select(fd+1, &rfds, NULL, NULL, &tv) > 0)
		{
			/* logic processing for production test mode polling */
			if(work_mode_poll_cnt++ > 5)
			{
				work_mode_poll_cnt = 0;
				get_production_test_mode();
			}

			/*read serial data from rs232 */
			if((recv_length = read_one_frame(fd, serial_recv_buf, &spec_recv_len)) > 0)
			{
				retry_cnt = 0;
				serial_commu_recv_state = 0;

				//if(0xd6 == *(serial_recv_buf+6))
				{
					gettimeofday(&tp, NULL);

					printf("%d ms recv_length is: %d data:", (tp.tv_sec*1000+tp.tv_usec/1000), recv_length);

					for(i=0; i<recv_length; i++ )
					{
						printf("%02X", serial_recv_buf[i]);
					}

				    printf("\n");
				}

			    /* parse receiving serial data */
			    if(parse_recv_pack_send(serial_recv_buf, spec_recv_len, serial_send_buf, &send_buf_len) > 0)
			    {
			    	serial_input_var_judge_2(serial_input_var);

			    	/*get dws warning message from cache fifo*/
			    	if(kfifo_len(dws_warn_fifo) > 0)
			    	{
			    		if((0 == timer_flag.timer_val) && (0 < serial_input_var.DDWS_switch))
			    		{
				    		kfifo_get(dws_warn_fifo, dws_mesg_array, sizeof(dws_mesg_array));
				    		pack_serial_send_message(D2_MESSAGE, dws_mesg_array, serial_send_buf, &send_buf_len);
			    		}
			    		else
			    		{
			    			kfifo_reset(dws_warn_fifo);
			    		}
			    	}
			    	else
			    	{
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
			    	}

			    	if(send_buf_len > 0)
				    {
						if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
						{
							//printf("periodic serial port send_length is: %d, data: ", send_buf_len);

							if(0xd6 == *(serial_send_buf+6))
							{
								gettimeofday(&tp, NULL);
								printf("%d ms periodic serial port send_length is: %d, data: ", (tp.tv_sec*1000+tp.tv_usec/1000), \
										send_buf_len);

								for(i=0; i<send_buf_len; i++)
								{
									printf("%02X ", serial_send_buf[i]);
								}

								puts("\n");
							}
						}
						else
						{
							printf("send data failed!\n");
						}
				    }

			    	if(JMC_bootloader_logic.bootloader_subseq >= ResetECU)
			    	{
			    		bootloader_completetion(&JMC_bootloader_logic);
			    		DEBUG_INFO(exit jmc_dws program\n);
			    		exit(0);
			    	}
			    }

//			    feed_watchdog();
			}
			else
			{
				goto recv_error;
			}
		}
		else
		{
recv_error:
			if(retry_cnt++ > 1000)
			{
				serial_commu_recv_state = -1;
				tcflush(fd, TCIFLUSH);
				retry_cnt = 0;
			}
			else
			{
				usleep(10000);
			}
		}
	}

	pthread_exit(NULL);
}
