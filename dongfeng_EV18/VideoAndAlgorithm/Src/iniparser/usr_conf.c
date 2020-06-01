/*
 * usr_conf.c
 *
 *  Created on: Mar 11, 2020
 *      Author: tony
 */

#include <pthread.h>
#include "usr_conf.h"
#include "../../../ShmCommon/rtc_operations.h"
#include "../../../ShmCommon/applicfg.h"
#include "../CurlPost/curl_post.h"

char *ini_file_default_conten = "#\n"
        "# dongfeng_EV18 ini file\n"
        "#\n"
        "\n"
        "["PROOF_FILE_SEC"] \n"
        DMS_CUR_FILE_INDEX "= 0 \n"
        MONITOR_CUR_FILE_INDEX "= 0\n"
        "\n"
        "["PROOF_SERVER_SEC"] \n"
        TOKEN_URL_KEY " = "  TOKEN_URL "\n"
        WARN_PROOF_URL_KEY " = "  PROOF_URL "\n"
        USER_NAME_KEY " = " USER_NAME"\n"
        PASSWORD_KEY " = " PASSWORD"\n"
        VEH_VIN_CODE_KEY " = " VIN_CODE"\n"
        "\n"
        "["SYS_TTIME_SEC"] \n"
        DATE_TIME_KEY" = 2020-04-28 00:00:01 \n" ;


extern UrlParams *url_params;

static pthread_mutex_t  ini_file_rwlock;

/*
 * create a default ini file
 */
int create_default_ini_file(char *ini_conf_file)
{
    FILE *ini = NULL;

    if(!ini_conf_file)
    {
    	return -1;
    }

    //if ini file is not existent, then create a default ini file
    if(access(ini_conf_file, F_OK) < 0)
    {
    	ini = fopen(ini_conf_file, "w+");
    	fwrite(ini_file_default_conten, 1, strlen(ini_file_default_conten), ini);
        fflush(ini);
        fsync(fileno(ini));
        fclose(ini);
    }

    return 0;
}


/*
 *
 */
int init_conf_file(char *ini_conf_file)
{
	char key_str[64] = "";
	char temp_buf[64] = "";

	if(!ini_conf_file)
	{
		return -1;
	}

create_ini_file:
	/* if file ini exist, nothing to be done, else create a new default file */
	if(create_default_ini_file(ini_conf_file) < 0)
	{
		create_default_ini_file(INI_CONF_FILE_PATH);
		strcpy(ini_conf_file, INI_CONF_FILE_PATH);
	}

	pthread_mutex_init(&ini_file_rwlock, NULL);
	//sprintf(key_str, "%s:%s", SYS_TTIME_SEC, DATE_TIME_KEY);
	memset(temp_buf, '\0', sizeof(temp_buf));
	DEBUG_INFO(ini_conf_file: %s \n, ini_conf_file);
	read_inifile(ini_conf_file,  SYS_TTIME_SEC":"DATE_TIME_KEY, temp_buf);  //get system time
	printf("sys_date_time %s \n", temp_buf);

	if(!strlen(temp_buf))
	{
		DEBUG_INFO(\n);
		printf("ini file name:%s\n", ini_conf_file);
		remove(ini_conf_file);
		sleep(1);
		sync();
		goto create_ini_file;
	}
	else
	{
		struct tm local_date_time;
		time_t  sec_t = 0;
		struct timeval time_val;

		if(strptime(temp_buf, "%Y-%m-%d %H:%M:%S", &local_date_time))
		{
			sec_t = mktime(&local_date_time);
			time_val.tv_sec = sec_t;
			time_val.tv_usec = 0;
			sec_t = time(NULL);

			printf("time_val.tv_sec: %d sec_t: %d\n", time_val.tv_sec, sec_t);

			if(sec_t < time_val.tv_sec)  //if time stored in file is later than firware time
			{
				set_datetime_according_str(temp_buf); //update system date time
			}
			else //if time stored in file is ahead of firware time
			{
				get_datetime_according_fmt(temp_buf);
				printf("temp_buf:%s\n",  temp_buf);
				change_inifile(ini_conf_file, SYS_TTIME_SEC":"DATE_TIME_KEY, temp_buf);
			}
		}
	}

	if(!url_params)
		url_params = (UrlParams*)malloc(sizeof(UrlParams));

	if(url_params)
	{
		memset(temp_buf, '\0', sizeof(temp_buf));
		read_inifile(ini_conf_file,  PROOF_SERVER_SEC":"USER_NAME_KEY, temp_buf); //get user name
		if(strlen(temp_buf))
		{
			strcpy(url_params->usr_name, temp_buf);
		}

		memset(temp_buf, '\0', sizeof(temp_buf));
		read_inifile(ini_conf_file,  PROOF_SERVER_SEC":"PASSWORD_KEY, temp_buf);  //get password
		if(strlen(temp_buf))
		{
			strcpy(url_params->password, temp_buf);
		}

		memset(temp_buf, '\0', sizeof(temp_buf));
		read_inifile(ini_conf_file, PROOF_SERVER_SEC":"VEH_VIN_CODE_KEY, temp_buf);  //get vin code
		if(strlen(temp_buf))
		{
			strcpy(url_params->vin_code, temp_buf);
		}
	}

    return 0;
}


