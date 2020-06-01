/*
 * curl_post.h
 *
 *  Created on: Apr 20, 2020
 *      Author: tony
 */

#ifndef CURL_POST_H_
#define CURL_POST_H_

#include <stdlib.h>
#include <stdio.h>

#define  TOKEN_URL  "https://test-api-dms.burnish.cn/auth/api-token-auth/"
#define  PROOF_URL  "https://test-api-dms.burnish.cn/monitor/upload-alert/"
#define  USER_NAME  "admin"
#define  PASSWORD   "rootroot"
#define  VIN_CODE   "dsm_0123456789"

typedef struct {
	char usr_name[16];
	char password[16];
	char vin_code[32];
} UrlParams;

int get_token(char *username, char *password, char**ret_token);

int post_proof(char *token, char *vin_code, char *violation_info, char *date_time, \
		char *location, char *descib, char *proof_file);

int upload_warn_proof(char *username, char *password, char *vin_code, char *violation_info, \
		              char *date_time, char *location, char *descib, char *proof_file);
#endif /* CURL_POST_H_ */
