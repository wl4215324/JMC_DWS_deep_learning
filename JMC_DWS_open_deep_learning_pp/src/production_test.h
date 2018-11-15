/*
 * production_test.h
 *
 *  Created on: Oct 23, 2018
 *      Author: tony
 */

#ifndef PRODUCTION_TEST_H_
#define PRODUCTION_TEST_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


extern unsigned char rs485_test_flag;

extern int read_test_picture(unsigned char *read_buffer, int *buffer_length);
extern int get_production_test_mode();

#endif /* PRODUCTION_TEST_H_ */
