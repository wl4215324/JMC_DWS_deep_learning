/*
 * production_test.h
 *
 *  Created on: Nov 27, 2019
 *      Author: tony
 */

#ifndef PRODUCTION_TEST_H_
#define PRODUCTION_TEST_H_

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>


#define  FACTORY_TEST_PICTURE  "/extp/yawn_720p.yuv"
#define  FACTORY_TEST_MODE_FILE  "/extp/work_mode"

typedef int(*is_test_mode)(int argv);

typedef enum {
	NORMAL_WROK = 0,
	FACTORY_TEST,
} ECU_Mode;


typedef struct {
	unsigned char *test_image;
	unsigned long image_size;
	int fd;
	ECU_Mode ecu_mode;
} Factory_Test_Image;


int init_factory_data(Factory_Test_Image *factory_test_image);

int get_production_test_mode();




#endif /* PRODUCTION_TEST_H_ */
