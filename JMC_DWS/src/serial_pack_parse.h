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
#include "serial_port_commu.h"
#include "driving_behav_analys.h"
#include "gpio_operation.h"
#include "software_version.h"
#include "watchdog.h"
#include "kfifo.h"


#define MAKE_WORD(hig_byte, low_byte) (unsigned short) (((hig_byte&0x00FF)<<8) |(low_byte&0x00ff))
#define GET_HIG_BYTE_FROM_WORD(word)  (unsigned char) ((word&0xff00) >> 8)
#define GET_LOW_BYTE_FROM_WORD(word)  (unsigned char) (word&0xff)
#define GET_BIT_OF_BYTE(byte_addr, bit)  (unsigned char)(((*byte_addr)&(0x01<<bit))>>bit)
#define CONFIG_PARAMS_COUNT  17

static inline unsigned int get_bits_of_bytes(unsigned char* bytes, unsigned char start_bit_index,  unsigned char bits)
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


#define MESSAGE_HEAD_INDEX  0
#define MESSAGE_LEN_INDEX   2
#define MESSAGE_LEN_COMPL_INDEX  4
#define MESSAGE_TYPE_ID  6
#define MESSAGE_VAR_NUM  7

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


/*
 * input variables message ID macro of CAN communication
 */
#define MESSAGE_ID_OF_VEHICLE_SPEED  0x0CFE6CEE
#define MESSAGE_ID_OF_VEHICLE_SPEED_INDEX 8

#define MESSAGE_ID_OF_TURN_SIGNAL  0x0CFDCC21
#define MESSAGE_ID_OF_TURN_SIGNAL_INDEX  20

#define MESSAGE_ID_OF_ACCEL_PEDAL  0x0CF00300
#define MESSAGE_ID_OF_ACCEL_PEDAL_INDEX  32

#define MESSAGE_ID_OF_BRAKE_SWITCH  0x18FEF100
#define MESSAGE_ID_OF_BRAKE_SWITCH_INDEX  44

#define MESSAGE_ID_OF_DRIVER_DOOR  0x18FED921
#define MESSAGE_ID_OF_DRIVER_DOOR_INDEX  56

#define MESSAGE_ID_OF_SMALL_LAMP  0x18FE4021
#define MESSAGE_ID_OF_SMALL_LAMP_INDEX  68

#define MESSAGE_ID_OF_DDWS_SWITCH  0x18FF0217
#define MESSAGE_ID_OF_DDWS_SWITCH_INDEX  80

#define MESSAGE_ID_OF_ENGINE_SPEED  0x0CF00400
#define MESSAGE_ID_OF_ENGINE_SPEED_INDEX  92

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
 *  input data type definition for receiving serial port
 */
typedef struct {
	unsigned short vehicle_speed;
	unsigned short turn_signal;
	unsigned short accel_pedal;
	unsigned short brake_switch;
	unsigned short driver_door;
	unsigned short engine_speed;
	unsigned short small_lamp;
    unsigned short DDWS_switch;
    unsigned short OK_switch;
} SerialInputVar;


/*
 *  warning data type definition for sending serial port
 */
typedef struct {
	unsigned char close_eye_one_level_warn;
	unsigned char close_eye_two_level_warn;
	unsigned char yawn_warn;
	unsigned char distract_warn;
	unsigned char calling_warn;
	unsigned char somking_warn;
	unsigned char warning_state;
	unsigned char close_eye_time;

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
extern unsigned char serial_send_buf[256];


extern ConfigParam config_param;
extern DWSAlgInitVal dws_alg_init_val;
extern D6MessageDataType D6_mesg_data;

extern int fd;
extern int serial_commu_recv_state;
extern KeyValuePair key_value_list[CONFIG_PARAMS_COUNT];

extern void* serial_commu_app(void* argv);
extern int pack_serial_send_message(unsigned char message_type, \
		void* send_data, unsigned char* send_buf, int* send_buf_len);
extern int send_spec_len_data(int fd, unsigned char* send_buf, unsigned short spec_send_data_len);

#endif /* SERIAL_PARSE_PACK_PARSE_H_ */
