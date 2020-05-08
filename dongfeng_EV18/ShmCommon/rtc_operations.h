/*
 * time_process.h
 *
 *  Created on: Nov 15, 2019
 *      Author: tony
 */

#ifndef TIME_PROCESS_H_
#define TIME_PROCESS_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>


int set_datetime_according_ms(unsigned long long ms_after_1970);
int set_datetime_according_str(const char *date_time_str);

int get_datetime_according_fmt(char *str_date_time);

time_t getDateTime(struct tm **local_time);

#endif /* TIME_PROCESS_H_ */
