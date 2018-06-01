/*
 * gpio_operation.h
 *
 *  Created on: Nov 28, 2017
 *      Author: tony
 */

#ifndef GPIO_OPERATION_H_
#define GPIO_OPERATION_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "serial_pack_parse.h"
#include "timer_delay.h"

#define CAMERA_LED_GPIO_INDEX     9
#define VIBRAT_MOTOR_GPIO_INDEX   6

extern pthread_mutex_t motor_lock;
extern pthread_cond_t  motor_cond;

/*
* function:
*          gpio_export

* arguments:
*          int pin: gpio pin index

* return:
*        -1: error; 0: success
*/
static inline int gpio_export(int pin)
{
	char buffer[64];
	int len;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd < 0)
	{
		perror("Failed to open export for writing!");
		return -1;
	}

	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "/sys/class/gpio/gpio%d", pin);

	if(access(buffer, F_OK) < 0)
	{
		len = snprintf(buffer, sizeof(buffer), "%d", pin);

		if (write(fd, buffer, len) < 0)
		{
			printf("Failed to export gpio!\n");
			close(fd);
			return -1;
		}
	}

	close(fd);
	return 0;
}



/*
* function:
*          gpio_unexport

* arguments:
*          int pin: gpio pin index

* return:
*        -1: error; 0: success
*/
static inline int gpio_unexport(int pin)
{
	char buffer[64];
	int len;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (fd < 0)
	{
		perror("Failed to open unexport for writing!");
		return -1;
	}

	len = snprintf(buffer, sizeof(buffer), "%d", pin);
	if (write(fd, buffer, len) < 0)
	{
		close(fd);
		perror("Failed to unexport gpio!");
		return -1;
	}

	close(fd);
	return 0;
}



/*
* function:
*          gpio_direction

* arguments:
*          int pin: gpio pin index
            int dir: input or out direction, 0-->input, 1-->output

* return:
*        -1: error; 0: success
*/
static inline int gpio_direction(int pin, int dir)
{
	static const char dir_str[] = "in\0out";
	char path[64];
	int fd;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (fd < 0)
	{
		printf("Failed to open gpio direction for writing!\n");
		return -1;
	}

	if (write(fd, &dir_str[dir == 0 ? 0 : 3], dir == 0 ? 2 : 3) < 0)
	{
		printf("Failed to set direction!\n");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}



/*
* function:
*          gpio_write

* arguments:
*          int pin: gpio pin index
            int value: high or low level, 0-->LOW, 1-->HIGH

* return:
*        -1: error; 0: success

*/
static inline int gpio_write(int pin, int value)
{
	static const char values_str[] = "01";
	char path[64] = "";
	int fd;

	sprintf(path, "/sys/class/gpio/gpio%d/value", pin);

	fd = open(path, O_WRONLY);

	if (fd < 0)
	{
		printf("Failed to open gpio value for writing!: %s\n", path);
		return -1;
	}

	if (write(fd, &values_str[value == 0 ? 0 : 1], 1) < 0)
	{
		printf("Failed to write value!\n");
		close(fd);
		return -1;
	}
	else
	{
		close(fd);
		return 0;
	}
}



/*
* function:
*          gpio_read
*
* arguments:
*          int pin: gpio pin index

* return:
*        -1: error; 0: low; 1: high
*
*/
static inline int gpio_read(int pin)
{
	char path[64];
	char value_str[3];
	int fd;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		printf("Failed to open gpio value for reading!\n");
		return -1;
	}

	if (read(fd, value_str, 3) < 0)
	{
		printf("Failed to read value!\n");
		close(fd);
		return -1;
	}
	else
	{
		close(fd);
		return (atoi(value_str));
	}
}


/*
* function:
*          gpio_init
*
* arguments:
*          int pin: gpio pin index
*          int dir: 0-->input, 1-->output

* return:
*        -1: error; 0: success
*
*/
static inline int gpio_init(int pin, int dir)
{
	if(gpio_export(pin) < 0)
	{
		return -1;
	}

	if(gpio_direction(pin, dir) < 0)
	{
		return -1;
	}

	if(gpio_write(pin, 0) < 0)
	{
		return -1;
	}

	return 0;
}

/*
* function:
*          set_led_brightness
*
* arguments:
*          unsigned char brightness

* return:
*        -1: error; 0: success
*
*/
static inline int set_led_brightness(unsigned char brightness)
{
	int fd;
	int ret = 0;
	char brightness_buf[8] = "";
	char path_buf[64] = "";

	strcpy(path_buf, "/sys/class/backlight/pwm-backlight.0/brightness");

	fd = open(path_buf, O_WRONLY);
	if (fd < 0)
	{
		return -1;
	}
	else
	{
		memset(brightness_buf, '\0', sizeof(brightness_buf));
		sprintf(brightness_buf, "%d", brightness);

		if((ret = write(fd, brightness_buf, strlen(brightness_buf))) < 0)
		{
			close(fd);
			return -1;
		}
		else
		{
			close(fd);
			return ret;
		}
	}
}


void* vibrate_motor(void* argv);

#endif /* GPIO_OPERATION_H_ */
