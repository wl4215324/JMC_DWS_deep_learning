/*
 * t7_camera_v4l2.h
 *
 *  Created on: Mar 23, 2019
 *      Author: tony
 */

#ifndef T7_CAMERA_V4L2_H_
#define T7_CAMERA_V4L2_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <getopt.h>        
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <fcntl.h>

#include <pthread.h>
#include "sunxi_camera.h"

#define LOG_OUT(fmt, arg...) printf(fmt, ##arg)
#define LOGD LOG_OUT
#define LOGW LOG_OUT
#define LOGE LOG_OUT

#define NB_BUFFER 8
#define V4L2_NV21
#define _SUNXIW17_


typedef int (*video_callback)(char *buf, int size);
typedef int (*disp_callback)(char *buf);

enum CAMERA_TYPE {
	CAMERA_TYPE_CSI,
	CAMERA_TYPE_UVC,
	CAMERA_TYPE_TVIN_NTSC,
	CAMERA_TYPE_TVIN_PAL,
	CAMERA_TYPE_TVIN_YPBPR,
	CAMERA_TYPE_VIR,
};

typedef enum {
	TVD_NTSC = 0,
	TVD_PAL = 1,
	TVD_SECAM = 2,
	TVD_YPBPR = 3,
} tvd_system_t;

enum TAKE_PICTURE_STATE {
	TAKE_PICTURE_NULL,		// do not take picture
	TAKE_PICTURE_NORMAL,	// stream off -> stream on -> take picture -> stream off -> stream on
	TAKE_PICTURE_FAST,		// normal mode but do not need to stream off/on
	TAKE_PICTURE_RECORD,	// take picture in recording
	TAKE_PICTURE_CONTINUOUS,
	TAKE_PICTURE_CONTINUOUS_FAST,
	TAKE_PICTURE_SMART,		// take smart picture
};

typedef struct v4l2_mem_map_t
{
	void *mem[NB_BUFFER];
	int length;
	int dmafd[NB_BUFFER];
} v4l2_mem_map_t;

struct VideoFrame
{
    char *vir_buf;
    int size;
    unsigned long long pts;
    unsigned int phy_addr;
};


extern pthread_mutex_t camera_buf_lock;
extern char YUV420_buf[1280*720*3/2];
extern unsigned int camera_buf_phy_addr;

extern video_callback callback;
//extern disp_callback disp_image;
//extern int openCameraDev(int camera_index);
//extern int startDevice(int width, int height, unsigned int pix_fmt);
//extern int startStream();
extern void* capture_video(void* para);

#endif /* T7_CAMERA_V4L2_H_ */
