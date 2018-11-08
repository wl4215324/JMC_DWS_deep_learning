/*
 * production_test.c
 *
 *  Created on: Oct 23, 2018
 *      Author: tony
 */
#include "production_test.h"
#include "v4l2_tvin.h"

unsigned char rs485_test_flag = 0;

const static char test_picture_name[32] = "/home/user/test/yawn.yuv";




int read_test_picture(unsigned char *read_buffer, int *buffer_length)
{
	int fd = 0;

	if((fd = open(test_picture_name, O_RDONLY)) < 0)
	{
		return -1;  // failed to read test picture
	}

	lseek(fd, 0, SEEK_SET);

	if((*buffer_length = read(fd, read_buffer, IMAGE_SIZE_FOR_YUVY)) < 0 )
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
