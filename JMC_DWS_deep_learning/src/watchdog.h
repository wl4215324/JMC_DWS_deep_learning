/*
 * watchdog.h
 *
 *  Created on: Jan 4, 2018
 *      Author: tony
 */

#ifndef WATCHDOG_H_
#define WATCHDOG_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <linux/watchdog.h>

#define  WATCHDOG_DEV  "/dev/watchdog"

extern int open_watchdog(void);

extern int shutdown_watchdog(void);

extern int feed_watchdog(void);

#endif /* WATCHDOG_H_ */
