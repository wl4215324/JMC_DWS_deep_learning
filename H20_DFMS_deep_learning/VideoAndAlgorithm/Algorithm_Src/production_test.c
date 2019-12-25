/*
 * production_test.c
 *
 *  Created on: Nov 27, 2019
 *      Author: tony
 */

#include <sys/mman.h>
#include "production_test.h"

Factory_Test_Image factory_test_data;

void* open_and_mmap_file(const char *file_name, int *fd, unsigned long *length)
{
	int ret = 0;
	unsigned long file_size = 0;
	void *file_content = NULL;

	if(!file_name || !fd || !length)
	{
		return (void*)(-1);
	}

	ret = open(file_name, O_RDWR);

	if(ret > 0)
	{
		file_size = lseek(ret, 0L, SEEK_END);
		lseek(ret, 0L, SEEK_SET);

		if(file_size > 0)
		{
			file_content = mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, ret, 0);

			if(!file_content || file_content == (void*)(-1))
			{
				close(ret);
				return (void*)(-1);
			}

			*fd = ret;
			*length = file_size;
			return file_content;
		}
		else
		{
			return (void*)(-1);
		}
	}
	else
	{
		return (void*)(-1);
	}
}


int unmap_and_close_file(void **unmap_addr, unsigned long length, int fd)
{
	if(*unmap_addr)
	{
		munmap(*unmap_addr, length);
		*unmap_addr = NULL;
		close(fd);
	}

	return 0;
}


int init_factory_data(Factory_Test_Image *factory_test_image)
{
	if(factory_test_image)
	{
		factory_test_image->ecu_mode = NORMAL_WROK;
		factory_test_image->test_image = \
				open_and_mmap_file(FACTORY_TEST_PICTURE, &(factory_test_image->fd), &(factory_test_image->image_size));
		return 0;
	}
	else
	{
		return -1;
	}
}


int get_production_test_mode()
{
	int fd = 0;
	char read_buffer[8] = "";

	if((fd = open(FACTORY_TEST_MODE_FILE, O_RDONLY)) < 0)
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
		return atoi(read_buffer);
	}
}
