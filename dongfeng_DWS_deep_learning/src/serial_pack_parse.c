/*
 * serial_pack_parse.c
 *
 *  Created on: Nov 21, 2017
 *      Author: tony
 */

#include "serial_pack_parse.h"
#include "user_timer.h"
#include "xml_operation.h"

SerialInputVar serial_input_var = {
		.vehicle_speed = 0,
		.turn_signal = 0,
		.accel_pedal = 0,
		.brake_switch = 0,
		.driver_door = 0,
		.engine_speed = 0,
		.small_lamp = 0,
	    .DDWS_switch = 0,
	    .OK_switch = 1,  //default enable
	    .MP5_DDWS_switch = 0,
	    .Cruise_switch = 0,
};


SerialOutputVar serial_output_var = {
		.warning_state = 0,
		.warning_sub_state = 0,
		.left_bytes = {0,}};


/*
 * configuration parameters
 */
ConfigParam config_param = {
		.vehicle_speed = 20,
		.freezing_time = 15,
		.led_power_level = 50,
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
		{"vehicle_speed", 20},
		{"freezing_time", 15},
		{"led_power_level", 20},
		{"motor_pwm_period", 2},
		{"motor_pwm_duty", 50},
		{"fun_config", 127},
		{"level1_closing_eye_time", 3},
		{"level2_closing_eye_time", 3},
		{"yawn_time", 2},
		{"distract_time", 3},
		{"somking_time", 1},
		{"calling_time", 4},
		{"covering_time", 10},
		{"driver_door_freeze_time", 15},
		{"brake_freeze_time", 20},
		{"turn_light_freeze_time", 20},
		{"acceleator_freeze_time", 20},
		{"ddws_switch", 1}
};

