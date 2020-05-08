/*
 * usr_conf.h
 *
 *  Created on: Mar 11, 2020
 *      Author: tony
 */

#ifndef USR_CONF_H_
#define USR_CONF_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "iniparser.h"

#define  INI_CONF_FILE_PATH  "/extp/dongfeng_EV18_conf.ini"

#define  PROOF_FILE_SEC  "proof_file_save"
#define  DMS_CUR_FILE_INDEX  "dsm_cur_file_index"
#define  MONITOR_CUR_FILE_INDEX "monitor_cur_file_index"

#define  PROOF_SERVER_SEC  "proof_server_ip"
#define  TOKEN_URL_KEY  "token_url"
#define  WARN_PROOF_URL_KEY  "warn_proof_url"
#define  USER_NAME_KEY  "user_name"
#define  PASSWORD_KEY  "password"
#define  VEH_VIN_CODE_KEY  "vin_code"


#define  SYS_TTIME_SEC  "sys_time"
#define  DATE_TIME_KEY  "date_time"

int init_conf_file(char *ini_conf_file);
int change_inifile(const char *ini_file_name, const char * entry, const char * val);
int read_inifile(const char *ini_file_name, const char * entry, char * ret_str);

#endif /* USR_CONF_H_ */
