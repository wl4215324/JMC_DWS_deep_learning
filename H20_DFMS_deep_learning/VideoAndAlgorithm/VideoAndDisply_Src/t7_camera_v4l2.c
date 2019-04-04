/*
 * t7_camera_v4l2.c
 *
 *  Created on: Mar 21, 2019
 *      Author: tony
 */

#include "t7_camera_v4l2.h"

static int nPlanes = 0;
static int mCameraFd = -1;
static int mCameraType = 0;
static int mCaptureFormat;  // the driver capture format
static int mFrameWidth = 0, mFrameHeight = 0;
static int mFrameRate = 30;
v4l2_mem_map_t mMapMem;
// actually buffer counts
static int mBufferCnt;

video_callback callback;

static int waitFrame(void)
{
	fd_set fds;
	struct timeval tv;
	int ret;

	FD_ZERO(&fds);
	FD_SET(mCameraFd, &fds);

	 /* Timeout */
	tv.tv_sec  = 2;
	tv.tv_usec = 0;
	ret = select(mCameraFd + 1, &fds, NULL, NULL, &tv);

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

int tryFmt(int format);

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

	mCameraFd = open(dev_node, O_RDWR | O_NONBLOCK, 0);

	if (mCameraFd == -1)
	{
		LOGE("ERROR opening %s: %s", dev_node, strerror(errno));
		return -1;
	}

	//-------------------------------------------------
	// check v4l2 device capabilities
	ret = ioctl(mCameraFd, VIDIOC_QUERYCAP, &cap);

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
		tvinType = getTVINSystemType(mCameraFd, cvbs_type);

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
		printf("TVIN std is: %d\n", getTVINSystemType(mCameraFd, cvbs_type));
		mCameraType = CAMERA_TYPE_CSI;
		LOGD("mCameraType = CAMERA_TYPE_CSI \n");
	}

	LOGD("The name of the Camera is '%s' \n", cap.card);

	if (mCameraType == CAMERA_TYPE_CSI)
	{
		// uvc do not need to set input
		inp.index = 0;
		if (-1 == ioctl(mCameraFd, VIDIOC_S_INPUT, &inp))
		{
			LOGE("VIDIOC_S_INPUT error id=%d!", camera_index);
			goto END_ERROR;
		}
	}

	if (mCameraType == CAMERA_TYPE_UVC)
	{
		// try to support this format: NV21, YUYV
		// we do not support mjpeg camera now

		if (tryFmt(V4L2_PIX_FMT_NV21) == 0)
		{
			mCaptureFormat = V4L2_PIX_FMT_NV21;
			LOGE("capture format: V4L2_PIX_FMT_NV21");
		}

		else if (tryFmt(V4L2_PIX_FMT_H264) == 0)
		{
			mCaptureFormat = V4L2_PIX_FMT_H264;
			LOGW("capture format: V4L2_PIX_FMT_H264");
		}
		else if (tryFmt(V4L2_PIX_FMT_YUYV) == 0)
		{
			mCaptureFormat = V4L2_PIX_FMT_YUYV;	// maybe usb camera
			LOGW("capture format: V4L2_PIX_FMT_YUYV");
		}
		else if (tryFmt(V4L2_PIX_FMT_MJPEG) == 0)
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
	return 0;

  END_ERROR:
	if (mCameraFd != -1)
	{
		close(mCameraFd);
		mCameraFd = -1;
	}

	return -1;
}


void closeCameraDev()
{
	if (mCameraFd != -1)
	{
		close(mCameraFd);
		mCameraFd = -1;
	}
}

