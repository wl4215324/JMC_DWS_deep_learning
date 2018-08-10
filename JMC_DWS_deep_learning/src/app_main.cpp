#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <iostream>
#include <string>

#include "algorithm.hpp"

extern "C"
{
#include "v4l2_tvin.h"
#include "serial_port_commu.h"
#include "serial_pack_parse.h"
#include "gpio_operation.h"
#include "timers_driver.h"
#include "watchdog.h"
#include "single_daemon_running.h"
#include "xml_operation.h"
#include "kfifo.h"
#include "bootloader.h"
#include "disp_num_on_image.h"
}

using namespace std;

pthread_t serial_pthread_id, dws_pthread_id, vibrate_motor_pthread_id, sample_image_pthread_id;

static void param_validity_detect(KeyValuePair* key_value_list)
{
	unsigned short i;

	for(i=0; i<CONFIG_PARAMS_COUNT; i++)
	{
		//printf("parameter %d, name: %s, value: %d\n", i, (key_value_list+i)->key_name, (key_value_list+i)->value);

		if(!strcmp((key_value_list+i)->key_name, "vehicle_speed"))
		{
			/* ensure that vehicle speed no more than 250km/h */
			config_param.vehicle_speed = ((key_value_list+i)->value > 250) ? 250: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "freezing_time"))
		{
			/* ensure that freezing time no more than 60 minutes */
			config_param.freezing_time = ((key_value_list+i)->value > 60) ? 60 :(key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "led_power_level"))
		{
			/* ensure that led_power_level no more than 100% */
			config_param.led_power_level = (key_value_list+i)->value > 100 ? 100: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "motor_pwm_period"))
		{
			/* ensure that motor_pwm_period no more than 10 seconds */
			config_param.motor_pwm_period = ((key_value_list+i)->value > 10) ? 10: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "motor_pwm_duty"))
		{
			/* ensure that motor_pwm_duty no more than 100% */
			config_param.motor_pwm_duty = ((key_value_list+i)->value > 100) ? 100: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "fun_config"))
		{
			dws_alg_init_val.dws_warning_enable_config.byte_val = (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "level1_closing_eye_time"))
		{
			/* ensure that level one closing_eye time no more than 10 seconds */
			dws_alg_init_val.level1_closing_eye_time = ((key_value_list+i)->value > 10) ? 10: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "level2_closing_eye_time"))
		{
			/* ensure that level two closing_eye time no more than 10 seconds */
			dws_alg_init_val.level2_closing_eye_time = ((key_value_list+i)->value > 10) ? 10: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "yawn_time"))
		{
			/* ensure that yawn time no more than 10 seconds */
			dws_alg_init_val.yawn_time = ((key_value_list+i)->value > 10) ? 10: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "distract_time"))
		{
			/* ensure that distract time no more than 10 seconds */
			dws_alg_init_val.distract_time = ((key_value_list+i)->value > 10) ? 10: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "somking_time"))
		{
			/* ensure that smoking time no more than 10 seconds */
			dws_alg_init_val.somking_time = ((key_value_list+i)->value > 10) ? 10: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "calling_time"))
		{
			/* ensure that calling time no more than 10 seconds */
			dws_alg_init_val.calling_time = ((key_value_list+i)->value > 10) ? 10: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "covering_time"))
		{
			/* ensure that covering time no more than 20 seconds */
			dws_alg_init_val.covering_time = ((key_value_list+i)->value > 20) ? 20: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "driver_door_freeze_time"))
		{
			/* ensure that driver's door freezing time no more than 60 minutes */
			config_param.driver_door_time = ((key_value_list+i)->value > 60) ? 60: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "brake_freeze_time"))
		{
			/* ensure that brake freezing time no more than 60 seconds */
			config_param.brake_time = ((key_value_list+i)->value > 60) ? 60: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "turn_light_freeze_time"))
		{
			/* ensure that turning lights freezing time no more than 60 seconds */
			config_param.turn_light_time = ((key_value_list+i)->value > 60) ? 60: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "acceleator_freeze_time"))
		{
			/* ensure that accelerator freezing time no more than 60 seconds */
			config_param.acceleator_time = ((key_value_list+i)->value > 60) ? 60: (key_value_list+i)->value;
		}
		else if(!strcmp((key_value_list+i)->key_name, "ddws_switch"))
		{
			/* get ddws switch status from xml file, 0: ddws turn off, 1: ddws turn on */
			serial_input_var.DDWS_switch = ((key_value_list+i)->value > 1) ? 1: (key_value_list+i)->value;
		}
	}
}




