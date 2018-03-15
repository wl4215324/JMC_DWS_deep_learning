/*
 * software_version.h
 *
 *  Created on: Jan 3, 2018
 *      Author: tony
 */

#ifndef SOFTWARE_VERSION_H_
#define SOFTWARE_VERSION_H_

/*
 * Software version is composed with one byte, top four bits are major version, other end four bits are minor version,
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
 */

#define ARM_APP_SOFTWARE_VER_MAJ  1
#define ARM_APP_SOFTWARE_VER_MIN  2

#endif /* SOFTWARE_VERSION_H_ */
