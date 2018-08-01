/*
 * v4l2_tvin.c
 *
 *  Created on: Mar 16, 2018
 *      Author: tony
 */
/*
 * v4l2_tvin.c
 *
 *  Created on: Nov 16, 2017
 *      Author: tony
 */
#include "v4l2_tvin.h"


#if HJWS_HARDWARE_I
static char v4l_capture_dev[100] = "/dev/video6";
#else
static char v4l_capture_dev[100] = "/dev/video0";
#endif

static char v4l_output_dev[100] = "/dev/video17";
static  int fd_capture_v4l = 0;
static  int fd_output_v4l = 0;
static  int g_cap_mode = 0;
static  int g_input = 1;
static  int g_fmt = V4L2_PIX_FMT_UYVY;
static  int g_rotate = 0;
static  int g_vflip = 0;
static  int g_hflip = 0;
static  int g_vdi_enable = 0;
static  int g_vdi_motion = 0;
static  int g_tb = 0;
static  int g_output = 3;
static  int g_output_num_buffers = 4;
static  int g_capture_num_buffers = 3;
static  int g_in_width = 0;
static  int g_in_height = 0;
static  int g_display_width = IMAGE_WIDTH;
static  int g_display_height = IMAGE_HEIGHT;
static  int g_display_top = 0;
static  int g_display_left = 0;
static  int g_frame_size;
static  int g_frame_period = 33333;
static  v4l2_std_id g_current_std = V4L2_STD_NTSC;

struct testbuffer output_buffers[4];
struct testbuffer capture_buffers[3];

unsigned char gray_image[IMAGE_WIDTH*IMAGE_HEIGHT];
unsigned char YUYV_image[IMAGE_WIDTH*IMAGE_HEIGHT*2];

pthread_mutex_t uyvy_image_mutex = PTHREAD_MUTEX_INITIALIZER;

static void get_Ycompnt_from_YUYV(const unsigned char* YUYV_image, unsigned char* gray_image, unsigned char YUYV_type)
{
	int i = 0;

	for(i=0; i<g_frame_size; i++)
	{
		if(YUYV_type == 0)
		{
			if(i%2 == 0)
			{
				*(gray_image++) = *(YUYV_image + i);
			}
		}
		else
		{
			if(i%2 == 1)
			{
				*(gray_image++) = *(YUYV_image + i);
			}
		}
	}
}


static int xioctl(int fd, int request, void* argp)
{
	int r;

	do
	{
		r = ioctl(fd, request, argp);
	}
	while (-1 == r && EINTR == errno);

	return r;
}