unsigned char vehicle_speed_judge_flag = 0;

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
				usleep(100000);
				continue;
			}
		}

		left_bytes -= read_bytes;
		recv_buf += read_bytes;
	}

	/*
	for(i=0; i < spec_len - left_bytes; i++)
	{
		printf("%2X", *(buf_begin+i));
	}
	printf("\n");
	*/

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
	int i = 0;
	int retry_cnt = 0;
	int retval = 0;
	unsigned char temp_buf[2];
	unsigned char message_type, var_cnt;
	unsigned short temp_var;
	unsigned short message_head, message_len, message_len_complement, check_sum;

	while(true)
	{
		if(i < 4)
		{
			retval = read_spec_len_data(fd, temp_buf, 2);
		}
		else
		{
			retval = read_spec_len_data(fd, recv_buff+8, (message_len-6));

			if(retval > 0)
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
				DEBUG_INFO(cal check_sum: %2X\n, check_sum);

				if(check_sum == (((*(recv_buff+ *recv_frame_leng -2) & 0xffff) << 8) |*(recv_buff+ *recv_frame_leng -1)))
				{
					return *recv_frame_leng;
				}
				else
				{
					return -1;
				}
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
			temp_var = MAKE_WORD(temp_buf[0], temp_buf[1]);

			if(MESSAGE_HEAD == temp_var)
			{
				message_head = MESSAGE_HEAD;
				*recv_frame_leng = 2;
				retry_cnt = 0;
				i = 1;
				continue;
			}

			if(1 == i)
			{
				message_len = temp_var;
				*recv_frame_leng = 4;
				i += 1;
			}
			else if(2 == i)
			{
				message_len_complement = temp_var;

				if(0xFFFF == (message_len + message_len_complement))
				{
					i += 1;
					*recv_frame_leng = 6;
				}
				else
				{
					i = 0;
					retry_cnt += 1;
					*recv_frame_leng = 0;
				}
			}
			else if(3 == i)
			{
				message_type = *temp_buf;
				var_cnt = *(temp_buf+1);

				if((message_type == D2_MESSAGE) || (message_type == D3_MESSAGE) || (message_type == D4_MESSAGE) ||\
						(message_type == D5_MESSAGE) || (message_type == D6_MESSAGE) || (message_type == D7_MESSAGE))
				{
					i += 1;
					*recv_frame_leng = 8;
				}
				else
				{
					i = 0;
					retry_cnt += 1;
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



static int is_all_max(unsigned char* temp_buf, unsigned char len)
{
	unsigned char i = 0;

	for(i=0; i < len; i++)
	{
		if(*(temp_buf+i) != 0xFF)
		{
			return -1;
		}
	}

	return 0;
}


/*
 * parse receiving frame and get input variables for DWS algorithm
 */
static int parse_serial_input_var(unsigned char* recv_buf, int recv_buf_len)
{

	unsigned char can_frame[8];
	char value_buf[8] = "";

	/*receiving message length error*/
	if((NULL == recv_buf) || (recv_buf_len < (8+12*7)))  //128 = 8+12*10
		return -1;

	/*get variable vehicle_speed from receiving data*/
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_VEHICLE_SPEED_INDEX), *(recv_buf+MESSAGE_ID_OF_VEHICLE_SPEED_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_VEHICLE_SPEED_INDEX+2), *(recv_buf+MESSAGE_ID_OF_VEHICLE_SPEED_INDEX+3)) == \
			MESSAGE_ID_OF_VEHICLE_SPEED)
	{
		serial_input_var.vehicle_speed = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_VEHICLE_SPEED_INDEX+4, 48, 16);
		DEBUG_INFO(vehicle_speed is: %4X\n, serial_input_var.vehicle_speed);
	}

	/*get variable turn_signal from receiving data*/
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_TURN_SIGNAL_INDEX), *(recv_buf+MESSAGE_ID_OF_TURN_SIGNAL_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_TURN_SIGNAL_INDEX+2), *(recv_buf+MESSAGE_ID_OF_TURN_SIGNAL_INDEX+3)) == \
			MESSAGE_ID_OF_TURN_SIGNAL)
	{
		serial_input_var.turn_signal = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_TURN_SIGNAL_INDEX+4, 8, 4);
		DEBUG_INFO(turn_signal is: %4X\n, serial_input_var.turn_signal);
	}

	/*get variable accel_pedal from receiving data*/
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_ACCEL_PEDAL_INDEX), *(recv_buf+MESSAGE_ID_OF_ACCEL_PEDAL_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_ACCEL_PEDAL_INDEX+2), *(recv_buf+MESSAGE_ID_OF_ACCEL_PEDAL_INDEX+3)) == \
			MESSAGE_ID_OF_ACCEL_PEDAL)
	{
		serial_input_var.accel_pedal = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_ACCEL_PEDAL_INDEX+4, 8, 8);
		DEBUG_INFO(accel_pedal is: %4X\n, serial_input_var.accel_pedal);
	}

	/*get variable brake_switch from receiving data*/
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX), *(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX+2), *(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX+3)) == \
			MESSAGE_ID_OF_BRAKE_SWITCH)
	{
		serial_input_var.brake_switch = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX+4, 28, 2);
		serial_input_var.Cruise_switch = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_BRAKE_SWITCH_INDEX+4, 53, 3);

		DEBUG_INFO(Cruise_switch is: %4X\n, serial_input_var.Cruise_switch);
		DEBUG_INFO(brake_switch is: %4X\n, serial_input_var.brake_switch);
	}

	/*get variable driver_door from receiving data*/
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_DRIVER_DOOR_INDEX), *(recv_buf+MESSAGE_ID_OF_DRIVER_DOOR_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_DRIVER_DOOR_INDEX+2), *(recv_buf+MESSAGE_ID_OF_DRIVER_DOOR_INDEX+3)) == \
			MESSAGE_ID_OF_DRIVER_DOOR)
	{
		serial_input_var.driver_door = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_DRIVER_DOOR_INDEX+4, 32, 2);
		DEBUG_INFO(driver_door is: %4X\n, serial_input_var.driver_door);
	}

	/*get variable small_lamp from receiving data*/
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_SMALL_LAMP_INDEX), *(recv_buf+MESSAGE_ID_OF_SMALL_LAMP_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_SMALL_LAMP_INDEX+2), *(recv_buf+MESSAGE_ID_OF_SMALL_LAMP_INDEX+3)) == \
			MESSAGE_ID_OF_SMALL_LAMP)
	{
		serial_input_var.small_lamp = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_SMALL_LAMP_INDEX+4, 30, 2);
		DEBUG_INFO(small_lamp is: %4X\n, serial_input_var.small_lamp);
	}


	/*get variable engine_speed from receiving data*/
	if(MAKE_DWORD(*(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX), *(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX+1),\
			*(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX+2), *(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX+3)) == \
			MESSAGE_ID_OF_ENGINE_SPEED)
	{
		serial_input_var.engine_speed = get_bits_of_bytes(recv_buf+MESSAGE_ID_OF_ENGINE_SPEED_INDEX+4, 24, 16);
		DEBUG_INFO(engine_speed is: %4X\n, serial_input_var.engine_speed);
	}

	return 1;
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
#if 0
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
		*(send_buf+*send_buf_len) = *(unsigned char*)(send_data+7);
		*send_buf_len += 1;
