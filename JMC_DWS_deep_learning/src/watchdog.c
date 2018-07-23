/*
 * watchdog.c
 *
 *  Created on: Jan 4, 2018
 *      Author: tony
 */

#include "watchdog.h"

int wdt_fd = -1;

int open_watchdog(void)
{
	if ((wdt_fd = open(WATCHDOG_DEV, O_WRONLY)) < 0)
	{
		perror("open watchdog error");
		return -1;
	}
	else
	{
		return 1;
	}
}

int shutdown_watchdog(void)
{
    if (wdt_fd != -1)
    {
        write(wdt_fd, "V", 1);
        close(wdt_fd);
        wdt_fd = -1;
        return 1;
    }
    else
    {
    	return -1;
    }
}

int feed_watchdog(void)
{
	ioctl(wdt_fd, WDIOC_KEEPALIVE, 0);
	return 1;

	/*
    if (wdt_fd != -1)
    {
    	write(wdt_fd, "a", 1);
    	return 1;
    }
    else
    {
    	return -1;
    }
    */
}
