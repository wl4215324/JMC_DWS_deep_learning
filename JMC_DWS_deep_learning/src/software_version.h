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
 * Software version is composed with one byte, top four bits are major version, bottom four bits are minor version,
 * which are showed with BCD codes.
 *
 * Version 1.0
 * Date: 2017/
 *
 *
 *
 * Version 1.1: 2018/02/07 10:30 am
 *           (1): Modify vibrating time of vibration motor when level 2 warning occurred,
 *           motor always vibrated until level 2 warning exited.
 *           (2): Modify frame rate of DDWS algorithm, which is set as 6 fps.
 *
 * Version 1.2: 2018/02/07 20:30 pm
 *           (1): DDWS algorithm modified by Xiaming, which made cover time close to 10 seconds.
 *
 *
 * Version 1.3: 2018/03/24 20:30 pm
 *           Following modifications are according to Function Specification V 2.6
 *           (1) Eye-closed time conversion is implemented, second(s) is directly sent to MCU via serial port communication.
 *           (2) When distraction warning comes out, 6 warning messages are continuously sent rather than only one message.
 *           (3) DDWS switch control logic is modified, one modification is saving switch status into xml file and reading
 *           updated value when program is executed. The other is specifying status of DDWS switch, 0 is off, 1 is on.
 *           (4) Driving behavior analysis is modified partially, cruise switch is newly added, which play the same role
 *           with accelerator pedal. If accelerator switch or cruise switch is active (on), one of driving behavior analysis
 *           is satisfied. In other words, the exit condition is both switches are off.
 *
 */

#define ARM_APP_SOFTWARE_VER_MAJ  1
#define ARM_APP_SOFTWARE_VER_MIN  10
#define SOFTWARE_VERSION_FILE  "/home/user/SoftVersion"

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
		sprintf(write_content, "V 1.4.%d.%d\n", ARM_APP_SOFTWARE_VER_MAJ, ARM_APP_SOFTWARE_VER_MIN);

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
