/*
 * curl_post.c
 *
 *  Created on: Apr 20, 2020
 *      Author: tony
 */


#include "curl_post.h"
#include "./include/curl/curl.h"
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

UrlParams *url_params = NULL;
pthread_mutex_t upload_proof_lock = PTHREAD_MUTEX_INITIALIZER;

int reveived_callback( char *buffer, size_t size, size_t nmemb, char *userdata)
{
	char *p = strstr(buffer, "\"token\":\"");

	if(!p)
		return -1;

	p += strlen("\"token\":\"");
	strncpy(userdata, p, strlen(p)-2);
	printf("reveived_callback token: %s\n", userdata);
	return strlen(userdata);
}


int get_token(char *username, char *password, char**ret_token)
{
	CURL *curl;
	CURLcode res;
	int ret = -1;

	// no account or password
	if(!username || !password || !strlen(username) || !strlen(password))
	{
		return -1;
	}

	if(*ret_token)
	{
		free(*ret_token);
		*ret_token = NULL;
	}

	if((*ret_token = (char*)malloc(256)) == NULL)
	{
		goto error_exit;
	}

	memset(*ret_token, '\0', 256);
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	struct curl_slist *headerlist = NULL;

	/*增加post请求参数，一般你的参数都在这里面进行设置*/
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "username",
                 CURLFORM_COPYCONTENTS, username,
                 CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "password",
                 CURLFORM_COPYCONTENTS, password,
                 CURLFORM_END);
    headerlist = curl_slist_append(headerlist, "Content-Type: multipart/form-data");
	curl = curl_easy_init();
	if (!curl)
	{
		fprintf(stderr,"curl init failed\n");
		goto error_exit;
	}

	curl_easy_setopt(curl,CURLOPT_URL, TOKEN_URL);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist); // fill in request headers
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);  //fill in request body
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reveived_callback); //对返回的数据进行操作的函数地址
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, *ret_token); //这是write_data的第四个参数值
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
	{
		ret = -1;
		switch(res)
		{
			case CURLE_UNSUPPORTED_PROTOCOL:
				fprintf(stderr,"不支持的协议,由URL的头部指定\n");
				break;

			case CURLE_COULDNT_CONNECT:
				fprintf(stderr,"不能连接到remote主机或者代理\n");
				break;

			case CURLE_HTTP_RETURNED_ERROR:
				fprintf(stderr,"http返回错误\n");
				break;

			case CURLE_READ_ERROR:
				fprintf(stderr,"读本地文件错误\n");
				break;

			case CURLE_WRITE_ERROR: //failed writing received data to disk/application
				ret = 0;
				break;

			default:
				fprintf(stderr,"返回值:%d\n",res);
				break;
		}
	}
	else
	{
		ret = 0;
		printf("token: %s\n", *ret_token);
	}

	curl_easy_cleanup(curl);
	return ret;

error_exit:
    if(*ret_token != NULL)
    {
    	free(*ret_token);
    	*ret_token = NULL;
    }
    return -1;
}

int post_proof_callback(char *buffer, size_t size, size_t nmemb, char *userdata)
{
	printf("post proof response: %s\n", buffer);
	return 0;
}

/*
 * function: send POST request to upload proof file
 */
int post_proof(char *token, char *vin_code, char *violation_info, char *date_time, \
		char *location, char *descib, char *proof_file)
{

	if(!token || !vin_code || !violation_info || !date_time || !location || !descib || !proof_file)
	{
		return -1;
	}

	if(!strlen(token) || !strlen(vin_code) || !strlen(violation_info) || !strlen(date_time) || \
	   !strlen(location) || !strlen(descib) || !strlen(proof_file))
	{
		return -1;
	}

	char temp[256] = "";
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	struct curl_slist *headerlist = NULL;
	int ret = -1;

	/*增加post请求参数，一般参数都在这里面进行设置*/
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "vin",
                 CURLFORM_COPYCONTENTS, vin_code,
                 CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "violation",
                 CURLFORM_COPYCONTENTS, violation_info,
                 CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "timestamp",
                 CURLFORM_COPYCONTENTS, date_time,
                 CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "address",
                 CURLFORM_COPYCONTENTS, location,
                 CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "description",
                 CURLFORM_COPYCONTENTS, descib,
                 CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "image",
                 CURLFORM_FILE, proof_file,
                 CURLFORM_END);

    sprintf(temp, "Authorization: JWT %s", token);
    headerlist = curl_slist_append(headerlist, temp);
    headerlist = curl_slist_append(headerlist, "Content-Type: multipart/form-data");

	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if (!curl)
	{
		fprintf(stderr,"curl init failed\n");
		return -1;
	}

	curl_easy_setopt(curl,CURLOPT_URL, PROOF_URL);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist); // fill in request headers
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);  //fill in request body
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, post_proof_callback); //对返回的数据进行操作的函数地址
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, temp); //这是write_data的第四个参数值
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	res = curl_easy_perform(curl);

	printf("post_proof: curl_easy_perform");

	if (res != CURLE_OK)
	{
		switch(res)
		{
			case CURLE_UNSUPPORTED_PROTOCOL:
				fprintf(stderr,"不支持的协议,由URL的头部指定\n");
				break;

			case CURLE_COULDNT_CONNECT:
				fprintf(stderr,"不能连接到remote主机或者代理\n");
				break;

			case CURLE_HTTP_RETURNED_ERROR:
				fprintf(stderr,"http返回错误\n");
				break;

			case CURLE_READ_ERROR:
				fprintf(stderr,"读本地文件错误\n");
				break;

			default:
				fprintf(stderr,"返回值:%d\n",res);
				break;
		}

		ret = -1;
	}
	else
	{
		ret = 0;
	}

	curl_easy_cleanup(curl);
	return ret;
}


int upload_warn_proof(char *username, char *password, char *vin_code, char *violation_info, \
		              char *date_time, char *location, char *descib, char *proof_file)
{
	int ret = -1;
	char *token = NULL;

	pthread_mutex_lock(&upload_proof_lock);
	if(!get_token(username, password, &token)) //get token successfully
	{
		if(!post_proof(token, vin_code, violation_info, date_time, \
		               location, descib, proof_file))  //post file successfully
		{
			ret = 0;
		}
		else  //post file error
		{
			ret = -1;
		}
	}
	else //get token error
	{
		ret = -1;
	}

	if(token)
		free(token);

	pthread_mutex_unlock(&upload_proof_lock);
	return ret;
}


