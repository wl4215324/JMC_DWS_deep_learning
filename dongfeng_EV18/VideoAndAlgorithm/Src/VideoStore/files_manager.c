/*
 * files_manager.c
 *
 *  Created on: Nov 13, 2019
 *      Author: tony
 */

#include "files_manager.h"
#include "file_operation.h"
#include "rtc_operations.h"

char g_PicPath[MAX_SUPPORT_CAM][PIC_NAME_LEN] = { };   //mod lss
char g_NamePic[MAX_SUPPORT_CAM][CM_MAX_PICTURE_NUM][PIC_NAME_LEN] = { };

char *pVideoPath[MAX_SUPPORT_CAM] = {
	"warn_video",
	"rearVideo1",
	"leftVideo2",
	"rightVideo3",
	"frontVideoCvbs4",
	"rearVideoCvbs5",
	"leftVideoCvbs6",
	"rightVideoCvbs7",
	"4ChVideoCvbs8",
	"4ChVideoCsi9",
};

char *pPicPath[MAX_SUPPORT_CAM] = {
	"frontPicture0",
	"rearPicture1",
	"leftPicture2",
	"rightPicture3",
	"frontPictureCvbs4",
	"rearPictureCvbs5",
	"leftPictureCvbs6",
	"rightPictureCvbs7",
	"4ChPictureCvbs8",
	"4ChPictureCsi9",
};

char *pSuffix[MAX_SUPPORT_CAM] = {
	"warn", "rear", "left", "right",
	"frontCvbs", "rearCvbs", "leftCvbs", "rightCvbs",
	"4ChCvbs", "4ChCsi"
};


file_status_t gFilemanger;

char **ppSuffix = pSuffix;

unsigned int mDuration = 10;
unsigned int bit_rate = 5 * 1024 * 1024;
unsigned int gAFileOccupySizePerMinMb = 64;




/**************************************
Function:
Description:
***************************************/
file_status_t *initFileListDir(char *dirname, unsigned char camera_index, \
		unsigned char bit_rate_Mb, unsigned short duration_s)
{
	DIR *dir_ptr;
	struct dirent *direntp;
	int i;
	int idx;
	char tmpbuf[256];
	char rmbuf[256];
	char real_path[256];
	char camera_path[256];
	int len = 0;

	if (!dirname || (len = strlen(dirname)) < 2 || (bit_rate_Mb <= 0) || (duration_s <= 0))
	{
		return (file_status_t*)(-1);
	}

	unsigned char l32_CameraId = 0;
	l32_CameraId = (camera_index >= MAX_SUPPORT_CAM) ? 0: camera_index;
	strcpy(tmpbuf, dirname);

	for (i = 0; i < len; i++)
	{
		if (tmpbuf[len - 1 - i] == '/')
		{
			tmpbuf[len - 1 - i] = 0;
		}
		else
		{
			break;
		}
	}

	int rmret = 0;
	rmret = readlink(tmpbuf, real_path, sizeof(real_path));
	if (rmret < 0)
	{
		strcpy(real_path, tmpbuf);
	}

	file_status_t *file_manager = (file_status_t*) malloc(sizeof(file_status_t));
	if(!file_manager)
	{
		goto init_files_dir_error;
	}

	file_manager->cur_max_filenum = 0;
	file_manager->cur_dir_file_num = 0;
	file_manager->cur_file_idx = 0;
	file_manager->file_size_MB = 0;
	file_manager->file_dir_status = SDCARD_NOT_MOUNT;
	memset(file_manager->cur_filedir, 0 ,sizeof(file_manager->cur_filedir));

	for(i=0; i<CM_MAX_RECORDFILE_NUM; i++)
	{
		memset(file_manager->cur_filesname+i, 0, CM_MAX_FILE_LEN);
	}

	if (isMounted(real_path))
    {
		printf("-------sdmmc------mounted \n");
	}
	else
	{
		printf("-------sdmmc------unmounted \n");
		return (file_status_t*)(-1);
	}

	file_manager->file_dir_status = FILE_DIR_NOT_CREATE;
	memset(camera_path, 0, sizeof(camera_path));
	sprintf(camera_path, "%s/%s", real_path, pVideoPath[l32_CameraId]);
	printf("camera_path: %s\n", camera_path);

	if (createdir(camera_path) != 0)
	{
		printf("create camera_path dir %s fail", camera_path);
		return (file_status_t*)(-1);
	}

	file_manager->file_dir_status = FILE_DIR_NORMAL;
	strcpy(file_manager->cur_filedir, camera_path);
	unsigned long long totMB  = totalSize(real_path);  //MB
	/* round up to an integer according to fomula (M+N-1)/N */
	file_manager->file_size_MB = ((duration_s * bit_rate_Mb + 7) >> 3);
	file_manager->cur_max_filenum = (totMB - RESERVED_SIZE + CM_MAX_RECORDFILE_NUM - 1) / CM_MAX_RECORDFILE_NUM;

	if(file_manager->cur_max_filenum  > CM_MAX_RECORDFILE_NUM)
	{
		file_manager->cur_max_filenum = CM_MAX_RECORDFILE_NUM;
	}

	if((dir_ptr = opendir(camera_path)))
	{
		while ((direntp = readdir(dir_ptr)) != NULL)
		{
			if (strstr(direntp->d_name, CM_SAVE_FILE_MARK) != NULL)  //skip file "save"
				continue;
			if (strstr(direntp->d_name, CM_LOCK_FILE_MARK) != NULL)  //skip file "lock"
				continue;

			for (i = 0; i < strlen(direntp->d_name); i++)  //file name not empty
			{
				if (direntp->d_name[i] == '-')  //file name 1-20100101_001013_front
				{
					memset(tmpbuf, 0, sizeof(tmpbuf));
					memcpy(tmpbuf, direntp->d_name, i);
					idx = atoi(tmpbuf);

					if (idx >= 0 && idx < file_manager->cur_max_filenum)  //if index is valid
					{
						/* if cur_filename[idx] is empty, then copy direntp->d_name to cur_filename[idx]*/
						if (strlen(file_manager->cur_filesname[idx]) == 0)
						{
							strcat(file_manager->cur_filesname[idx], direntp->d_name);
							file_manager->cur_dir_file_num++;
						}
						else  //if cur_filename[idx] was assigned yet, maybe old file need to be removed
						{
							/* if rmbuf is not substring of cur_filename[idx], then remove file cur_filename[idx] */
							if (strcasecmp(file_manager->cur_filesname[idx], direntp->d_name) == 0)
							{
								printf("find duplicate  so rm old %s idx=%d  name=%s \n", file_manager->cur_filesname[idx], idx, direntp->d_name);
								memset(rmbuf, 0 ,sizeof(rmbuf));
								sprintf(rmbuf, "%s/%s", file_manager->cur_filedir, direntp->d_name);
								rmret = remove(rmbuf);

//								strcpy(p->cur_filesname[idx], camera_path);
//								strcat(p->cur_filesname[idx], "/");
								strcat(file_manager->cur_filesname[idx], direntp->d_name);
							}
						}
					}
					else //if index is invalid
					{
						;
//						memset(rmbuf, 0 ,sizeof(rmbuf));
//						sprintf(rmbuf, "%s/%s", file_manager->cur_filedir, direntp->d_name);
//						rmret = remove(rmbuf);
					}
				}
			}

			if(i >= strlen(direntp->d_name))
			{
				memset(rmbuf, 0 ,sizeof(rmbuf));
				sprintf(rmbuf, "%s/%s", file_manager->cur_filedir, direntp->d_name);
				rmret = remove(rmbuf);
			}
		}

		closedir(dir_ptr);
		sync();
	}

	return file_manager;

init_files_dir_error:
	return (file_status_t*)(-1);
}



