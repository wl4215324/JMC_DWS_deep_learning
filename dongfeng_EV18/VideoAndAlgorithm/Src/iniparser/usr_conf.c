/*
 * usr_conf.c
 *
 *  Created on: Mar 11, 2020
 *      Author: tony
 */

#include "usr_conf.h"
#include "../VideoStore/rtc_operations.h"

char *ini_file_default_conten = "#\n"
        "# dongfeng_EV18 ini file\n"
        "#\n"
        "\n"
        PROOF_FILE_SEC "\n"
        "cur_file_index = 0 ;\n"
        "\n"
        PROOF_SERVER_SEC"\n"
        "host_name = www.baidu.com ;\n"
        "port      = 80 ;\n"
        "\n"
        SYS_TTIME_SEC"\n"
        "date_time = 2020-03-12 00:00:00; \n" ;


void create_default_ini_file(char *ini_conf_file)
{
    FILE *ini ;

    if(!(ini=fopen(ini_conf_file, "r"))) //if file ini alread not exist
    {
    	ini = fopen(ini_conf_file, "w+");
    	fwrite(ini_file_default_conten, 1, strlen(ini_file_default_conten), ini);
        fflush(ini);
        fsync(fileno(ini));
        fclose(ini);
    }
    else
    {
    	fclose(ini);
    }
}


int init_conf_file(char *ini_conf_file)
{
	dictionary  *ini_file = NULL;
	char *sys_date_time = NULL;

	if(!ini_conf_file)
	{
		return -1;
	}

	/* if file ini exist, nothing to be done, else create a new default file */
	create_default_ini_file(ini_conf_file);

	ini_file = iniparser_load(ini_conf_file);
	sys_date_time = iniparser_getstring(ini_file, "sys_time:date_time", "null");

	printf("sys_date_time %s \n", sys_date_time);

	if(sys_date_time)
	{
		set_datetime_according_str(sys_date_time);
	}

	iniparser_freedict(ini_file);
    return 0;
}


int read_inifile(const char *ini_file_name, const char * entry, char * ret_str)
{
    //Parse an ini file and return an allocated dictionary object
	if(ret_str)
	{
	    dictionary * dict = NULL;

	    if(!(dict = iniparser_load(ini_file_name)))
	    {
	    	return -1;
	    }

	    strcpy(ret_str, '\0');
	    strcat(ret_str, iniparser_getstring(dict, entry, "null"));
	    iniparser_freedict(dict);
	    return 0;
	}
	else
	{
		return -1;
	}
}


int change_inifile(const char *ini_file_name, const char * entry, const char * val)
{
    int re = 0;
    dictionary * dict = NULL;

    if(!(dict = iniparser_load(ini_file_name)))
    {
    	return -1;
    }

    if(iniparser_set(dict, entry, val))
    	return -1;
#if 1
    FILE * f = fopen(ini_file_name, "w");
    iniparser_dump_ini(dict, f);
    fclose(f);
#endif

    iniparser_freedict(dict);
    return 0;
}




