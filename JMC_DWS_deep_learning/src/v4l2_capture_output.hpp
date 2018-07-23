/*
 * v4l2_capture_output.h
 *
 *  Created on: Nov 13, 2017
 *      Author: tony
 */

#ifndef V4L2_CAPTURE_OUTPUT_H_
#define V4L2_CAPTURE_OUTPUT_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>


extern "C"{
#include "mxcfb.h"
#include "mxc_v4l2.h"
#include "ipu.h"
#include "serial_pack_parse.h"
#include "driving_behav_analys.h"
#include "gpio_operation.h"
}

//#define HJWS_HARDWARE_I  1


#define TFAIL -1
#define TPASS 0


struct testbuffer
{
	unsigned char *start;
	size_t offset;
	unsigned int length;
};

extern void* image_process_algorithm(void*);

#endif /* V4L2_CAPTURE_OUTPUT_H_ */
