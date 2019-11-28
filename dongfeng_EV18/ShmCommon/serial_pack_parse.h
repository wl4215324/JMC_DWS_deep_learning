/*
 * serial_parse_pack_parse.h
 *
 *  Created on: Nov 21, 2017
 *      Author: tony
 */

#ifndef SERIAL_PARSE_PACK_PARSE_H_
#define SERIAL_PARSE_PACK_PARSE_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#include "applicfg.h"



#define MAKE_WORD(hig_byte, low_byte) (unsigned short) (((hig_byte&0x00FF)<<8) |(low_byte&0x00ff))
#define GET_HIG_BYTE_FROM_WORD(word)  (unsigned char) ((word&0xff00) >> 8)
#define GET_LOW_BYTE_FROM_WORD(word)  (unsigned char) (word&0xff)
#define GET_BIT_OF_BYTE(byte_addr, bit)  (unsigned char)(((*byte_addr)&(0x01<<bit))>>bit)
#define CONFIG_PARAMS_COUNT  18


static inline unsigned int get_bits_of_bytes(unsigned char* bytes, unsigned char start_bit_index, \
		unsigned char bits)
{
	unsigned int ret_val = 0;
	int i = 0;
	unsigned char start_byte_index, begin_bit_index;
	unsigned get_bit_val;

	start_byte_index = start_bit_index/8;
	begin_bit_index = start_bit_index%8;

	for(i=0; i<bits; i++)
	{
		get_bit_val = GET_BIT_OF_BYTE((bytes+start_byte_index+i/8), (begin_bit_index+i%8));
		ret_val |= get_bit_val<<i;
	}

	return ret_val;
}


#define MAKE_DWORD(hig_byte, mid_hig_byte, mid_low_byte, low_byte) \
	(unsigned int)((hig_byte<<24) | (mid_hig_byte<<16) | (mid_low_byte<<8) | low_byte)

/*
 *             application message format as below
 *
 * |0xAA|0x55|LEN_H|LEN_L|CLEN_H|CLEN_L|Type|byte 0|byte 1|***|byte n|CHE_H|CHE_L|
 * |-header--|                              |-------app message------|---check---|
 *           |------------------length (not include header)----------------------|
 * |--------------------check message length-------------------------|
 */


#define MESSAGE_HEAD_INDEX  0
#define MESSAGE_LEN_INDEX   2
#define MESSAGE_LEN_COMPL_INDEX  4
#define MESSAGE_TYPE_ID  6
#define MESSAGE_VAR_NUM  7

#define HEAD_AND_TAIL_LENGTH  9

/*
 *  message type macro
 */
#define MESSAGE_HEAD  0xAA55
#define D1_MESSAGE  0xD1
#define D2_MESSAGE  0xD2
#define D3_MESSAGE  0xD3
#define D4_MESSAGE  0xD4
#define D5_MESSAGE  0xD5
#define D6_MESSAGE  0xD6
#define D7_MESSAGE  0xD7

#define D2_APP_MESSAGE_LENTH  7
#define D2_MESSAGE_LENGTH  (HEAD_AND_TAIL_LENGTH+D2_APP_MESSAGE_LENTH)


/*
 * input variables message ID of CAN communication
 */
#define MESSAGE_ID_OF_VEHICLE_SPEED  0x0C0128D0
#define MESSAGE_ID_OF_VEHICLE_SPEED_INDEX 8

#define MESSAGE_ID_OF_VCU_GEAR  0xC02D4D0
#define MESSAGE_ID_OF_VCU_GEAR_INDEX  32

#define MESSAGE_ID_OF_TURN_LIGHT  0x18FF7021
#define MESSAGE_ID_OF_TURN_LIGHT_INDEX  44

#define MESSAGE_ID_OF_DRIVER_DOOR  0x1811D4F5
#define MESSAGE_ID_OF_DRIVER_DOOR_INDEX  56

#define MESSAGE_ID_OF_DFMS_SWITCH  0x18FFF8D9
#define MESSAGE_ID_OF_DFMS_SWITCH_INDEX  68


/*
 * configuration variables message ID of CAN communication
 */
#define MESSAGE_ID_OF_TRIGGERING_SPEED           7800
#define MESSAGE_ID_OF_LED_POWER_LEVEL            7801
#define MESSAGE_ID_OF_FREEZING_TIME              7802
#define MESSAGE_ID_OF_DWS_WARNING_ENABLE_CONFIG  7803
#define MESSAGE_ID_OF_MOTOR_PWM_PERIOD           7804
#define MESSAGE_ID_OF_MOTOR_PWM_DUTY             7805
#define MESSAGE_ID_OF_LEVEL1_CLOSING_EYE_TIME    7806
#define MESSAGE_ID_OF_LEVEL2_CLOSING_EYE_TIME    7807
#define MESSAGE_ID_OF_YAWN_TIME                  7808
#define MESSAGE_ID_OF_DISTRACT_TIME              7809
#define MESSAGE_ID_OF_SOMKING_TIME               7810
#define MESSAGE_ID_OF_CALLING_TIME               7811
#define MESSAGE_ID_OF_COVERING_TIME              7812
#define MESSAGE_ID_OF_DRIVERDOOR_TIME            7813
#define MESSAGE_ID_OF_BRAKE_TIME                 7814
#define MESSAGE_ID_OF_TURNLIGHT_TIME             7815
#define MESSAGE_ID_OF_ACCELERATOR_TIME           7816