#endif

		message_len = 16;
		message_len_compl = ~(16);
		message_type_id = D2_MESSAGE;
		memcpy(send_buf+7, (unsigned char*)send_data, 8);
		*send_buf_len = 7 + 8;
		*(send_buf+*send_buf_len) = *(unsigned char*)(send_data+7);
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
		*(send_buf+24) = (unsigned char)(((ARM_APP_SOFTWARE_VER_MAJ&0xff)<<4)|ARM_APP_SOFTWARE_VER_MIN);
		*send_buf_len = 7 + 18;
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

	return pack_serial_send_message(D2_MESSAGE, (void*)&serial_output_var, send_buf, send_buf_len);

}



/*
 * type D3 message processing, mainly receiving D3 message and replying the values of configuration parameters
 */
static int D3_message_process(unsigned char* recv_buf, int recv_buf_len,\
		unsigned char* send_buf, int* send_buf_len)
{
	if((D3_MESSAGE != *(recv_buf+MESSAGE_TYPE_ID)) || \
			( (recv_buf_len-2) != MAKE_WORD(*(recv_buf+MESSAGE_LEN_INDEX), *(recv_buf+MESSAGE_LEN_INDEX+1)) ))
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
			recv_buf_len, (send_buf+7), send_buf_len) < 0)
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
	DEBUG_INFO(D6 message deal normal \n);

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
		DEBUG_INFO(enter D6 message deal);
		//DEBUG_INFO(send_buf_len: %d\n, send_buf_len);
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

	return (spec_send_data_len- nleft);
}




