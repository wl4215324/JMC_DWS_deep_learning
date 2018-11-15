/*
 * production_test.c
 *
 *  Created on: Oct 23, 2018
 *      Author: tony
 */
#include "production_test.h"
#include "v4l2_tvin.h"

unsigned char rs485_test_flag = 0;

/* production test image for yawn testing */
const static char test_picture_name[32] = "/home/user/test/yawn.yuv";

/* choice for production test or normal working mode */
const static char test_mode_file_path[32] = "/home/user/WorkMode";


/*
 * The following function mainly reads yuv picture from local file
 * return
 *      -1: error, nonnegative number: length of yuv image converted into linear array
 * arguments
 *     unsigned char *read_buffer: buffer to be stored yuv image
 *     int *buffer_length: length of yuv image converted into linear array
 */
int read_test_picture(unsigned char *read_buffer, int *buffer_length)
{
	int fd = 0;

	if((fd = open(test_picture_name, O_RDONLY)) < 0)
	{
		return -1;  // failed to read test picture
	}

	lseek(fd, 0, SEEK_SET);

	if((*buffer_length = read(fd, read_buffer, YUYV_IMAGE_SIZE)) < 0 )
	{
		close(fd);
		return -1;
	}
	else
	{
		close(fd);
		return *buffer_length;
	}
}

/*
 * The following function mainly reads work status flag
 * return
 *     -1: error, nonnegative number: working mode flag
 *
 * arguments
 *     none
 */
int get_production_test_mode()
{
	int fd = 0;
	char read_buffer[8] = "";

	if((fd = open(test_mode_file_path, O_RDONLY)) < 0)
	{
		return -1;  // failed to read rs485_test_flag
	}

	lseek(fd, 0, SEEK_SET);

	/* if failed to read working mode flag, return -1*/
	if(read(fd, read_buffer, sizeof(read_buffer)) < 0 )
	{
		close(fd);
		return -1;
	}
	else  // if successfully read working mode
	{
		close(fd);
		rs485_test_flag = atoi(read_buffer);
		return rs485_test_flag;
	}
}