/*
 * function describtion: update value of specified key
 */
int read_inifile(const char *ini_file_name, const char *key, char * ret_str)
{
	char *temp = NULL;
	int ret_val = -1;
    dictionary * dict = NULL;

    pthread_mutex_lock(&ini_file_rwlock);
    //Parse an ini file and return an allocated dictionary object
	if(ret_str)
	{
	    if(!(dict = iniparser_load(ini_file_name)))
	    {
	    	goto read_inifile_exit;
	    }

	    temp = iniparser_getstring(dict, key, NULL);

	    if(temp)
	    {
	    	strcpy(ret_str, temp);
	    	ret_val = 0;
	    }
	}

read_inifile_exit:
    if(dict)
    	iniparser_freedict(dict);
    pthread_mutex_unlock(&ini_file_rwlock);
    return ret_val;
}



/*
 * function describtion: update value of specified key
 */
int change_inifile(const char *ini_file_name, const char * key, const char * val)
{
    int ret_val = -1;
    dictionary * dict = NULL;

    pthread_mutex_lock(&ini_file_rwlock);
    if(!(dict = iniparser_load(ini_file_name)))
    {
    	ret_val =  -1;
    	goto fun_exit;
    }

    if(iniparser_set(dict, key, val))
    {
    	ret_val = -1;
    	goto fun_exit;
    }

#if 1
    FILE * f = fopen(ini_file_name, "w");
    iniparser_dump_ini(dict, f);
    fsync(fileno(f));
    fclose(f);
    ret_val = 0;
#endif

fun_exit:
    if(dict)
    	iniparser_freedict(dict);
    pthread_mutex_unlock(&ini_file_rwlock);
    return ret_val;
}



/*lock_set函数*/
void lock_set(int fd, int type)
{
    struct flock lock;
    lock.l_whence = SEEK_SET; //赋值lock结构体
    lock.l_start = 0;
    lock.l_len = 0;

    while (1)
    {
        lock.l_type = type;
        /*根据不同的type值给文件上锁或解锁*/
        if ((fcntl(fd, F_SETLK, &lock)) == 0)
        {
            if (lock.l_type == F_RDLCK)
                printf("read lock set by %d\n", getpid());
            else if (lock.l_type == F_WRLCK)
                printf("write lock set by %d\n", getpid());
            else if (lock.l_type == F_UNLCK)
                printf("release lock by %d\n", getpid());
            return;
        }

        /*判断文件是否可以上锁*/
        fcntl(fd, F_GETLK, &lock);
        /*判断文件不能上锁的原因*/
        if (lock.l_type != F_UNLCK)
        {
            /*该文件已有写入锁*/
            if (lock.l_type == F_RDLCK)
                printf("read lock already set by %d\n", lock.l_pid);
            /*该文件已有读取锁*/
            else if (lock.l_type == F_WRLCK)
                printf("write lock already set by %d\n", lock.l_pid);
            getchar();
        }
    }
}
