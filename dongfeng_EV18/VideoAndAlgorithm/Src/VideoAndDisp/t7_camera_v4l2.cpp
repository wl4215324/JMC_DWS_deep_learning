/*
 * t7_camera_v4l2.c
 *
 *  Created on: Mar 21, 2019
 *      Author: tony
 */

#include "t7_camera_v4l2.h"
#include "gl_display.h"
#include "video_layer_test.h"
#include "t7_enc_test.h"
#include "sunxiMemInterface.h"

extern "C" {
#include <stdbool.h>
#include "serial_pack_parse.h"
#include "../VideoStore/files_manager.h"
#include "../VideoStore/video_encode.h"
#include "../VideoStore/warn_video_store.h"
#include "G2dApi.h"
#include "water_mark_interface.h"
}


#include "../Algorithm/Src/run_algorithm.hpp"

unsigned char picture_is_ready = 0;
pthread_mutex_t picture_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t picture_cond =  PTHREAD_COND_INITIALIZER;

extern WaterMarkUsrInterface *t7_watermark;
extern SingleWaterMark watermark_array[MAX_WATERMARK_NUM];
extern SingleWaterMark monitor_watermark_array[MAX_WATERMARK_NUM];

extern Video_File_Resource *dsm_video_record;
extern Video_File_Resource *monitor_video_record;

static int nPlanes = 0;
static int mCameraFd = -1;
static int mCameraType = 0;
static int mCaptureFormat;  // the driver capture format
static int mFrameWidth = 0, mFrameHeight = 0;
static int mFrameRate = 30;
v4l2_mem_map_t mMapMem;
v4l2_mem_map_t monitor_camera_MapMem;

// actually buffer counts
static int mBufferCnt;

video_callback callback;


static int waitFrame(int fd)
{
	fd_set fds;
	struct timeval tv;
	int ret;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	 /* Timeout */
	tv.tv_sec  = 2;
	tv.tv_usec = 0;
	ret = select(fd + 1, &fds, NULL, NULL, &tv);

	if (ret == -1)
	{
		printf("select err, %s\n", strerror(errno));
		return -1;
	}
	else if (ret == 0)
	{
		printf("select timeout");
		return 1;
	}

	return 0;
}

int get_video()
{
    int ret = -1;

    while(1)
    {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(mCameraFd, &fds);
        /* Timeout */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        //hm2131->dump_reg();
        r = select(mCameraFd + 1, &fds, NULL, NULL, &tv);

        if (r == -1)
        {
            printf("select err, %s\n", strerror(errno));
            exit(-1);
        }
        else if (r == 0)
        {
            printf("select timeout %s\n", "video 0");
            continue;
        }


        struct v4l2_buffer buf2 = {0};
        struct v4l2_buffer *buf = &buf2;

        buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (mCameraType == CAMERA_TYPE_CSI)
        {
            struct v4l2_plane planes[VIDEO_MAX_PLANES];
            buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf->memory = V4L2_MEMORY_MMAP;
            buf->length = nPlanes;
            buf->m.planes = planes;
        }

        buf->memory = V4L2_MEMORY_MMAP;

        ret = ioctl(mCameraFd, VIDIOC_DQBUF, buf);

        if (ret < 0)
        {
            printf("GetPreviewFrame: VIDIOC_DQBUF Failed, %s\n", strerror(errno));
            //ln add
            printf("GetPreviewFrame, VIDIOC_DQBUF Failed\n");
            return __LINE__;	// can not return false
        }

        ret = ioctl(mCameraFd, VIDIOC_QBUF, buf);

        if (ret < 0)
        {
            printf("VIDIOC_QBUF Failed\n");
            return ret;
        }
    }

    return 0;
}

int tryFmt(int fd, int format);

