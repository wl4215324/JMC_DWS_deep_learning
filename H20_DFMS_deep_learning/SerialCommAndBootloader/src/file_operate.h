/*
 * file_operate.h
 *
 *  Created on: Dec 5, 2018
 *      Author: tony
 */

#ifndef FILE_OPERATE_H_
#define FILE_OPERATE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>



extern int createMultiLevelDir(char* sPathName);

#endif /* FILE_OPERATE_H_ */
