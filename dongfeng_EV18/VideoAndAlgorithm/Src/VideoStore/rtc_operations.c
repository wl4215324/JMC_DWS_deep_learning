/*
 * time_process.c
 *
 *  Created on: Nov 15, 2019
 *      Author: tony
 */


#include "rtc_operations.h"

int rtcSetTime(const struct tm *tm_time);

int setDateTime(struct tm* ptm)
{
	time_t timep;
	struct timeval tv;

	timep = mktime(ptm);
	tv.tv_sec = timep;
	tv.tv_usec = 0;

	if(settimeofday(&tv, NULL) < 0)
	{
		printf("Set system date and time error.\n");
		return -1;
	}

	time_t t = time(NULL);
	struct tm *local = localtime(&t);

	printf("datetime->tm_year=%d \n",local->tm_year+1900);
	printf("datetime->tm_mon=%d \n",local->tm_mon);
	printf("datetime->tm_mday=%d \n",local->tm_mday);
	printf("datetime->tm_hour=%d \n",local->tm_hour);
	printf("datetime->tm_min=%d \n",local->tm_min);
	printf("datetime->tm_sec=%d \n",local->tm_sec);

	rtcSetTime(local);
	return 0;
}


void resetDateTime(void)
{
	struct tm tm;

	tm.tm_year = (2014-1900);
	tm.tm_mon = (11-1);
	tm.tm_mday = 1;
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	tm.tm_wday = 0;
	tm.tm_yday = 0;
	tm.tm_isdst = 0;

	setDateTime(&tm);
}


time_t getDateTime(struct tm **local_time)
{
	time_t timer;
	timer = time(NULL);
    *local_time = localtime(&timer);
	return timer;
}


#define  RTC_SET_TIME  10
int rtcSetTime(const struct tm *tm_time)
{
    int rtc_handle = -1;
	int ret = 0;
	struct rtc_time rtc_tm;

	if(tm_time == NULL)
	{
	    return  -1;
	}

    rtc_handle = open("/dev/rtc0", O_RDWR);
	if (rtc_handle < 0)
	{
		printf("open /dev/rtc0 fail \n");
		return  -1;
	}

	memset(&rtc_tm, 0, sizeof(rtc_tm));
	rtc_tm.tm_sec   = tm_time->tm_sec;
	rtc_tm.tm_min   = tm_time->tm_min;
	rtc_tm.tm_hour  = tm_time->tm_hour;
	rtc_tm.tm_mday  = tm_time->tm_mday;
	rtc_tm.tm_mon   = tm_time->tm_mon;
	rtc_tm.tm_year  = tm_time->tm_year;
	rtc_tm.tm_wday  = tm_time->tm_wday;
	rtc_tm.tm_yday  = tm_time->tm_yday;
	rtc_tm.tm_isdst = tm_time->tm_isdst;

	ret = ioctl(rtc_handle, RTC_SET_TIME, &rtc_tm);
    if (ret < 0)
    {
        printf("rtcSetTime fail\n");
        close(rtc_handle);
        return -1;
    }

	printf("rtc_set_time ok\n");
	close(rtc_handle);
	return 0;
}


#define  RTC_RD_TIME  11
int rtcGetTime(struct tm *tm_time)
{
    int rtc_handle;
	int ret = 0;
	struct rtc_time rtc_tm;

    rtc_handle = open("/dev/rtc0", O_RDWR);
	if (rtc_handle < 0)
	{
		printf("open /dev/rtc0 fail\n");
		return  -1;
	}

	memset(&rtc_tm,0,sizeof(rtc_tm));
	ret = ioctl(rtc_handle, RTC_RD_TIME, &rtc_tm);
	if(ret < 0)
	{
		printf("rtcGetTime fail\n");
		close(rtc_handle);
	    return -1;
	}

	tm_time->tm_sec		= rtc_tm.tm_sec;
	tm_time->tm_min		= rtc_tm.tm_min;
	tm_time->tm_hour	= rtc_tm.tm_hour;
	tm_time->tm_mday	= rtc_tm.tm_mday;
	tm_time->tm_mon		= rtc_tm.tm_mon;
	tm_time->tm_year	= rtc_tm.tm_year;
	tm_time->tm_wday	= rtc_tm.tm_wday;
	tm_time->tm_yday	= rtc_tm.tm_yday;
	tm_time->tm_isdst	= rtc_tm.tm_isdst;
	close(rtc_handle);
	return 0;
}

