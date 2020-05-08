/*
 * run_algorithm.h
 *
 *  Created on: Oct 12, 2019
 *      Author: tony
 */

#ifndef RUN_ALGORITHM_H_
#define RUN_ALGORITHM_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

extern "C" {
#include "../../../../ShmCommon/rtc_operations.h"
}

extern void copy_dfms_image(unsigned char* src_image);

extern void copy_monitor_image(unsigned char* src_image);

extern void* run_DFMS_algorithm(void *argc);

extern void* run_off_wheel_algorithm(void* argc);

int init_algorithm();
int run_algorithm();

#endif /* RUN_ALGORITHM_H_ */