int v4l_capture_setup(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	//struct v4l2_dbg_chip_ident chip;
	struct v4l2_streamparm parm;
	v4l2_std_id id;
	unsigned int min;

	if (ioctl (fd_capture_v4l, VIDIOC_QUERYCAP, &cap) < 0)
	{
		if (EINVAL == errno)
		{
			fprintf (stderr, "%s is no V4L2 device\n", v4l_capture_dev);
			return TFAIL;
		}
		else
		{
			fprintf (stderr, "%s isn not V4L device,unknow error\n", v4l_capture_dev);
			return TFAIL;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		fprintf (stderr, "%s is no video capture device\n", v4l_capture_dev);
		return TFAIL;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING))
	{
		fprintf (stderr, "%s does not support streaming i/o\n", v4l_capture_dev);
		return TFAIL;
	}

#if 0
	if (ioctl(fd_capture_v4l, VIDIOC_DBG_G_CHIP_IDENT, &chip))
	{
		printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
		close(fd_capture_v4l);
		return TFAIL;
	}
	printf("TV decoder chip is %s\n", chip.match.name);
#endif

	if (ioctl(fd_capture_v4l, VIDIOC_S_INPUT, &g_input) < 0)
	{
		printf("VIDIOC_S_INPUT failed\n");
		close(fd_capture_v4l);
		return TFAIL;
	}

	if (ioctl(fd_capture_v4l, VIDIOC_G_STD, &id) < 0)
	{
		printf("VIDIOC_G_STD failed\n");
		close(fd_capture_v4l);
		return TFAIL;
	}

	g_current_std = id;

	if (ioctl(fd_capture_v4l, VIDIOC_S_STD, &id) < 0)
	{
		printf("VIDIOC_S_STD failed\n");
		close(fd_capture_v4l);
		return TFAIL;
	}

	/* Select video input, video standard and tune here. */
	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl (fd_capture_v4l, VIDIOC_CROPCAP, &cropcap) < 0)
	{
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (ioctl (fd_capture_v4l, VIDIOC_S_CROP, &crop) < 0)
		{
			switch (errno)
			{
				case EINVAL:
					/* Cropping not supported. */
					fprintf (stderr, "%s  doesn't support crop\n", v4l_capture_dev);
					break;
				default:
					/* Errors ignored. */
					break;
			}
		}
	}
	else
	{
		/* Errors ignored. */
	}

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = 0;
	parm.parm.capture.capturemode = 0;
	if (ioctl(fd_capture_v4l, VIDIOC_S_PARM, &parm) < 0)
	{
		printf("VIDIOC_S_PARM failed\n");
		close(fd_capture_v4l);
		return TFAIL;
	}

	memset(&fmt, 0, sizeof(fmt));
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = 720;
	fmt.fmt.pix.height      = 480;
	fmt.fmt.pix.pixelformat = g_fmt;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	if (ioctl (fd_capture_v4l, VIDIOC_S_FMT, &fmt) < 0)
	{
		fprintf (stderr, "%s iformat not supported \n", v4l_capture_dev);
		return TFAIL;
	}

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;

	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	if (ioctl(fd_capture_v4l, VIDIOC_G_FMT, &fmt) < 0)
	{
		printf("VIDIOC_G_FMT failed\n");
		close(fd_capture_v4l);
		return TFAIL;
	}

	g_in_width = fmt.fmt.pix.width;
	g_in_height = fmt.fmt.pix.height;

	memset(&req, 0, sizeof (req));
	req.count  = g_capture_num_buffers;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (ioctl (fd_capture_v4l, VIDIOC_REQBUFS, &req) < 0)
	{
		if (EINVAL == errno)
		{
			fprintf (stderr, "%s does not support "
					 "memory mapping\n", v4l_capture_dev);
			return TFAIL;
		}
		else
		{
			fprintf (stderr, "%s does not support "\
					"memory mapping, unknow error\n", v4l_capture_dev);
			return TFAIL;
		}
	}

	if (req.count < 2)
	{
		fprintf (stderr, "Insufficient buffer memory on %s\n", v4l_capture_dev);
		return TFAIL;
	}

	return 0;
}