int getTVINSystemType(int fd, int cvbs_type)
{
	struct v4l2_format format;
	struct v4l2_frmsizeenum frmsize;
	enum v4l2_buf_type type;
	int i = 0;
	int temp_height = 0;

	memset(&format, 0, sizeof(struct v4l2_format));
	memset(&frmsize, 0, sizeof(struct v4l2_frmsizeenum));
	frmsize.pixel_format = V4L2_PIX_FMT_NV21;
	frmsize.index = 0;
#if 1
	while ((!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) && (i < 20))
	{
		LOGW("i = %d\n", i);
		LOGW("framesize width = %d, height = %d\n", frmsize.discrete.width,
			 frmsize.discrete.height);
		i++;
		frmsize.index++;
	}

	memset(&format, 0, sizeof(struct v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; //V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;

	switch(cvbs_type)
	{
#if 0
	case 0:
		format.fmt.pix.width = 720;
		format.fmt.pix.height = 480;
		if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
			LOGW("set tvin image format failed 0\n");
			return -1;
		}
		return TVD_NTSC;

	case 1:
		format.fmt.pix.width = 720;
		format.fmt.pix.height = 576;
		if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
			LOGW("set tvin image format failed 1\n");
			return -1;
		}
		return TVD_PAL;
#else
	case 0:
	case 1:
		i = 0;
		while (ioctl(fd, VIDIOC_G_FMT, &format) &&(i++ < 20))  //this will use 200ms at lease
		{
			printf("get tvin signal failed.\n");
			return -1;
		}

		if (ioctl(fd, VIDIOC_S_FMT, &format) == -1)
		{
			LOGW("set tvin image format failed 1\n");
			return -1;
		}

		if(format.fmt.pix.height == 480)
		{
			return TVD_NTSC;
		}
		else if(format.fmt.pix.height == 576)
		{
			return TVD_PAL;
		}
		else
		{
			LOGE("not NTSC and PAL get tvin signal failed.\n");
			return -1;
		}
	#endif
	case 2:
		format.fmt.pix.width = 1440;
		format.fmt.pix.height = 960;
		return TVD_NTSC;

	case 3:
		format.fmt.pix.width = 1440;
		format.fmt.pix.height = 1152;
		return TVD_PAL;

	default:
		format.fmt.pix.width = 720;
		format.fmt.pix.height = 576;

		if (ioctl(fd, VIDIOC_S_FMT, &format) == -1)
		{
			LOGE("set tvin image format failed 4\n");
			return -1;
		}
		return TVD_PAL;
	}

#else
	while ((!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) && (i < 20)) {
		LOGW("i = %d\n", i);
		LOGW("framesize width = %d, height = %d\n", frmsize.discrete.width,
			 frmsize.discrete.height);
		i++;
		frmsize.index++;
	}

	memset(&format, 0, sizeof(struct v4l2_format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

#ifdef DEFAULT_CVBS_CAMERA_TYPE_PAL
	i = 0;
	while (ioctl(fd, VIDIOC_G_FMT, &format) && (i++ < 20))	//this will use 200ms at lease
	{
		printf("get tvin signal failed.\n");
		return -1;
	}
	//if you dont want to waste 200ms,try to set pix width and height directory(haven't test)
	//format.fmt.pix.width = 720;
	//format.fmt.pix.height = 576;
#else
	format.fmt.pix.width = 720;
	format.fmt.pix.height = 576;
#endif

	LOGW("width= %d, height =[%d]\n", format.fmt.pix.width, format.fmt.pix.height);

	format.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;
	if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
		LOGW("set tvin image format failed\n");
		return -1;
	}

	LOGW("debug width= %d, height =[%d]\n", format.fmt.pix.width, format.fmt.pix.height);

	if ((720 == format.fmt.pix.width) && (480 == format.fmt.pix.height))
		return TVD_NTSC;

	if ((720 == format.fmt.pix.width) && (576 == format.fmt.pix.height))
		return TVD_PAL;
	return TVD_YPBPR;
#endif
}

// -----------------------------------------------------------------------------
// extended interfaces here <***** star *****>
// -----------------------------------------------------------------------------
int openCameraDev(int camera_index)
{
	int ret = -1;
	struct v4l2_input inp;
	struct v4l2_capability cap;
	char dev_node[16];
	int cvbs_type = 0;
	int camera_fd = -1;

	sprintf(dev_node, "/dev/video%d", camera_index);
	ret = access(dev_node, F_OK);

	if (ret == 0)
	{
		printf("camera=%s ok!!!\n", dev_node);
	}
	else
	{
		printf("camera=%s error!!!\n", dev_node);
		return -1;
	}

	camera_fd = open(dev_node, O_RDWR | O_NONBLOCK, 0);

	if(camera_fd == -1)
	{
		LOGE("ERROR opening %s: %s", dev_node, strerror(errno));
		return -1;
	}

	//-------------------------------------------------
	// check v4l2 device capabilities
	ret = ioctl(camera_fd, VIDIOC_QUERYCAP, &cap);

	if (ret < 0)
	{
		LOGE("Error opening device: unable to query device.");
		goto END_ERROR;
	}

	if (!strcmp((char *) cap.driver, "uvcvideo"))
	{
		mCameraType = CAMERA_TYPE_UVC;
		LOGD("mCameraType = CAMERA_TYPE_UVC");
	}
	else if (!strcmp((char *) cap.driver, "sunxi-tvd"))
	{
		int tvinType;
		tvinType = getTVINSystemType(camera_fd, cvbs_type);

		if (tvinType == TVD_PAL)
		{
			mCameraType = CAMERA_TYPE_TVIN_PAL;
			if(camera_index == 8)  // /dev/video8 WxH is 1440x1152
			{
				mFrameWidth = 1440;
				mFrameHeight = 1152;
			}
			else
			{
				mFrameWidth = 720;
				mFrameHeight = 576;
			}

			LOGD("mCameraType = CAMERA_TYPE_TVIN_PAL");
		}
		else if (tvinType == TVD_NTSC)
		{
			mCameraType = CAMERA_TYPE_TVIN_NTSC;

			if(camera_index == 8)   // /dev/video8 WxH is 1440x960
			{
				mFrameWidth = 1440;
				mFrameHeight = 960;
			}
			else
			{
				mFrameWidth = 720;
				mFrameHeight = 480;
			}

			LOGD("mCameraType = CAMERA_TYPE_TVIN_NTSC");
		}
		else if (tvinType == TVD_YPBPR)
		{
			mCameraType = CAMERA_TYPE_TVIN_YPBPR;
			LOGD("mCameraType = CAMERA_TYPE_TVIN_YPBPR");
		}
		else
		{
#ifdef DEFAULT_CVBS_CAMERA_TYPE_PAL
			mCameraType = CAMERA_TYPE_TVIN_PAL;
			LOGD("mCameraType = CAMERA_TYPE_TVIN_PAL");
#else
			mCameraType = CAMERA_TYPE_TVIN_NTSC;
			LOGD("Default mCameraType = CAMERA_TYPE_TVIN_NTSC;");
#endif
		}
	}
	else
	{
		printf("TVIN std is: %d\n", getTVINSystemType(camera_fd, cvbs_type));
		mCameraType = CAMERA_TYPE_CSI;
		LOGD("mCameraType = CAMERA_TYPE_CSI \n");
	}

	LOGD("The name of the Camera is '%s' \n", cap.card);

	if (mCameraType == CAMERA_TYPE_CSI)
	{
		// uvc do not need to set input
		inp.index = 0;
		if (-1 == ioctl(camera_fd, VIDIOC_S_INPUT, &inp))
		{
			LOGE("VIDIOC_S_INPUT error id=%d!", camera_index);
			goto END_ERROR;
		}
	}

	if (mCameraType == CAMERA_TYPE_UVC)
	{
		// try to support this format: NV21, YUYV
		// we do not support mjpeg camera now

		if (tryFmt(camera_fd, V4L2_PIX_FMT_NV21) == 0)
		{
			mCaptureFormat = V4L2_PIX_FMT_NV21;
			LOGE("capture format: V4L2_PIX_FMT_NV21");
		}

		else if (tryFmt(camera_fd, V4L2_PIX_FMT_H264) == 0)
		{
			mCaptureFormat = V4L2_PIX_FMT_H264;
			LOGW("capture format: V4L2_PIX_FMT_H264");
		}
		else if (tryFmt(camera_fd, V4L2_PIX_FMT_YUYV) == 0)
		{
			mCaptureFormat = V4L2_PIX_FMT_YUYV;	// maybe usb camera
			LOGW("capture format: V4L2_PIX_FMT_YUYV");
		}
		else if (tryFmt(camera_fd, V4L2_PIX_FMT_MJPEG) == 0)
		{
			mCaptureFormat = V4L2_PIX_FMT_MJPEG;	// maybe usb camera
			LOGE("capture format: V4L2_PIX_FMT_MJPEG");
		}
		else
		{
			LOGE("driver should surpport NV21/NV12 or YUYV format, but it not!");
			goto END_ERROR;
		}
	}

	LOGD("openCameraDev execute ok!\n");
	//mCameraFd = camera_fd;
	//return 0;
	return camera_fd;

  END_ERROR:
	if (camera_fd != -1)
	{
		close(camera_fd);
		camera_fd = -1;
	}

	//mCameraFd = camera_fd;
	return -1;
}


void closeCameraDev(int fd)
{
	if (fd != -1)
	{
		close(fd);
		fd = -1;
	}
}


int v4l2SetVideoParams(int fd, int width, int height, unsigned int pix_fmt)
{
	int ret = -1;
	struct v4l2_format format;

	LOGW("%s line %d, w: %d, h: %d, pfmt: %d \n",
		 __FILE__, __LINE__, width, height, pix_fmt);

	memset(&format, 0, sizeof(format));

#ifdef _SUNXIW17_
	if (mCameraType == CAMERA_TYPE_CSI)
	{
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		format.fmt.pix_mp.width = width;
		format.fmt.pix_mp.height = height;
		format.fmt.pix_mp.field = V4L2_FIELD_NONE;
		format.fmt.pix_mp.pixelformat = pix_fmt;
	}
	else
#endif
	{
		int i = 0;
		if (mCameraType == CAMERA_TYPE_TVIN_PAL || mCameraType == CAMERA_TYPE_TVIN_NTSC)
		{
			format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			while(ioctl(fd, VIDIOC_G_FMT, &format) && (i++ < 20))
			{
				LOGD("+++get tvin signal failed.\n");
				return -1;
			}
		}

		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		format.fmt.pix.width = width;
		format.fmt.pix.height = height;

		if (mCaptureFormat == V4L2_PIX_FMT_YUYV)
		{
			format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		}
		else if (mCaptureFormat == V4L2_PIX_FMT_MJPEG)
		{
			format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
		}
		else if (mCaptureFormat == V4L2_PIX_FMT_H264)
		{
			format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
		}
		else
		{
			format.fmt.pix.pixelformat = pix_fmt;
		}

		format.fmt.pix.field = V4L2_FIELD_NONE;
	}

	ret = ioctl(fd, VIDIOC_S_FMT, &format);
	if (ret < 0)
	{
		LOGE("VIDIOC_S_FMT Failed: %s", strerror(errno));
		return ret;
	}

	if ((mCameraType == CAMERA_TYPE_CSI)||(mCameraType ==CAMERA_TYPE_UVC))
	{
		ret = ioctl(fd, VIDIOC_G_FMT, &format);
		if (ret < 0)
		{
			LOGE("VIDIOC_G_FMT Failed: %s", strerror(errno));
			return ret;
		}
		else
		{
			nPlanes = format.fmt.pix_mp.num_planes;
			LOGD("VIDIOC_G_FMT resolution = %d*%d num_planes = %d\n", format.fmt.pix_mp.width,
				 format.fmt.pix_mp.height, format.fmt.pix_mp.num_planes);
		}

		mFrameWidth = format.fmt.pix_mp.width;
		mFrameHeight = format.fmt.pix_mp.height;
	}
	else
	{
		mFrameWidth = format.fmt.pix.width;
		mFrameHeight = format.fmt.pix.height;
	}

	return 0;
}


int v4l2ReqBufs(int fd, int *buf_cnt)
{
	int ret = -1;
	struct v4l2_requestbuffers rb;

	LOGW("TO VIDIOC_REQBUFS count: %d \n", *buf_cnt);

	memset(&rb, 0, sizeof(rb));
	rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

#ifdef _SUNXIW17_
	if (mCameraType == CAMERA_TYPE_CSI)
	{
		rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	}
#endif
	rb.memory = V4L2_MEMORY_MMAP;
	rb.count = *buf_cnt;

	ret = ioctl(fd, VIDIOC_REQBUFS, &rb);
	if (ret < 0)
	{
		LOGE("Init: VIDIOC_REQBUFS failed: %s \n", strerror(errno));
		return ret;
	}

	*buf_cnt = rb.count;
	LOGW("VIDIOC_REQBUFS count: %d \n", *buf_cnt);

	return 0;
}

int buffer_export(int v4lfd, enum v4l2_buf_type bt, int index, int *dmafd)
{
	struct v4l2_exportbuffer expbuf;

	memset(&expbuf, 0, sizeof(expbuf));
	expbuf.type = bt;
	expbuf.index = index;
	expbuf.plane =0;

	if (ioctl(v4lfd, VIDIOC_EXPBUF, &expbuf) == -1)
	{
		perror("VIDIOC_EXPBUF");
		return -1;
	}

	*dmafd = expbuf.fd;
	return 0;
}


int v4l2QueryBuf(int fd, v4l2_mem_map_t *pMapMem)
{
	int ret = -1;
	struct v4l2_buffer buf;
	struct v4l2_exportbuffer exp;

	for(int i = 0; i < mBufferCnt; i++)
	{
		memset(&buf, 0, sizeof(struct v4l2_buffer));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

#ifdef _SUNXIW17_
		if (mCameraType == CAMERA_TYPE_CSI)
		{
			struct v4l2_plane planes[VIDEO_MAX_PLANES];
			memset(planes, 0, VIDEO_MAX_PLANES * sizeof(struct v4l2_plane));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			buf.length = nPlanes;
			buf.m.planes = planes;

			if (NULL == buf.m.planes)
			{
				LOGE("buf.m.planes calloc failed!\n");
			}
		}
#endif
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0)
		{
			LOGE("Unable to query buffer (%s)", strerror(errno));
			return ret;
		}

#ifdef _SUNXIW17_
		if (mCameraType == CAMERA_TYPE_CSI)
		{
			for (int j = 0; j < nPlanes; j++)
			{
				pMapMem->mem[i] = mmap(0, buf.m.planes[j].length, PROT_READ | PROT_WRITE,
									  MAP_SHARED, fd, buf.m.planes[j].m.mem_offset);
				pMapMem->length = buf.length;

				if (pMapMem->mem[i] == MAP_FAILED)
				{
					LOGE("Unable to map buffer (%s)", strerror(errno));

					for (int j = 0; j < i; j++)
					{
						munmap(pMapMem->mem[j], pMapMem->length);
					}

					return -1;
				}

				buffer_export(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, i, &(pMapMem->dmafd[i]));
				LOGW("index: %d, mem: %x, len: %x, offset: %x dmafd=%d \n" , i, \
						(unsigned long) pMapMem->mem[i], buf.length, buf.m.planes[i].m.mem_offset, pMapMem->dmafd[i]);
			}
		}
		else
#endif
		{
			pMapMem->mem[i] = mmap(0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
			pMapMem->length = buf.length;
			LOGW("index: %d, mem: %x, len: %x, offset: %x", i, (unsigned long)(pMapMem->mem[i]), buf.length,
				 buf.m.offset);

			if (pMapMem->mem[i] == MAP_FAILED)
			{
				LOGE("Unable to map buffer (%s)", strerror(errno));
				return -1;
			}

		}

		// start with all buffers in queue
		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret < 0)
		{
			LOGE("VIDIOC_QBUF Failed");
			return ret;
		}

		if ((mCaptureFormat == V4L2_PIX_FMT_MJPEG)||(mCaptureFormat == V4L2_PIX_FMT_YUYV)
			||(mCaptureFormat == V4L2_PIX_FMT_H264))	// star to do
		{
//			int buffer_len = ALIGN_32B(mFrameWidth) * ALIGN_32B(mFrameHeight) * 3 / 2;
//
//			mV4l2CameraMemops.size = buffer_len;
//			ret = allocAlloc(MEM_TYPE_CDX_NEW, &mV4l2CameraMemops, NULL);
//			if(ret < 0){
//				ALOGE("v4l2QueryBuf:allocAlloc failed");
//			}
//			mVideoBuffer.buf_vir_addr[i] = mV4l2CameraMemops.vir;
//			mVideoBuffer.buf_phy_addr[i] = mV4l2CameraMemops.phy;
//
//			LOGW("video buffer: index: %d, vir: %x, phy: %x, len: %x",
//				 i, mVideoBuffer.buf_vir_addr[i], mVideoBuffer.buf_phy_addr[i], buffer_len);
//
//			memset((void *) mVideoBuffer.buf_vir_addr[i], 0x10, mFrameWidth * mFrameHeight);
//			memset((void *) mVideoBuffer.buf_vir_addr[i] + mFrameWidth * mFrameHeight,
//				   0x80, mFrameWidth * mFrameHeight / 2);
			;
		}
	}

	return 0;
}


