/*
 * single_daemon_running.h
 *
 *  Created on: Dec 27, 2017
 *      Author: tony
 */

#ifndef SINGLE_DAEMON_RUNNING_H_
#define SINGLE_DAEMON_RUNNING_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>


#define LOCKFILE "/home/user/daemon.pid"

#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

int already_running(void);

#endif /* SINGLE_DAEMON_RUNNING_H_ */
