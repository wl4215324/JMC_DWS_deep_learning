/*
 * usr_conf.c
 *
 *  Created on: Mar 11, 2020
 *      Author: tony
 */

#include "usr_conf.h"
#include "../../../ShmCommon/rtc_operations.h"
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

	/* if file ini exist, nothing to be done, else create a new default file */
	if(create_default_ini_file(ini_conf_file) < 0)
	{
		create_default_ini_file(INI_CONF_FILE_PATH);
		strcpy(ini_conf_file, INI_CONF_FILE_PATH);
	}

	sprintf(key_str, "%s:%s", SYS_TTIME_SEC, DATE_TIME_KEY);
	read_inifile(ini_conf_file, key_str, temp_buf);  //get system time
	printf("sys_date_time %s \n", temp_buf);

	if(strlen(temp_buf))
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
    //Parse an ini file and return an allocated dictionary object
	if(ret_str)
	{
	    dictionary * dict = NULL;

	    if(!(dict = iniparser_load(ini_file_name)))
	    {
	    	return -1;
	    }

	    temp = iniparser_getstring(dict, key, "null");

	    if(temp)
	    {
	    	strcpy(ret_str, temp);
	    }

	    iniparser_freedict(dict);
	    return 0;
	}
	else
	{
		return -1;
	}
}

/*
 * function describtion: update value of specified key
 */
int change_inifile(const char *ini_file_name, const char * key, const char * val)
{
    int re = 0;
    dictionary * dict = NULL;

    if(!(dict = iniparser_load(ini_file_name)))
    {
    	return -1;
    }

    if(iniparser_set(dict, key, val))
    	return -1;
#if 1
    FILE * f = fopen(ini_file_name, "w");
    iniparser_dump_ini(dict, f);
    fsync(fileno(f));
    fclose(f);
#endif

    iniparser_freedict(dict);
    return 0;
}