int v4l2StartStreaming(int fd)
{
	int ret = -1;
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

#ifdef _SUNXIW17_
	if (mCameraType == CAMERA_TYPE_CSI)
	{
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	}
#endif
	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if (ret < 0)
	{
		LOGE("StartStreaming: Unable to start capture: %s", strerror(errno));
		return ret;
	}

	return 0;
}


int v4l2StopStreaming(int fd)
{
	int ret = -1;
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

#ifdef _SUNXIW17_
	if (mCameraType == CAMERA_TYPE_CSI)
	{
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	}
#endif
	ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	if (ret < 0)
	{
		LOGE("StopStreaming: Unable to stop capture: %s", strerror(errno));
		return ret;
	}

	LOGW("V4L2Camera::v4l2StopStreaming OK");
	return 0;
}


int v4l2UnmapBuf(v4l2_mem_map_t *pMapMem)
{
	int ret = -1;

	for (int i = 0; i < mBufferCnt; i++)
	{
		ret = munmap(pMapMem->mem[i], pMapMem->length);

		if (ret < 0)
		{
			LOGE("v4l2CloseBuf Unmap failed");
			return ret;
		}

		pMapMem->mem[i] = NULL;
	}

	return 0;
}


int v4l2setCaptureParams(int fd)
{
	int ret = -1;
	struct v4l2_streamparm params;
	params.parm.capture.timeperframe.numerator = 1;
	params.parm.capture.timeperframe.denominator = mFrameRate;
	params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

#ifdef _SUNXIW17_
	if (mCameraType == CAMERA_TYPE_CSI)
	{
		params.parm.capture.reserved[0] = 0;
		params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	}
#endif
//	if (mTakePictureState == TAKE_PICTURE_NORMAL)
//	{
//		params.parm.capture.capturemode = V4L2_MODE_IMAGE;
//	}
//	else
//	{
		params.parm.capture.capturemode = V4L2_MODE_VIDEO;
//	}

	LOGW("VIDIOC_S_PARM mFrameRate: %d, capture mode: %d \n", mFrameRate,
		 params.parm.capture.capturemode);

	ret = ioctl(fd, VIDIOC_S_PARM, &params);
	if (ret < 0)
	{
		LOGE("v4l2setCaptureParams failed, %s", strerror(errno));
	}
	else
	{
		LOGW("v4l2setCaptureParams ok \n");
	}

	return ret;
}