int v4l2SetVideoParams(int width, int height, unsigned int pix_fmt)
{
	int ret = -1;
	struct v4l2_format format;

	LOGW("%s, line: %d, w: %d, h: %d, pfmt: %d \n",
		 __FUNCTION__, __LINE__, width, height, pix_fmt);

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
			while (ioctl(mCameraFd, VIDIOC_G_FMT, &format) && (i++ < 20))
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

	ret = ioctl(mCameraFd, VIDIOC_S_FMT, &format);
	if (ret < 0)
	{
		LOGE("VIDIOC_S_FMT Failed: %s", strerror(errno));
		return ret;
	}

	if ((mCameraType == CAMERA_TYPE_CSI)||(mCameraType ==CAMERA_TYPE_UVC))
	{
		ret = ioctl(mCameraFd, VIDIOC_G_FMT, &format);
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


int v4l2ReqBufs(int *buf_cnt)
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

	ret = ioctl(mCameraFd, VIDIOC_REQBUFS, &rb);
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

int v4l2QueryBuf()
{
	int ret = -1;
	struct v4l2_buffer buf;
	struct v4l2_exportbuffer exp;

	for (int i = 0; i < mBufferCnt; i++)
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

		ret = ioctl(mCameraFd, VIDIOC_QUERYBUF, &buf);
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
				mMapMem.mem[i] = mmap(0, buf.m.planes[j].length,
									  PROT_READ | PROT_WRITE,
									  MAP_SHARED, mCameraFd, buf.m.planes[j].m.mem_offset);
				mMapMem.length = buf.length;

				if (mMapMem.mem[i] == MAP_FAILED)
				{
					LOGE("Unable to map buffer (%s)", strerror(errno));

					for (int j = 0; j < i; j++)
					{
						munmap(mMapMem.mem[j], mMapMem.length);
					}

					return -1;
				}

				buffer_export(mCameraFd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, i, &mMapMem.dmafd[i]);

				LOGW("index: %d, mem: %x, len: %x, offset: %x dmafd=%d \n" , i,
					(unsigned long) mMapMem.mem[i], buf.length, buf.m.planes[i].m.mem_offset, mMapMem.dmafd[i]);
			}
		}
		else
#endif
		{
			mMapMem.mem[i] = mmap(0, buf.length,
								  PROT_READ | PROT_WRITE, MAP_SHARED, mCameraFd, buf.m.offset);
			mMapMem.length = buf.length;
			LOGW("index: %d, mem: %x, len: %x, offset: %x", i, (int) mMapMem.mem[i], buf.length,
				 buf.m.offset);

			if (mMapMem.mem[i] == MAP_FAILED)
			{
				LOGE("Unable to map buffer (%s)", strerror(errno));
				return -1;
			}

		}

		// start with all buffers in queue
		ret = ioctl(mCameraFd, VIDIOC_QBUF, &buf);
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


int v4l2StartStreaming()
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
	ret = ioctl(mCameraFd, VIDIOC_STREAMON, &type);
	if (ret < 0)
	{
		LOGE("StartStreaming: Unable to start capture: %s", strerror(errno));
		return ret;
	}

	return 0;
}

int v4l2StopStreaming()
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
	ret = ioctl(mCameraFd, VIDIOC_STREAMOFF, &type);
	if (ret < 0)
	{
		LOGE("StopStreaming: Unable to stop capture: %s", strerror(errno));
		return ret;
	}

	LOGW("V4L2Camera::v4l2StopStreaming OK");
	return 0;
}


int v4l2UnmapBuf()
{
	int ret = -1;

	for (int i = 0; i < mBufferCnt; i++)
	{
		ret = munmap(mMapMem.mem[i], mMapMem.length);

		if (ret < 0)
		{
			LOGE("v4l2CloseBuf Unmap failed");
			return ret;
		}

		mMapMem.mem[i] = NULL;
	}

	return 0;
}

int v4l2setCaptureParams()
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

	ret = ioctl(mCameraFd, VIDIOC_S_PARM, &params);
	if (ret < 0)
		LOGE("v4l2setCaptureParams failed, %s", strerror(errno));
	else
		LOGW("v4l2setCaptureParams ok \n");

	return ret;
}


int tryFmt(int format)
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

		if (-1 == ioctl(mCameraFd, VIDIOC_ENUM_FMT, &fmtdesc))
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


int startDevice(int width, int height, unsigned int pix_fmt)
{
	LOGD("%s, wxh: %dx%d, fmt: %d\n", __FUNCTION__, width, height, pix_fmt);

	int ret = v4l2SetVideoParams(width, height, pix_fmt);	/*for uvc usb by */

	if(ret < 0)
	{
		LOGE("error:v4l2SetVideoParams failed,check if the video is online pls!");
		return EINVAL;
	}

	// set capture mode and fps
	v4l2setCaptureParams();

	// v4l2 request buffers
	int buf_cnt = NB_BUFFER;
	v4l2ReqBufs(&buf_cnt);
	mBufferCnt = buf_cnt;

	// v4l2 query buffers
	if(v4l2QueryBuf() < 0)
	{
		return -1;
	}

	// stream on the v4l2 device
	if(v4l2StartStreaming() < 0)
	{
		return -1;
	}

	return 0;
}

void* capture_video(void* para)
{
	int ret = -1;
	int camera_dev_index = *(int*)para;

	if(openCameraDev(camera_dev_index) < 0)
	{
		printf("file %s line %d calling openCameraDev error!\n" , __FILE__, __LINE__);
	}

	if(startDevice(1280, 720, V4L2_PIX_FMT_NV21) < 0)
	{
		printf("file %s line %d calling startDevice error!\n" , __FILE__, __LINE__);
	}

	while(true)
	{
		if(!waitFrame())
		{
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
			}

			//------------do calllback
			callback(mMapMem.mem[buf->index], 1280*720*1.5);

			ret = ioctl(mCameraFd, VIDIOC_QBUF, buf);
			if (ret < 0)
			{
				printf("VIDIOC_QBUF Failed, %s\n", strerror(errno));
			}
		}
	}

	pthread_exit(NULL);
}