void serial_input_var_judge_2(SerialInputVar serial_input_var_temp)
{
	/* if vehicle speed is less than 20km/h */
	if((serial_input_var_temp.vehicle_speed>>8) < 20)
	{
		if( (1 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_flag) && \
			(0 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_timer_stat) )
		{
			vehicle_speed_timer_flag.bits.speed_less_than_threshold_timer_stat = 1;
			SetAlarm(&vehicle_speed_timer_flag, speed_less_than_threshold_after_3s, &timeout_execute_activity, \
					S_TO_TIMEVAL(3), 0);
			free_spec_type_alarm(speed_more_than_threshold_after_3s);
		}
		else if((1 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_flag) && \
				(1 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_timer_stat))
		{
			;
		}
		else if((0 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_flag) && \
				(0 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_timer_stat))
		{
			vehicle_speed_timer_flag.bits.speed_more_than_threshold_flag = 1;
			vehicle_speed_timer_flag.bits.speed_more_than_threshold_timer_stat = 0;
			vehicle_speed_judge_flag = 0;
		}
	}
	else  // if vehicle speed is more than 20km/h
	{
		/* if vehicle speed >= 20km/h for 3S */
		if( (1 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_flag) && \
			(0 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_timer_stat) )
		{
			vehicle_speed_timer_flag.bits.speed_more_than_threshold_timer_stat = 1;
			SetAlarm(&vehicle_speed_timer_flag, speed_more_than_threshold_after_3s, &timeout_execute_activity, \
					S_TO_TIMEVAL(3), 0);
			free_spec_type_alarm(speed_less_than_threshold_after_3s);
		}
		else if((1 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_flag) && \
				(1 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_timer_stat))
		{
			;
		}
		else if((0 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_flag) && \
				(0 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_timer_stat))
		{
			vehicle_speed_timer_flag.bits.speed_less_than_threshold_flag = 1;
			vehicle_speed_timer_flag.bits.speed_less_than_threshold_timer_stat = 0;
			vehicle_speed_judge_flag = 1;
		}
	}

#if 0
	static unsigned char exec_once_flag = 0;

	/* if vehicle speed more than 0 */
	if((serial_input_var_temp.vehicle_speed>>8) > 0)
	{
		/* if 15mins timer for vehicle speed more than zero is not setup */
		if( (timer_flag.bits.engine_start_timer_stat == 0) && \
			(timer_flag.bits.engine_start_afer_15min_flag == 1) && \
			(0 == exec_once_flag) )
		{
			timer_flag.bits.engine_start_timer_stat = 1;
			SetAlarm(&timer_flag, engine_start_after_15min, &timeout_execute_activity, \
					MIN_TO_TIMEVAL(config_param.freezing_time), 0);
					//MIN_TO_TIMEVAL(15), 0);
			printf("engine start, config_param.freezing_time: %d\n", config_param.freezing_time);
		}
		/* if 15mins timer for vehicle speed more than zero is running */
		else if((timer_flag.bits.engine_start_timer_stat == 1) && \
				(timer_flag.bits.engine_start_afer_15min_flag == 1) && \
				(0 == exec_once_flag))
		{
			; //do nothing
		}
		/* if if 15mins timer for vehicle speed more than zero time out */
		else if((timer_flag.bits.engine_start_timer_stat == 0) && \
				(timer_flag.bits.engine_start_afer_15min_flag == 0) && \
				(0 == exec_once_flag))
		{
			exec_once_flag = 1;
		}
		else if((timer_flag.bits.engine_start_timer_stat == 0) && \
				(timer_flag.bits.engine_start_afer_15min_flag == 0) && \
				(1 == exec_once_flag))
		{
			/* if vehicle speed is less than 20km/h */
			if((serial_input_var_temp.vehicle_speed>>8) < 20)
			{
				if( (1 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_flag) && \
					(0 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_timer_stat) )
				{
					timer_flag.bits.speed_less_than_threshold_timer_stat = 1;
					SetAlarm(&timer_flag, speed_less_than_threshold_after_3s, &timeout_execute_activity, \
							S_TO_TIMEVAL(3), 0);
					free_spec_type_alarm(speed_more_than_threshold_after_3s);
				}
				else if((1 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_flag) && \
						(1 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_timer_stat))
				{
					;
				}
				else if((0 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_flag) && \
						(0 == vehicle_speed_timer_flag.bits.speed_less_than_threshold_timer_stat))
				{
					timer_flag.bits.speed_more_than_threshold_flag = 1;
					timer_flag.bits.speed_more_than_threshold_timer_stat = 0;
				}
			}
			else  // if vehicle speed is more than 20km/h
			{
				/* if vehicle speed >= 20km/h for 3S */
				if( (1 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_flag) && \
					(0 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_timer_stat) )
				{
					timer_flag.bits.speed_more_than_threshold_timer_stat = 1;
					SetAlarm(&timer_flag, speed_more_than_threshold_after_3s, &timeout_execute_activity, \
							S_TO_TIMEVAL(3), 0);
					free_spec_type_alarm(speed_less_than_threshold_after_3s);
				}
				else if((1 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_flag) && \
						(1 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_timer_stat))
				{
					;
				}
				else if((0 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_flag) && \
						(0 == vehicle_speed_timer_flag.bits.speed_more_than_threshold_timer_stat))
				{
					timer_flag.bits.speed_less_than_threshold_flag = 1;
					timer_flag.bits.speed_less_than_threshold_timer_stat = 0;
				}
			}
		}
	}
	else
	{   if(0 == exec_once_flag)
	    {
			/*if vehicle stop*/
			timer_flag.timer_val = 0x1f;
			free_all_alarm();
			DEBUG_INFO(engine stop!);
	    }
	}

	/*if driver door is closed*/
	if(serial_input_var_temp.driver_door == 0)
	{
		/*launch 15mins-timer for closing driver side door*/
		if((timer_flag.bits.driver_door_close_timer_stat == 0) && (timer_flag.bits.driver_door_close_after_15min_flag == 1))
		{
			timer_flag.bits.driver_door_close_timer_stat = 1;
			SetAlarm(&timer_flag, driver_door_close_after_15min, &timeout_execute_activity, \
					MIN_TO_TIMEVAL(config_param.driver_door_time), 0);
					//MIN_TO_TIMEVAL(15), 0);
			printf("driver door close, config_param.driver_door_time: %d\n", config_param.driver_door_time);
		}
	}
	else
	{
		/*if driver door opened now */
		timer_flag.timer_val = 0x1f;
		free_all_alarm();
		printf("driver door open!\n");
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

	/* if accelerator pedal is kicked down or cruise mode is active */
	//if((1==serial_input_var_temp.accel_pedal) || (1 == serial_input_var_temp.Cruise_switch))
	if(((serial_input_var_temp.accel_pedal > 0) && (serial_input_var_temp.accel_pedal <= 0xFA)) || \
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
	else if(((0 == serial_input_var_temp.accel_pedal) && (0 == serial_input_var_temp.Cruise_switch)) || \
			((0xFA < serial_input_var_temp.accel_pedal) && (0 == serial_input_var_temp.Cruise_switch)) || \
			((0 == serial_input_var_temp.accel_pedal) && (1 < serial_input_var_temp.Cruise_switch)) || \
			((0xFA < serial_input_var_temp.accel_pedal) && (1 < serial_input_var_temp.Cruise_switch)))
	{
		/*if pedal is not kicked down*/
		timer_flag.bits.accelerator_active_timer_stat = 0;
		timer_flag.bits.accelerator_active_after_20s_flag = 1;
		free_spec_type_alarm(accelerator_active_after_20s);
	}
#endif

}


#if 0
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
#endif


/* rs232 communication task */
void* serial_commu_app(void* argv)
{
	int i  = 0, retry_cnt = 0;
	fd_set rfds;
	int recv_length, spec_recv_len;
	int send_buf_len;
	unsigned char dws_mesg_array[8] = {0};
	struct timeval tv = {
			.tv_sec = 3,
			.tv_usec = 300000,
	};

	while(true)
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		if(select(fd+1, &rfds, NULL, NULL, &tv) > 0)
		{
			/*read serial data from rs232 */
			if((recv_length = read_one_frame(fd, serial_recv_buf, &spec_recv_len)) > 0)
			{
				retry_cnt = 0;
				serial_commu_recv_state = 0;

				printf("recv_length is: %d data:", recv_length);

				for(i=0; i<recv_length; i++ )
				{
					printf("%02X", serial_recv_buf[i]);
				}

			    puts("\n");

			    /* parse receiving serial data */
			    if(parse_recv_pack_send(serial_recv_buf, spec_recv_len, serial_send_buf, &send_buf_len) > 0)
			    {
			    	DEBUG_INFO(send_buf_len: %d\n, send_buf_len);

			    	if(send_buf_len > 0)
				    {
						if(send_spec_len_data(fd, serial_send_buf, send_buf_len) >= send_buf_len)
						{
							printf("send_length is: %d, data: ", send_buf_len);

							for(i=0; i<send_buf_len; i++)
							{
								printf("%02X ", serial_send_buf[i]);
							}

							puts("\n");
						}
						else
						{
							printf("send data failed!\n");
						}
				    }

				    serial_input_var_judge_2(serial_input_var);
			    }

			    feed_watchdog();
			}
			else
			{
				goto recv_error;
			}
		}
		else
		{
recv_error:
			if(retry_cnt++ > 10)
			{
				serial_commu_recv_state = -1;
				tcflush(fd, TCIOFLUSH);
				retry_cnt = 11;
			}
			else
			{
				tcflush(fd, TCIOFLUSH);
				usleep(500000);
			}
		}
	}

	pthread_exit(NULL);
}