static int create_task()
{
	/* create rs232 communication task */
    if (pthread_create(&serial_pthread_id, NULL, serial_commu_app, NULL) < 0)
	{
		perror("create serial_commu_app error!");
		return -1;
	}

    /* create dws algorithm detection task */
	//if (pthread_create(&dws_pthread_id, NULL, image_process_algorithm, NULL) < 0)
    if (pthread_create(&dws_pthread_id, NULL, algorithm_process, NULL) < 0)
	{
		perror("create image_process_algorithm error!");
		return -1;
	}

	/* create vibrating motor and led power control task */
	if(pthread_create(&vibrate_motor_pthread_id, NULL, vibrate_motor, NULL) < 0)
	{
		perror("create image_process_algorithm error!");
		return -1;
	}

	/* create image sampling task */
	if(pthread_create(&sample_image_pthread_id, NULL, sample_image_task, NULL) < 0)
	{
		perror("create sample_image_task error!");
		return -1;
	}

	return 0;
}



static int wait_task()
{
	pthread_join(serial_pthread_id, NULL);
	pthread_join(dws_pthread_id, NULL);
	pthread_join(vibrate_motor_pthread_id, NULL);
	pthread_join(sample_image_pthread_id, NULL);
	return 0;
}


static void process_cmd(int argc, char** argv)
{
	int i  = 0;

	for(i=1; i < argc; i++)
	{
		if(0 == strcmp(argv[i], "-level1_warn"))
		{
			serial_output_var_test.close_eye_one_level_warn = atoi(argv[i+1]);
		}

		if(0 == strcmp(argv[i], "-level2_warn"))
		{
			serial_output_var_test.close_eye_two_level_warn = atoi(argv[i+1]);
		}

		if(0 == strcmp(argv[i], "-yawn"))
		{
			serial_output_var_test.yawn_warn = atoi(argv[i+1]);
		}

		if(0 == strcmp(argv[i], "-distract"))
		{
			serial_output_var_test.distract_warn = atoi(argv[i+1]);
		}

		if(0 == strcmp(argv[i], "-call"))
		{
			serial_output_var_test.calling_warn = atoi(argv[i+1]);
		}

		if(0 == strcmp(argv[i], "-smoke"))
		{
			printf("serial_output_var_test.somking_warn: %2X \n", serial_output_var_test.warnning_level.somking_warn);
			serial_output_var_test.warnning_level.somking_warn = atoi(argv[i+1]);
		}

		if(0 == strcmp(argv[i], "-close_time"))
		{
			serial_output_var_test.close_eye_time = atoi(argv[i+1]);
		}

		if(0 == strcmp(argv[i], "-warn_state"))
		{
			serial_output_var_test.warnning_level.warning_state = atoi(argv[i+1]);
		}
	}
}



/*
 *  initialize hardware resources to be used
 */
static int hardware_init()
{
	unsigned char i = 0;
	char para_config_xml_path[32] = PARAM_CONFIG_XML_PATH;

	/* gpio initialization */
	if(set_led_brightness(10) < 0)
	{
		printf("set camera led brightness error!\n");
		return -1;
	}

	if(gpio_init(VIBRAT_MOTOR_GPIO_INDEX, 1) < 0)
	{
		printf("initial gpio%d error!\n", VIBRAT_MOTOR_GPIO_INDEX);
		return -1;
	}

	/* pull down gpio for vibrating motor, turn out vibrating motor */
	gpio_write(VIBRAT_MOTOR_GPIO_INDEX, 0);

    /* initialize rs232 serial port */
	if((fd = open_set_serial_port()) < 0)
	{
		printf("open rs232 error!\n");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

    /*
	if(open_watchdog() < 0)
	{
		return -1;
	}
	*/

	init_xml_file(para_config_xml_path, key_value_list);
	param_validity_detect(key_value_list);

	if(already_running() == 0)
	{
		DEBUG_INFO(program is to be executed!);
	}
	else
	{
		DEBUG_INFO(program has already been executed!);
		return -1;
	}

	/* create cache fifo for DDWS warning message */
	dws_warn_fifo = kfifo_alloc(DWS_WARNING_FIFO_SIZE);

	if(!dws_warn_fifo)
	{
		perror("create cache fifo for DDWS warning message error:");
		return -1;
	}

	/* initial bootloader business logic object */
	if(bootloader_logic_init(&JMC_bootloader_logic) < 0)
	{
		DEBUG_INFO(bootloader_logic_init failed! \n);
		return -1;
	}

	/* initialization for numerical display on monitor */
	Hz32Init();

	if(write_software_info() < 0)
	{
		DEBUG_INFO(write_software_info error!\n);
		return -1;
	}

	return 0;
}



int main()
{
	//daemon(0, 0);
	if(hardware_init() < 0)
	{
		return -1;
	}

	TimerInit();
	create_task();
	wait_task();
	TimerCleanup();

	return 0;
}