int tryFmt(int fd, int format)
{
	struct v4l2_fmtdesc fmtdesc;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

#ifdef _SUNXIW17_
	if (mCameraType == CAMERA_TYPE_CSI)
	{
		fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	}
#endif

	for (int i = 0; i < 12; i++)
	{
		fmtdesc.index = i;

		if (-1 == ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc))
		{
			break;
		}

		LOGW("format index = %d, name = %s, v4l2 pixel format = %x\n",
			 i, fmtdesc.description, fmtdesc.pixelformat);

		if (fmtdesc.pixelformat == format)
		{
			return 0;
		}
	}

	return -1;
}


int startDevice(int fd, int width, int height, unsigned int pix_fmt, v4l2_mem_map_t *pMapMem)
{
	int ret = v4l2SetVideoParams(fd, width, height, pix_fmt);	/*for uvc usb by */

	if(ret < 0)
	{
		LOGE("error:v4l2SetVideoParams failed,check if the video is online pls!");
		return EINVAL;
	}

	// set capture mode and fps
	v4l2setCaptureParams(fd);

	// v4l2 request buffers
	int buf_cnt = NB_BUFFER;
	v4l2ReqBufs(fd, &buf_cnt);
	mBufferCnt = buf_cnt;

	// v4l2 query buffers
	if(v4l2QueryBuf(fd, pMapMem) < 0)
	{
		return -1;
	}

	// stream on the v4l2 device
	if(v4l2StartStreaming(fd) < 0)
	{
		return -1;
	}

	return 0;
}