/*
 * JMC soft switch trigger type
 */
#define TRIGGER_STYLE  0
#define RISING_EDGE    1
#define HIGH_LEVEL     2


/*
 * JMC warning levels definition
 */
#define  NO_WARNING  0
#define  LEVEL_ONE_WARNING    0x01
#define  LEVEL_TWO_WARNING    0x02
#define  LEVEL_THREE_WARNING  0x03
#define  COVER_WARNING        0x0D
#define  FAULT_WARNING        0x0E


/*
 * OK_Switch function enabled or disabled definition
 */
#define  OK_SWITCH_ENABLE   1
#define  OK_SWITCH_DISABLE  0



/*
 *  input data type definition for receiving serial port
 */
typedef struct {
	unsigned char vehicle_speed;
	unsigned char VCU_gear;
	unsigned char left_turn_light;
	unsigned char right_turn_light;
	unsigned char brake_switch;
	unsigned char driver_door;
	unsigned char DFMS_switch;
} SerialInputVar;


/*
 *  warning data type definition for sending serial port
 */
typedef struct {
	union
	{
		unsigned char DFMS_warn_bits;

		struct
		{
			unsigned char off_wheel:1;
			unsigned char yawn:1;
			unsigned char distraction:1;
			unsigned char phoning:1;
			unsigned char smoking:1;
			unsigned char reserve:1;
			unsigned char close_eye:1;
		} DFMS_warn;
	};

	unsigned char DFMS_work_state;
	unsigned char reserved_3;
	unsigned char reserved_4;
	unsigned char reserved_5;
	unsigned char reserved_6;
	unsigned char reserved_7;
	unsigned char reserved_8;
}SerialOutputVar;


/*
 * configuration parameters type definition for threshold value
 */
typedef struct {
	unsigned char vehicle_speed;
	unsigned char freezing_time;
	unsigned char led_power_level;
	unsigned char motor_pwm_period;
	unsigned char motor_pwm_duty;
	unsigned char driver_door_time;
	unsigned char brake_time;
	unsigned char turn_light_time;
	unsigned char acceleator_time;
} ConfigParam;


/*
 * configuration parameters type definition for enabling algorithm modules
 */
typedef struct {
	union {
		unsigned char byte_val;
		struct {
			unsigned char level1_closing_eye_enable:1;
			unsigned char level2_closing_eye_enable:1;
			unsigned char yawn_enable:1;
			unsigned char distract_enable:1;
			unsigned char smoking_enable:1;
			unsigned char calling_enable:1;
			unsigned char covering_enable:1;
		}bits;
	}dws_warning_enable_config;

	unsigned char level1_closing_eye_time;
	unsigned char level2_closing_eye_time;
	unsigned char yawn_time;
	unsigned char distract_time;
	unsigned char somking_time;
	unsigned char calling_time;
	unsigned char covering_time;
}DWSAlgInitVal;


typedef struct {
	ConfigParam threshold_value;
	DWSAlgInitVal dws_init_val;

} D6MessageDataType;


typedef struct{
	char key_name[32];
	unsigned char value;
}KeyValuePair;

extern SerialInputVar serial_input_var;
extern SerialOutputVar serial_output_var;
extern SerialOutputVar serial_output_var_test;
extern unsigned char serial_send_buf[512];
extern pthread_mutex_t serial_output_var_mutex;


extern ConfigParam config_param;
extern DWSAlgInitVal dws_alg_init_val;
extern D6MessageDataType D6_mesg_data;

extern int fd;
extern int serial_commu_recv_state;


//void* serial_commu_app(void* argv);
//int serial_commu_app( );
extern int read_one_frame(int fd, unsigned char* recv_buff, int* recv_frame_leng);

extern int pack_serial_send_message(unsigned char message_type, \
		void* send_data, unsigned char* send_buf, int* send_buf_len);
extern int send_spec_len_data(int fd, unsigned char* send_buf, unsigned short spec_send_data_len);

extern int parse_recv_pack_send(unsigned char* recv_buf, int recv_buf_len,\
		unsigned char* send_buf, int* send_buf_len);

extern unsigned short calc_check_sum(unsigned char* data_buf, int data_len);

#endif /* SERIAL_PARSE_PACK_PARSE_H_ */
