/*
 * software_version.h
 *
 *  Created on: Jan 3, 2018
 *      Author: tony
 */

#ifndef SOFTWARE_VERSION_H_
#define SOFTWARE_VERSION_H_

/*
 * Software version is composed with one byte, top four bits are major version, bottom four bits are minor version,
 * which are showed with BCD codes.
 *
 * Version 1.0
 * Date: created on 2018/07/11
 * Author: Tony Wang
 * Explanation:
 *             (1) This project is established because of demo project demand from Hongyan auto.
 *             (2) It derives from project TMXJH625-E05, but fatigue warnings are triggered without any external
 *             limited condition.
 *
 *
 *

 *
 *
 */

#define ARM_APP_SOFTWARE_VER_MAJ  1
#define ARM_APP_SOFTWARE_VER_MIN  0

#endif /* SOFTWARE_VERSION_H_ */