int v4l_output_setup(void)
{
	struct v4l2_control ctrl;
	struct v4l2_format fmt;
	struct v4l2_framebuffer fb;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_capability cap;
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_requestbuffers buf_req;

	if (!ioctl(fd_output_v4l, VIDIOC_QUERYCAP, &cap))
	{
		printf("driver=%s, card=%s, bus=%s, "
			"version=0x%08x, "
			"capabilities=0x%08x\n",
			cap.driver, cap.card, cap.bus_info,
			cap.version,
			cap.capabilities);
	}

	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	while (!ioctl(fd_output_v4l, VIDIOC_ENUM_FMT, &fmtdesc))
	{
		printf("fmt %s: fourcc = 0x%08x\n",
			fmtdesc.description,
			fmtdesc.pixelformat);
		fmtdesc.index++;
	}

	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (ioctl(fd_output_v4l, VIDIOC_CROPCAP, &cropcap) < 0)
	{
		printf("get crop capability failed\n");
		close(fd_output_v4l);
		return TFAIL;
	}

	crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	crop.c.top = g_display_top;
	crop.c.left = g_display_left;
	crop.c.width = g_display_width;
	crop.c.height = g_display_height;
	if (ioctl(fd_output_v4l, VIDIOC_S_CROP, &crop) < 0)
	{
		printf("set crop failed\n");
		close(fd_output_v4l);
		return TFAIL;
	}

	// Set rotation
	ctrl.id = V4L2_CID_ROTATE;
	ctrl.value = g_rotate;
	if (ioctl(fd_output_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
	{
		printf("set ctrl rotate failed\n");
		close(fd_output_v4l);
		return TFAIL;
	}
	ctrl.id = V4L2_CID_VFLIP;
	ctrl.value = g_vflip;
	if (ioctl(fd_output_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
	{
		printf("set ctrl vflip failed\n");
		close(fd_output_v4l);
		return TFAIL;
	}
	ctrl.id = V4L2_CID_HFLIP;
	ctrl.value = g_hflip;
	if (ioctl(fd_output_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
	{
		printf("set ctrl hflip failed\n");
		close(fd_output_v4l);
		return TFAIL;
	}

	if (g_vdi_enable) {
		ctrl.id = V4L2_CID_MXC_MOTION;
		ctrl.value = g_vdi_motion;
		if (ioctl(fd_output_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
		{
			printf("set ctrl motion failed\n");
			close(fd_output_v4l);
			return TFAIL;
		}
	}

	fb.flags = V4L2_FBUF_FLAG_OVERLAY;
	ioctl(fd_output_v4l, VIDIOC_S_FBUF, &fb);

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width= g_in_width;
	fmt.fmt.pix.height= g_in_height;
	fmt.fmt.pix.pixelformat = g_fmt;
	fmt.fmt.pix.bytesperline = g_in_width;
	fmt.fmt.pix.priv = 0;
	fmt.fmt.pix.sizeimage = 0;
	if (g_tb)
		fmt.fmt.pix.field = V4L2_FIELD_INTERLACED_TB;
	else
		fmt.fmt.pix.field = V4L2_FIELD_INTERLACED_BT;
	if (ioctl(fd_output_v4l, VIDIOC_S_FMT, &fmt) < 0)
	{
		printf("set format failed\n");
		return TFAIL;
	}

	if (ioctl(fd_output_v4l, VIDIOC_G_FMT, &fmt) < 0)
	{
		printf("get format failed\n");
		return TFAIL;
	}
	g_frame_size = fmt.fmt.pix.sizeimage;

	memset(&buf_req, 0, sizeof(buf_req));
	buf_req.count = g_output_num_buffers;
	buf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf_req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(fd_output_v4l, VIDIOC_REQBUFS, &buf_req) < 0)
	{
		printf("request buffers failed\n");
		return TFAIL;
	}

	return 0;
}


int start_capturing(void)
{
	unsigned int i;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	for (i = 0; i < g_capture_num_buffers; i++)
	{
		memset(&buf, 0, sizeof (buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (xioctl(fd_capture_v4l, VIDIOC_QUERYBUF, &buf) < 0)
		{
				printf("VIDIOC_QUERYBUF error\n");
				return TFAIL;
		}

		capture_buffers[i].length = buf.length;
		capture_buffers[i].offset = (size_t) buf.m.offset;
		capture_buffers[i].start = (unsigned char*)mmap (NULL, capture_buffers[i].length,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd_capture_v4l, capture_buffers[i].offset);
		memset(capture_buffers[i].start, 0xFF, capture_buffers[i].length);
	}

	for (i = 0; i < g_capture_num_buffers; i++)
	{
		memset(&buf, 0, sizeof (buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		buf.m.offset = capture_buffers[i].offset;
		if (xioctl (fd_capture_v4l, VIDIOC_QBUF, &buf) < 0)
		{
			printf("VIDIOC_QBUF error\n");
			return TFAIL;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (xioctl (fd_capture_v4l, VIDIOC_STREAMON, &type) < 0)
	{
		printf("VIDIOC_STREAMON error\n");
		return TFAIL;
	}

	return 0;
}


int prepare_output(void)
{
	int i;
	struct v4l2_buffer output_buf;
	enum v4l2_buf_type type;

	for (i = 0; i < g_output_num_buffers; i++)
	{
		memset(&output_buf, 0, sizeof(output_buf));
		output_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		output_buf.memory = V4L2_MEMORY_MMAP;
		output_buf.index = i;
		if (xioctl(fd_output_v4l, VIDIOC_QUERYBUF, &output_buf) < 0)
		{
			printf("VIDIOC_QUERYBUF error\n");
			return TFAIL;
		}

		output_buffers[i].length = output_buf.length;
		output_buffers[i].offset = (size_t) output_buf.m.offset;
		output_buffers[i].start = (unsigned char*)mmap (NULL, output_buffers[i].length,
						PROT_READ | PROT_WRITE, MAP_SHARED,
						fd_output_v4l, output_buffers[i].offset);
		if (output_buffers[i].start == NULL)
		{
			printf("v4l2 tvin test: output mmap failed\n");
			return TFAIL;
		}
	}

	for (i = 0; i < g_output_num_buffers; i++)
	{
		memset(&output_buf, 0, sizeof(output_buf));
		output_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		output_buf.memory = V4L2_MEMORY_MMAP;
		output_buf.index = i;

		if (xioctl(fd_output_v4l, VIDIOC_QBUF, &output_buf) < 0)
		{
			printf("VIDIOC_QUERYBUF failed\n");
			return TFAIL;
		}
	}

	/*
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (xioctl(fd_output_v4l, VIDIOC_STREAMON, &type) < 0)
	{
		printf("Could not start stream\n");
		return TFAIL;
	}
	*/

	return 0;
}



int mxc_v4l_tvin_test(void)
{
	struct v4l2_buffer capture_buf, output_buf;
	//v4l2_std_id id;
	unsigned int i, j;
	enum v4l2_buf_type type;
	int total_time;
	struct timeval tv_start, tv_current;
	unsigned char gray_image[720*480*2];
	char warning_str[16] = "";

v4l2_init:
	if (v4l_capture_setup() < 0)
	{
		printf("Setup v4l capture failed.\n");
	}

	if (start_capturing() < 0)
	{
		printf("start_capturing failed\n");
	}

	if (v4l_output_setup() < 0)
	{
		printf("Setup v4l output failed.\n");
	}

	if (prepare_output() < 0)
	{
		printf("prepare_output failed\n");
	}

	gettimeofday(&tv_start, NULL);

	for (i = 0; ; i++)
	{
		memset(&capture_buf, 0, sizeof(capture_buf));
		capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		capture_buf.memory = V4L2_MEMORY_MMAP;

		if (xioctl(fd_capture_v4l, VIDIOC_DQBUF, &capture_buf) < 0)
		{
			printf("capture VIDIOC_QBUF failed\n");
		}

		if(g_frame_size > IMAGE_WIDTH*IMAGE_HEIGHT*2)
		{
			g_frame_size = IMAGE_WIDTH*IMAGE_HEIGHT*2;
		}

		memcpy(YUYV_image, capture_buffers[capture_buf.index].start, g_frame_size);

		if (xioctl(fd_capture_v4l, VIDIOC_QBUF, &capture_buf) < 0)
		{
			printf("capture VIDIOC_QBUF failed\n");
		}

		if(i < 3)
		{
			continue;
		}
		else if(3 == i)
		{
			type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

			if (xioctl(fd_output_v4l, VIDIOC_STREAMON, &type) < 0)
			{
				printf("Could not start stream\n");
				return TFAIL;
			}
		}

		memset(&output_buf, 0, sizeof(output_buf));
		output_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		output_buf.memory = V4L2_MEMORY_MMAP;

		if (xioctl(fd_output_v4l, VIDIOC_DQBUF, &output_buf) < 0)
		{
			printf("output VIDIOC_DQBUF failed\n");
		}

		/*copy image buffer of camera to display buffer */
		uyvy_2_gray(YUYV_image, gray_image);
		//DrawHz32(40, 10, A_IMAGE_WIDTH , serial_output_var.warning_sub_state, gray_image);
		switch(serial_output_var.warning_sub_state)
		{
		case 0:
		default:
			strcpy(warning_str, "nowarning");
			break;

		case 1:
			strcpy(warning_str, "yawning");
			break;

		case 2:
			strcpy(warning_str, "distracting");
			break;

		case 3:
			strcpy(warning_str, "smoking");
			break;

		case 4:
			strcpy(warning_str, "phoning");
			break;

		case 5:
			strcpy(warning_str, "nodriver");
			break;

		case 6:
			strcpy(warning_str, "closingeye");
			break;
		}

		disp_str_on_monitor(40, 10, warning_str, gray_image);
		gray_2_uyvy(gray_image, YUYV_image);

		memcpy(output_buffers[output_buf.index].start, YUYV_image, sizeof(YUYV_image));

		output_buf.timestamp.tv_sec = tv_start.tv_sec;
		output_buf.timestamp.tv_usec = tv_start.tv_usec + (g_frame_period * i);

		if (g_vdi_enable)
		{
			output_buf.field = g_tb ? V4L2_FIELD_INTERLACED_TB :
						  V4L2_FIELD_INTERLACED_BT;
		}
		if (xioctl(fd_output_v4l, VIDIOC_QBUF, &output_buf) < 0)
		{
			printf(" output VIDIOC_QBUF failed\n");
		}
	}

	gettimeofday(&tv_current, 0);
	total_time = (tv_current.tv_sec - tv_start.tv_sec) * 1000000L;
	total_time += tv_current.tv_usec - tv_start.tv_usec;
	printf("total time for %u frames = %u us =  %lld fps\n", i, total_time, (i * 1000000ULL) / total_time);

	return 0;
}



int process_cmdline(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-ow") == 0)
		{
			g_display_width = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-oh") == 0)
		{
			g_display_height = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-ot") == 0)
		{
			g_display_top = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-ol") == 0)
		{
			g_display_left = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-r") == 0)
		{
			g_rotate = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-f") == 0)
		{
			i++;
			g_fmt = v4l2_fourcc(argv[i][0], argv[i][1],argv[i][2],argv[i][3]);

			if ((g_fmt != V4L2_PIX_FMT_NV12) &&
				(g_fmt != V4L2_PIX_FMT_UYVY) &&
				(g_fmt != V4L2_PIX_FMT_YUYV) &&
				(g_fmt != V4L2_PIX_FMT_YUV420))
			{
					printf("Default format is used: UYVY\n");
			}
		}
		else if (strcmp(argv[i], "-m") == 0)
		{
			g_vdi_enable = 1;
			g_vdi_motion = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-tb") == 0)
		{
			g_tb = 1;
		}
		else if (strcmp(argv[i], "-help") == 0)
		{
			printf("MXC Video4Linux TVin Test\n\n" \
				   "Syntax: mxc_v4l2_tvin.out\n" \
				   " -ow <capture display width>\n" \
				   " -oh <capture display height>\n" \
				   " -ot <display top>\n" \
				   " -ol <display left>\n" \
							   " -r <rotation> -c <capture counter> \n"
				   " -m <motion> 0:medium 1:low 2:high, 0-default\n"
				   " -tb top field first, bottom field first-default\n"
				   " -f <format, only YU12, YUYV, UYVY and NV12 are supported> \n");
			return TFAIL;
		}
	}

	if ((g_display_width == 0) || (g_display_height == 0))
	{
		printf("Zero display width or height\n");
		return TFAIL;
	}

	return 0;
}



void* sample_image_task(void* argv)
{
#ifdef BUILD_FOR_ANDROID
	char fb_device[100] = "/dev/graphics/fb0";
#else
	char fb_device[100] = "/dev/fb0";
#endif
	int fd_fb = 0, i;
	struct mxcfb_gbl_alpha alpha;
	enum v4l2_buf_type type;

	/*avoid black screen caused by screen protection */
	int fd_tty;
	fd_tty = open("/dev/tty0", O_RDWR);
	write(fd_tty, "\033[9;0]", 8);
	close(fd_tty);

	if ((fd_capture_v4l = open(v4l_capture_dev, O_RDWR, 0)) < 0)
	{
		printf("Unable to open %s\n", v4l_capture_dev);
		return NULL;
	}

	if ((fd_output_v4l = open(v4l_output_dev, O_RDWR, 0)) < 0)
	{
		printf("Unable to open %s\n", v4l_output_dev);
		return NULL;
	}


	if ((fd_fb = open(fb_device, O_RDWR )) < 0)
	{
		printf("Unable to open frame buffer\n");
		close(fd_capture_v4l);
		close(fd_output_v4l);
		return NULL;
	}

	/* Overlay setting */
	alpha.alpha = 0;
	alpha.enable = 1;
	if (ioctl(fd_fb, MXCFB_SET_GBL_ALPHA, &alpha) < 0)
	{
		printf("Set global alpha failed\n");
		close(fd_fb);
		close(fd_capture_v4l);
		close(fd_output_v4l);
		return NULL;
	}

	mxc_v4l_tvin_test();

	/* close outputting image */
	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ioctl(fd_output_v4l, VIDIOC_STREAMOFF, &type);

	/* close inputting image*/
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd_capture_v4l, VIDIOC_STREAMOFF, &type);

	/* release memory mapping*/
	for (i = 0; i < g_output_num_buffers; i++)
	{
		munmap(output_buffers[i].start, output_buffers[i].length);
	}

	/* release memory mapping */
	for (i = 0; i < g_capture_num_buffers; i++)
	{
		munmap(capture_buffers[i].start, capture_buffers[i].length);
	}

	close(fd_capture_v4l);  // close camera
	close(fd_output_v4l);   //close monitor
	close(fd_fb);

	pthread_exit(NULL);

	//return NULL;
}