int genfilename(char *name, file_status_t *file_manager)
{
	char rec_filename[256], new_file_name[256];
	int rmidx = 0, reidx = 0;
	int l32_CameraId = 0;

	if (!name || !file_manager || (file_manager->cur_max_filenum == 0))
		return -1;

	//p->cur_file_idx = config_get_camfileidx(0); // get "cur-fileidx" value into file /etc/dvrconfig.ini
	//p->cur_file_idx = 0;

	if (file_manager->cur_file_idx >= file_manager->cur_max_filenum)
	{
		file_manager->cur_file_idx = 0;
	}

	//unsigned long long as = availSize(p->cur_filedir);  //获得指定路径所在磁盘剩余空间大小，单位MB
	//unsigned long afile_size = ((mDuration * bit_rate) >> 23) + (13 * mDuration) / 60;  //视频文件大小MB
	//int afile_blksize = (mDuration / 60) * gAFileOccupySizePerMinMb;  //即将录下文件的大小，根据记录时间计算

	unsigned long long as = 0;
	rmidx = file_manager->cur_file_idx;
	reidx = 0;
	int ret;
	unsigned long oldfileSize  = 0;

	do
	{
		as = availSize(file_manager->cur_filedir);  //获得指定路径所在磁盘剩余空间大小，单位MB
		printf("dbg-rec as is %lluM, re is %dM \n", as, (int)(file_manager->file_size_MB + RESERVED_SIZE));

		if (as > (file_manager->file_size_MB + RESERVED_SIZE)) //磁盘剩余空间大于指定录取文件大小，we have enough space
		{
			break;
		}
		else  //there is no enought space for new video file
		{
			if(strlen(file_manager->cur_filesname[rmidx]) != 0)  //if file name not empty
			{
				memset(rec_filename, 0, sizeof(rec_filename));
				sprintf(rec_filename, "%s/%s", file_manager->cur_filedir, file_manager->cur_filesname[rmidx]);

				if(access(rec_filename, F_OK) != 0)  //if file not exist
				{
					printf("file[%d] %s doesn't exist, pass this file.\n", rmidx, file_manager->cur_filesname[rmidx]);
					memset(file_manager->cur_filesname[rmidx], 0, CM_MAX_FILE_LEN);

					if(rmidx >= file_manager->cur_max_filenum)
					{
						rmidx = 0;
					}
					else
					{
						rmidx++;
					}

					continue;
				}

				oldfileSize = getFileOccupySizeMb(rec_filename);

				if (oldfileSize >= file_manager->file_size_MB)
				{
					printf("dbg-rec file size dismatch,remove idx[%d] file now, oldfilesize=%d \n", rmidx, oldfileSize);

					if(rmidx != file_manager->cur_file_idx )
					{
						if(remove(rec_filename) == 0)
						{
							memset(file_manager->cur_filesname[rmidx], 0, CM_MAX_FILE_LEN);
						}
					}

					break;
				}
#if 0
				else if ((oldfileSize >= afile_blksize) && (oldfileSize <= ( afile_blksize + gAFileOccupySizePerMinMb)))
				{
					if (rmidx == p->cur_file_idx)
					{
						printf("dbg-rec no need to rm[%d],oldfilesize=%d,just use the old one \n", rmidx, oldfileSize);
						break;
					}

					printf("dbg-rec rmidx != curr_idx, can not use, rm[%d]\n", rmidx);

					ret = remove(p->cur_filename[rmidx]);

					if (ret == 0)
					{
						memset(p->cur_filename[rmidx], 0, CM_MAX_FILE_LEN);
					}
					else
					{
						printf("dbg-rec file room is match but rm file %s fail %d \n",
							  p->cur_filename[rmidx], errno);
					}
				}
#endif
				else
				{
					printf("dbg-rec need to rm[%d]\n", rmidx);

					if ((ret = remove(rec_filename)) == 0)
					{
						memset(file_manager->cur_filesname[rmidx], 0, CM_MAX_FILE_LEN);
					}
					else
					{
						printf("dbg-rec old file room is too small, but rm file %s fail %d \n", \
								rec_filename, errno);
					}
				}
			}

			if (rmidx >= CM_MAX_RECORDFILE_NUM)
			{
				rmidx = 0;
			}
			else
			{
				rmidx++;
			}
		}
	}
	while (reidx++ < file_manager->cur_max_filenum);

	struct tm *tm = NULL;
	time_t now = getDateTime(&tm); //获取系统当前时间
	sprintf(rec_filename, "%s%s%d%c%04d%02d%02d_%02d%02d%02d_%s%s", file_manager->cur_filedir, \
			"/", file_manager->cur_file_idx, '-', tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
			tm->tm_min, tm->tm_sec, ppSuffix[l32_CameraId], EXT_VIDEO);

#ifdef FILE_NO_REMOVE

#ifdef FILE_HOLES_SYS_SUPPORT

	int perfilesize = (gAFileOccupySizePerMinMb << 20) * (mDuration / 60);

	if (access(p->cur_filename[p->cur_file_idx], F_OK) == 0)
	{
		ALOGW("dbg-rec t a file %s ", p->cur_filename[p->cur_file_idx]);
		ret = xxCreateMyFile(p->cur_filename[p->cur_file_idx], perfilesize);	//this is needed to prevent kernel crash
		if (ret < 0)
		{
			ret = remove(p->cur_filename[p->cur_file_idx]);

			if (ret == 0)
			{
				ALOGW("dbg-rec room dismatch,rm old file first and then create it again");
				ret = xxCreateMyFile(p->cur_filename[p->cur_file_idx], perfilesize);	//this is needed to prevent kernel crash
			}
			else
			{
				ALOGE("dbg-rec old file OccupySize is not match the new one ,but rm old file err%d",
					  errno);
			}
		}

		ret = truncate(p->cur_filename[p->cur_file_idx], 0);
		if (ret != 0) {
			ALOGW("dbg-rec t a file %s ret=%d (%d):%s", rec_filename, ret, errno, strerror(errno));
		}

		ret = rename(p->cur_filename[p->cur_file_idx], rec_filename);
		if (ret != 0) {
			ALOGE("genfile re file faile %s ret=%d (%d):%s", rec_filename, ret, errno,
				  strerror(errno));
		}

	}
	else
	{
		ALOGV("dbg-rec f a file %s size=%dM", rec_filename, perfilesize >> 20);
		xxCreateMyFile(rec_filename, perfilesize);
	}

	//ALOGV("dbg-rec ids=[%d] perfilesize=%dM,getFileOccupySizeMb=%d,getFileSize=%d",p->cur_file_idx,(perfilesize>>20),
	//                                      getFileOccupySizeMb(rec_filename),getFileSize(rec_filename));
#endif
#else //haven't define FILE_NO_REMOVE
//	ret = rename(file_manager->cur_filesname[file_manager->cur_file_idx], rec_filename);
//	if (ret != 0)
//	{
//		printf("genfile re file fail \n");
//	}
#endif

	strcpy(name, rec_filename);
	printf("genfile %s \n", name);

	strcpy(file_manager->cur_filesname[file_manager->cur_file_idx], rec_filename+strlen(file_manager->cur_filedir)+1);

	if (file_manager->cur_file_idx >= file_manager->cur_max_filenum)
	{
		file_manager->cur_file_idx = 0;
	}
	else
	{
		file_manager->cur_file_idx++;
	}

	//config_set_camfileidx(0, p->cur_file_idx);
	return 0;
}
