/*
 * single_daemon_running.c
 *
 *  Created on: Dec 27, 2017
 *      Author: tony
 */


#include "single_daemon_running.h"


/*
 *
 */
static int lockfile(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;

    return(fcntl(fd, F_SETLK, &fl));
}

/*
 *
 */
int already_running(void)
{
    int fd;
    char buf[16];

    fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);

    if(fd < 0)
    {
        //syslog(LOG_ERR, "can't open %s: %s", LOCKFILE, strerror(errno));
    	fprintf(stderr, "can't open %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }

    if(lockfile(fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }

        //syslog(LOG_ERR, "can't lock %s: %s", LOCKFILE, strerror(errno));
        fprintf(stderr, "can't lock %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }

    ftruncate(fd, 0); // clear file content
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    return 0;
}



