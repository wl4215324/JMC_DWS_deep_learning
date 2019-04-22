/*
 * start_app.c
 *
 *  Created on: Apr 18, 2019
 *      Author: tony
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <wait.h>

#define  APP_CNT  2

char *app_name[APP_CNT] = {
		"/extp/SerialCommAndBootloader",
		"/extp/VideoAndAlgorithm",
};

int pidGrp[APP_CNT] = {0};


/*
 *  check program is running
 *  *proc: program name
 *
 */
bool isRunning(char *proc)
{
    bool ret = false;
    FILE *fstream = NULL;
    char buf[128] = {0};
    int pid = 0;

    sprintf(buf, "ps -ef| grep %s| grep -v grep| awk '{print $1}'", proc);  //$1 first column for pid

    if( NULL == (fstream = popen(buf, "r")) )
    {
        fprintf(stderr,"execute command failed: %s", strerror(errno));
        return false;
    }

    while(NULL != fgets(buf, sizeof(buf), fstream))
    {
        if (strlen(buf) > 0)
        {
        	pid = atoi(buf); //pid of program
        }
        else
        {
        	ret = false;
        }
    }

    pclose(fstream);
    fstream = NULL;

    if((pid == 1) || (pid <= 0)) // pid =1 for init
    {
    	ret = false;
    }
    else
    {
    	ret = true;
    }

    return ret;
}


int main(int argc, char* argv[])
{
    int i  = 0;
    char buf[128] = {0};
	pid_t parent_pid, rePid;
	parent_pid = getpid();

	char *path[1];
	path[0] = NULL;

    for(i=0; i<APP_CNT; i++)
    {
    	if(isRunning(app_name[i]))  //app_name[i] is live
    	{
    		sprintf(buf, "killall %s", app_name[i]);  //kill living program
    		system(buf);
    		sleep(1);
    	}
    }

	system("ipcs -m | awk '$2 ~/[0-9]+/ {print $2}' | while read s; do ipcrm -m $s; done;");
	system("ipcs -q | awk '$2 ~/[0-9]+/ {print $2}' | while read s; do ipcrm -q $s; done;");

	for(i = 0; i < APP_CNT; i++)
	{
		if(parent_pid == getpid())
		{
			pidGrp[i] = fork();
		}

		// 循环启动新的进程
		if (pidGrp[i] == 0)  //child process
		{
			printf("%s\n", app_name[i]);
			execv(app_name[i], path);
			sleep(1);
		}
	}

	// 如果有其他进程异常结束，则重新启动该进程
	int reStartFlag = 0;

	while (1)
	{
		// 等待异常结束的进程
		rePid = wait(NULL);

		for(i = 0; i < APP_CNT; i++)
		{
			if (rePid == pidGrp[i])
			{
				reStartFlag = 0x01;
				break;
			}
		}

		printf("rePid=%d\n", rePid);

//		if(rePid<0)
//		{
//	        retpid();
//		}

		if (reStartFlag == 0x01)
		{
			// 重新启动新的进程
			if (parent_pid == getpid())
			{
				pidGrp[i] = fork();
			}

			if (pidGrp[i] == 0)
			{
				printf("%s\n", app_name[i]);
				execv(app_name[i], path);
				sleep(2);
			}

			reStartFlag = 0;
		}

		sleep(2);
	}

	return 0;
}