int save_data_as_file(const char *file_name, unsigned char *data, unsigned int data_len)
{
	int fd = 0;
	unsigned int i = 0, left = 0;
	static unsigned int cnt = 0;

	if(cnt >= 2)
	{
		return 1;
	}

	fd = open(file_name, O_RDWR|O_CREAT);

	if(fd < 0)
	{
		return -1;
	}

	left = data_len % 1024;
	lseek(fd, 0, SEEK_SET);

	for(i=0; i<data_len/1024; i++)
	{
		write(fd, data+(i*1024), 1024);
	}

	write(fd, data+(i*1024), left);
	sync();
	close(fd);
	cnt++;
	return 0;
}


void* capture_video(void* para)
{
	int ret = -1;
	int dsm_camera_fd = -1;
	int monitor_camera_fd = -1;

	if((dsm_camera_fd = openCameraDev(DSM_CAMERA_INDEX)) < 0)
	{
		printf("%s line %d calling openCameraDev error!\n" , __FILE__, __LINE__);
		return NULL;
	}

	if(startDevice(dsm_camera_fd, 1280, 720, V4L2_PIX_FMT_NV21, &mMapMem) < 0)
	{
		printf("%s line %d calling startDevice error!\n" , __FILE__, __LINE__);
		return NULL;
	}

	if((monitor_camera_fd = openCameraDev(MONITOR_CAMERA_INDEX)) < 0)
	{
		printf("%s line %d calling openCameraDev error!\n" , __FILE__, __LINE__);
		return NULL;
	}

	if(startDevice(monitor_camera_fd, 1280, 720, V4L2_PIX_FMT_NV21, &monitor_camera_MapMem) < 0)
	{
		printf("%s line %d calling startDevice error!\n" , __FILE__, __LINE__);
		return NULL;
	}

	unsigned int i = 0;
	unsigned int encode_cnt = 0;

	init_video_layer();

#ifdef ROTATE90_DISP
	dma_mem_des_t g2d_image_mem_ops;
	if(allocOpen(MEM_TYPE_DMA, &g2d_image_mem_ops, NULL) < 0)
	{
		printf("ion_alloc_open failed\r\n");
	}
	g2d_image_mem_ops.size = 1280*720*1.5;

	if(allocAlloc(MEM_TYPE_DMA, &g2d_image_mem_ops, NULL) < 0)
	{
		printf("allocAlloc for g2d image memory error!\n");
	}

	int g2d_handle = g2dInit();  //initialize g2d
#endif

#if 0
	//T7_Video_Encode* video_encoder = init_video_encoder(1280, 720, 720, 576, 4, 25, VENC_CODEC_JPEG);
	T7_Video_Encode* video_encoder = init_video_encoder(1280, 720, 720, 576, 4, 25, VENC_CODEC_H264);

	if(video_encoder == (T7_Video_Encode*)(-1))
	{
		printf("init_video_encoder error \n");
		exit(-1);
	}
	else
	{
		printf("init_video_encoder success! \n");
	}


	Video_Queue *video_encoded_queue = init_video_queue();

	if(video_encoded_queue == (Video_Queue*)(-1))
	{
		printf("init_video_queue error \n");
		exit(-1);
	}
	else
	{
		printf("init_video_queue success! \n");
	}


	file_status_t *file_manager = NULL;

	if((file_manager=initFileListDir((char*)("/mnt/sdcard/mmcblk1p1/"), DSM_CAMERA_INDEX, 5, 10)) == (file_status_t*)(-1))
	{
		printf("initFileListDir error \n");
		exit(-1);
	}
	else
	{
		printf("initFileListDir success !\n");
	}

    unsigned char save_video_file = 0;
    FILE *fd_video = NULL;
#endif

	char h264_video_name[256] = "";
	fd_set fds;
	struct timeval tv;
	int max_fd = -1;
	struct v4l2_buffer buf2 = {0};
    struct v4l2_buffer *buf = &buf2;
    unsigned char *temp = NULL;

    unsigned int v4l2_buf_addr_phy_Y_dsm;
    unsigned int v4l2_buf_addr_phy_Y_monitor;
    unsigned int v4l2_buf_addr_vir_Y;

#ifdef ADD_WATERMARK
    char water_mark_str[128] = "";
    char pos_str[32] = "";
#endif

	while(true)
	{
		FD_ZERO(&fds);
		FD_SET(dsm_camera_fd, &fds);
		FD_SET(monitor_camera_fd, &fds);

		 /* Timeout */
		tv.tv_sec  = 2;
		tv.tv_usec = 0;
		max_fd = (dsm_camera_fd > monitor_camera_fd) ? dsm_camera_fd+1:monitor_camera_fd+1 ;
		ret = select(max_fd, &fds, NULL, NULL, &tv);

		if(ret > 0)
		{
			if(FD_ISSET(dsm_camera_fd, &fds))
			{
				buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

				if (mCameraType == CAMERA_TYPE_CSI)
				{
					struct v4l2_plane planes[VIDEO_MAX_PLANES];
					buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
					buf->memory = V4L2_MEMORY_MMAP;
					buf->length = nPlanes;
					buf->m.planes = planes;
				}

				buf->memory = V4L2_MEMORY_MMAP;

				ret = ioctl(dsm_camera_fd, VIDIOC_DQBUF, buf);
				if (ret < 0)
				{
				    printf("GetPreviewFrame: VIDIOC_DQBUF Failed, %s\n", strerror(errno));
				}

				v4l2_buf_addr_phy_Y_dsm = buf->m.offset;	// - 0x20000000;  //dram addr offset for ve
				#ifdef _SUNXIW17_
				if (mCameraType == CAMERA_TYPE_CSI)
				{
                    #ifdef ROTATE90_DISP
					g2dRotate(g2d_handle, G2D_ROTATE90, (unsigned char*)(buf->m.planes[0].m.mem_offset), \
							  1280, 720, (unsigned char*)(g2d_image_mem_ops.phy), 720, 1280);

					#ifdef ADD_WATERMARK
					get_datetime_according_fmt((watermark_array+SYSTIME_DATE_IDX)->content);
					get_gps_format_str(serial_input_var.gps_locate_state, serial_input_var.longtitude, \
								   serial_input_var.latitude, ':', (watermark_array+GPS_POS_IDX)->content);
					sprintf((watermark_array+VEH_SPEED_IDX)->content, "%d KM/H", serial_input_var.vehicle_speed);
					sprintf(water_mark_str, "%d,%d,%s,%d,%d,GPS %s,%d,%d,%s,%d,%d,%s,%d,%d,%s", \
				    (watermark_array+SYSTIME_DATE_IDX)->mPositionX, (watermark_array+SYSTIME_DATE_IDX)->mPositionY, \
				    (watermark_array+SYSTIME_DATE_IDX)->content,\
				    (watermark_array+GPS_POS_IDX)->mPositionX, (watermark_array+GPS_POS_IDX)->mPositionY, (watermark_array+GPS_POS_IDX)->content,\
				    (watermark_array+VEH_SPEED_IDX)->mPositionX, (watermark_array+VEH_SPEED_IDX)->mPositionY, (watermark_array+VEH_SPEED_IDX)->content, \
				    (watermark_array+WARN_TYPE_IDX)->mPositionX, (watermark_array+WARN_TYPE_IDX)->mPositionY, (watermark_array+WARN_TYPE_IDX)->content,\
				    (watermark_array+SOFT_VER_IDX)->mPositionX, (watermark_array+SOFT_VER_IDX)->mPositionY, (watermark_array+SOFT_VER_IDX)->content);
					addWaterMark((unsigned char*)(g2d_image_mem_ops.vir), 720, 1280, water_mark_str, t7_watermark);
					#endif  //ADD_WATERMARK

					copy_dfms_image((unsigned char*)g2d_image_mem_ops.vir); //virtual addr

					#ifdef SAVE_WARN_VIDEO_FILE
					if(0 == pthread_mutex_lock(&(dsm_video_record->file_lock)))  //no-blocking lock
					{
						if((dsm_video_record->sd_card_status == SD_MOUNT) && \
						   (dsm_video_record->file_status != (file_status_t *)(-1)) && \
						   (dsm_video_record->file_status->file_dir_status == FILE_DIR_NORMAL)) //if sd card is normally mounted
						{
							//encoding dsm video
							encode_video_frame_according_to_vir_addr(dsm_video_record->t7_video_encode, \
									(unsigned char*)g2d_image_mem_ops.vir, (unsigned char*)g2d_image_mem_ops.vir+1280*720);
							push_data_into_video_queue(dsm_video_record->video_file_queue, \
									dsm_video_record->t7_video_encode->outputBuffer.pData0, \
									dsm_video_record->t7_video_encode->outputBuffer.nSize0);
						}
						else if(dsm_video_record->sd_card_status == SD_UNMOUNT) //if sd card is not mounted, do nothing
						{
							;
						}

						pthread_mutex_unlock(&(dsm_video_record->file_lock));
					}
					#endif //SAVE_WARN_VIDEO_FILE

					flushCache(MEM_TYPE_DMA, &g2d_image_mem_ops, NULL);
				    //g2d scale just for display
					g2dScale(g2d_handle, (unsigned char*)(g2d_image_mem_ops.phy), 720, 1280, \
							(unsigned char*)v4l2_buf_addr_phy_Y_dsm, 1280, 720);
                    #else
					v4l2_buf_addr_phy_Y_dsm = buf->m.planes[0].m.mem_offset;
                    #endif  //ROTATE90_DISP
				}
				#endif  //_SUNXIW17_

				dsm_camera_display(v4l2_buf_addr_phy_Y_dsm);  //display dsm camera video

				ret = ioctl(dsm_camera_fd, VIDIOC_QBUF, buf);
				if (ret < 0)
				{
					printf("VIDIOC_QBUF Failed, %s\n", strerror(errno));
				}
			}

			//get monitor camera video
			if(FD_ISSET(monitor_camera_fd, &fds))
			{
				buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

				if (mCameraType == CAMERA_TYPE_CSI)
				{
					struct v4l2_plane planes[VIDEO_MAX_PLANES];
					buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
					buf->memory = V4L2_MEMORY_MMAP;
					buf->length = nPlanes;
					buf->m.planes = planes;
				}

				buf->memory = V4L2_MEMORY_MMAP;

				ret = ioctl(monitor_camera_fd, VIDIOC_DQBUF, buf);
				if (ret < 0)
				{
					printf("GetPreviewFrame: VIDIOC_DQBUF Failed, %s\n", strerror(errno));
				}

				v4l2_buf_addr_phy_Y_monitor = buf->m.offset;	// - 0x20000000;  //dram addr offset for ve
				#ifdef _SUNXIW17_
				if (mCameraType == CAMERA_TYPE_CSI)
				{
					v4l2_buf_addr_phy_Y_monitor = buf->m.planes[0].m.mem_offset;
				}
				#endif

				#ifdef ADD_WATERMARK
				strcpy((monitor_watermark_array+SYSTIME_DATE_IDX)->content, (watermark_array+SYSTIME_DATE_IDX)->content);
				strcpy((monitor_watermark_array+GPS_POS_IDX)->content, (watermark_array+GPS_POS_IDX)->content);
				strcpy((monitor_watermark_array+VEH_SPEED_IDX)->content, (watermark_array+VEH_SPEED_IDX)->content);
				sprintf(water_mark_str, "%d,%d,%s,%d,%d,GPS %s,%d,%d,%s,%d,%d,%s,%d,%d,%s", \
			    (monitor_watermark_array+SYSTIME_DATE_IDX)->mPositionX, (monitor_watermark_array+SYSTIME_DATE_IDX)->mPositionY, \
			    (monitor_watermark_array+SYSTIME_DATE_IDX)->content,\
			    (monitor_watermark_array+GPS_POS_IDX)->mPositionX, (monitor_watermark_array+GPS_POS_IDX)->mPositionY, \
			    (monitor_watermark_array+GPS_POS_IDX)->content,\
			    (monitor_watermark_array+VEH_SPEED_IDX)->mPositionX, (monitor_watermark_array+VEH_SPEED_IDX)->mPositionY, \
			    (monitor_watermark_array+VEH_SPEED_IDX)->content, \
			    (monitor_watermark_array+WARN_TYPE_IDX)->mPositionX, (monitor_watermark_array+WARN_TYPE_IDX)->mPositionY, \
			    (monitor_watermark_array+WARN_TYPE_IDX)->content,\
			    (monitor_watermark_array+SOFT_VER_IDX)->mPositionX, (monitor_watermark_array+SOFT_VER_IDX)->mPositionY, \
			    (monitor_watermark_array+SOFT_VER_IDX)->content);
				addWaterMark((unsigned char*)(monitor_camera_MapMem.mem[buf->index]), 1280, 720, water_mark_str, t7_watermark);
				#endif  //ADD_WATERMARK

				#ifdef SAVE_WARN_VIDEO_FILE
				if(0 == pthread_mutex_lock(&(monitor_video_record->file_lock)))  //blocking lock
				{
					if((monitor_video_record->sd_card_status == SD_MOUNT) && \
					   (monitor_video_record->file_status != (file_status_t *)(-1)) && \
					   (monitor_video_record->file_status->file_dir_status == FILE_DIR_NORMAL)) //if sd card is normally mounted
					{
						//encoding dsm video
						encode_video_frame_according_to_vir_addr(monitor_video_record->t7_video_encode, \
								(unsigned char*)(monitor_camera_MapMem.mem[buf->index]), \
								(unsigned char*)(monitor_camera_MapMem.mem[buf->index])+1280*720);
						push_data_into_video_queue(monitor_video_record->video_file_queue, \
								monitor_video_record->t7_video_encode->outputBuffer.pData0, \
								monitor_video_record->t7_video_encode->outputBuffer.nSize0);
					}
					else if(monitor_video_record->sd_card_status == SD_UNMOUNT) //if sd card is not mounted, do nothing
					{
						;
					}

					pthread_mutex_unlock(&(monitor_video_record->file_lock));
				}
				#endif //SAVE_WARN_VIDEO_FILE

				monitor_camera_display(v4l2_buf_addr_phy_Y_monitor); //display monitor camera video
				copy_monitor_image((unsigned char*)(monitor_camera_MapMem.mem[buf->index]));

				ret = ioctl(monitor_camera_fd, VIDIOC_QBUF, buf);
				if (ret < 0)
				{
					printf("VIDIOC_QBUF Failed, %s\n", strerror(errno));
				}
			}
		}
	}

	pthread_exit(NULL);
	//return NULL;
}


int release_camera_v4l2()
{
	int i = 0;

	for(i=0; i<NB_BUFFER; i++)
	{
		if(mMapMem.mem[i])
		{
			munmap(mMapMem.mem[i], mMapMem.length);
			mMapMem.mem[i] = NULL;
		}
	}

	return 0;
}




