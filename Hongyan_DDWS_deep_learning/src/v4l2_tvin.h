/*
 * v4l2_tvin.h
 *
 *  Created on: Mar 16, 2018
 *      Author: tony
 */

#ifndef V4L2_TVIN_H_
#define V4L2_TVIN_H_

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

#include "mxcfb.h"
#include "mxc_v4l2.h"
#include "ipu.h"
#include "serial_pack_parse.h"


#define TFAIL -1
#define TPASS 0

#define  IMAGE_WIDTH   720
#define  IMAGE_HEIGHT  480


struct testbuffer
{
	unsigned char *start;
	size_t offset;
	unsigned int length;
};

extern unsigned char YUYV_image[IMAGE_WIDTH*IMAGE_HEIGHT*2];
extern pthread_mutex_t uyvy_image_mutex;

extern unsigned char temp_drowsyLevel;

extern int mxc_v4l_tvin_test(void);

extern void* sample_image_task(void *);

#endif /* V4L2_TVIN_H_ */
