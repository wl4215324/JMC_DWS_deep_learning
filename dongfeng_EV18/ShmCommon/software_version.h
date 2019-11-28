/*
 * software_version.h
 *
 *  Created on: Jan 3, 2018
 *      Author: tony
 */

#ifndef SOFTWARE_VERSION_H_
#define SOFTWARE_VERSION_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 * Software version is composed with 4 fields. First 2 fields are prepared for MCU software, and left 2 fields are for arm software.
 * Head of 2 fields is major version, the other is minor number.

 */

/* following version information is prepared for production */
#define MCU_APP_SOFTWARE_VER_MAJ  1
#define MCU_APP_SOFTWARE_VER_MIN  0

#define ARM_APP_SOFTWARE_VER_MAJ  1
#define ARM_APP_SOFTWARE_VER_MIN  0


#define SOFTWARE_VERSION_FILE  "/extp/SoftVersion"

static inline int write_software_info()
{
	int fd;
	int ret = 0;
	char write_content[16] = "";

	fd = open(SOFTWARE_VERSION_FILE, O_RDWR|O_CREAT, 777);

	if (fd < 0)
	{
		return -1;
	}
	else
	{
		sprintf(write_content, "V %d.%d.%d.%d\n", MCU_APP_SOFTWARE_VER_MAJ, MCU_APP_SOFTWARE_VER_MIN,\
				ARM_APP_SOFTWARE_VER_MAJ, ARM_APP_SOFTWARE_VER_MIN);

		if((ret = write(fd, write_content, strlen(write_content))) < 0)
		{
			close(fd);
			return -1;
		}
		else
		{
			close(fd);
			return ret;
		}
	}
}

#endif /* SOFTWARE_VERSION_H_ */
