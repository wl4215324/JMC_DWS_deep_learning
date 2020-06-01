/*
 * files_manager.c
 *
 *  Created on: Nov 13, 2019
 *      Author: tony
 */

#include "files_manager.h"
#include "file_operation.h"
#include "../../../ShmCommon/rtc_operations.h"
#include "warn_video_store.h"
#include "../iniparser/usr_conf.h"

char g_PicPath[MAX_SUPPORT_CAM][PIC_NAME_LEN] = { };   //mod lss
char g_NamePic[MAX_SUPPORT_CAM][CM_MAX_PICTURE_NUM][PIC_NAME_LEN] = { };

char *pVideoPath[MAX_SUPPORT_CAM] = {
	"dsm_video",
	"monitor_video",
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

char *pFileType[5] = {
		".jpg",
		".h264",
		".h265",
};


file_status_t gFilemanger;

char **ppSuffix = pSuffix;
unsigned int mDuration = 10;
unsigned int bit_rate = 5 * 1024 * 1024;
unsigned int gAFileOccupySizePerMinMb = 64;


/*
 * function: initialize object file_manager according to dirname
 */
int init_file_manager(char *dirname, file_status_t *file_manager, unsigned char camera_idx, \
		              unsigned char bit_rate_Mb, unsigned short duration_s)
{
	DIR *dir_ptr;
	struct dirent *direntp;
	int i;
	char tmpbuf[256] = "";
	char rmbuf[256] = "";
	char real_path[256] = "";
	char camera_path[256] = "";
	char dir_len = 0;
	int idx;

	if(!dirname || !file_manager || file_manager == (file_status_t *)(-1) || !bit_rate_Mb)
	{
		return -1;
	}

	if(camera_idx > MAX_SUPPORT_CAM)
	{
		camera_idx = MAX_SUPPORT_CAM;
	}

	if(0 == camera_idx)
	{
		sprintf(tmpbuf, "%s:%s", PROOF_FILE_SEC, DMS_CUR_FILE_INDEX);
	}
	else
	{
		sprintf(tmpbuf, "%s:%s", PROOF_FILE_SEC, MONITOR_CUR_FILE_INDEX);
	}

	read_inifile(INI_CONF_FILE_PATH, tmpbuf, rmbuf);  //get file index
	file_manager->cur_file_idx = atoi(rmbuf);
	file_manager->cur_max_filenum = 0;
	file_manager->cur_dir_file_num = 0;
	file_manager->file_size_MB = 0;
	file_manager->file_dir_status = SDCARD_NOT_MOUNT;
	memset(file_manager->cur_filedir, 0, sizeof(file_manager->cur_filedir));

	for(i=0; i<CM_MAX_RECORDFILE_NUM; i++)
	{
		memset(file_manager->cur_filesname+i, 0, CM_MAX_FILE_LEN);
	}

	strcpy(tmpbuf, dirname);
	dir_len = strlen(dirname);

	//if the last char of string is '/' then remove it, else nothing to be done
	if (tmpbuf[dir_len-1] == '/')
	{
		tmpbuf[dir_len-1] = '\0';
	}

	if(access(tmpbuf, F_OK) < 0)  //is dir or file existent ?
	{
		return -1;
	}

	int rmret = 0;
	// if tmpbuf is just a symbol link, then get real path
	rmret = readlink(tmpbuf, real_path, sizeof(real_path));
	if (rmret < 0)  //not a symbol link
	{
		strcpy(real_path, tmpbuf);
	}
	else  //real path
	{
		real_path[rmret] = '\0';
	}

	if(isMounted(real_path))  //judge SD card is mounted or not
    {
		printf("-------sdmmc------mounted \n");
	}
	else
	{
		//sd card is not unmounted, then following is unnecessary to be executed
		printf("-------sdmmc------unmounted \n");
		return -1;
	}

	file_manager->file_dir_status = FILE_DIR_NOT_CREATE;
	memset(camera_path, 0, sizeof(camera_path));
	sprintf(camera_path, "%s/%s", real_path, pVideoPath[camera_idx]);
	printf("camera_path: %s\n", camera_path);

	/* if warnning file path is existent then do nothing, or else create a new directory for files */
	if (createdir(camera_path) != 0)
	{
		printf("create camera_path dir %s fail", camera_path);
		return -1;
	}

	file_manager->file_dir_status = FILE_DIR_NORMAL;
	strcpy(file_manager->cur_filedir, camera_path);
	unsigned long long totMB  = totalSize(real_path);  //MB

	/* round up to an integer according to fomula (M+N-1)/N */
	file_manager->file_size_MB = (duration_s*bit_rate_Mb  + 7) >> 3;
	file_manager->file_size_MB += 1; //additional 1MB for jpeg image

	if(file_manager->file_size_MB <= 1)
	{
		file_manager->file_size_MB = 10; //default 10MB
	}

	file_manager->cur_max_filenum = (totMB - RESERVED_SIZE) / file_manager->file_size_MB;

	if(file_manager->cur_max_filenum  > CM_MAX_RECORDFILE_NUM)
	{
		file_manager->cur_max_filenum = CM_MAX_RECORDFILE_NUM;
	}

	// read files's name stored in specified directory and put specified array according to index
	if((dir_ptr = opendir(camera_path)))
	{
		while ((direntp = readdir(dir_ptr)) != NULL)
		{
		    if(strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) //current dir or parrent dir
			{
			    continue;
			}

			if(direntp->d_type & DT_DIR)  //dir
			{
				if (strstr(direntp->d_name, CM_SAVE_FILE_MARK) != NULL)  //skip dir "save"
					continue;
				if (strstr(direntp->d_name, CM_LOCK_FILE_MARK) != NULL)  //skip dir "lock"
					continue;

				for (i = 0; i < strlen(direntp->d_name); i++)  //dir name not empty
				{
					if (direntp->d_name[i] == '-')  //dir name 1-20100101_001013_front
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
									sprintf(tmpbuf, "rm -rf %s", rmbuf);
									system(tmpbuf);
	                                //strcpy(p->cur_filesname[idx], camera_path);
	                                //strcat(p->cur_filesname[idx], "/");
									strcat(file_manager->cur_filesname[idx], direntp->d_name);
								}
							}
						}
						else //if index is invalid, delete the dir named with invalid index
						{
							memset(rmbuf, 0 ,sizeof(rmbuf));
							sprintf(rmbuf, "%s/%s", file_manager->cur_filedir, direntp->d_name);
							sprintf(tmpbuf, "rm -rf %s", rmbuf);
							system(tmpbuf);
						}
					}
				}

				/* if dir name format is invalid, then delete the dir */
				if(i >= strlen(direntp->d_name))
				{
					memset(rmbuf, 0 ,sizeof(rmbuf));
					sprintf(rmbuf, "%s/%s", file_manager->cur_filedir, direntp->d_name);
					sprintf(tmpbuf, "rm -rf %s", rmbuf);
					system(tmpbuf);
				}
			}
		}

		closedir(dir_ptr);
		sync();
	}

	return 0;
}


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
	char tmpbuf[256] = "";
	char rmbuf[256] = "";
	char real_path[256] = "";
	char camera_path[256] = "";
	int len = 0;

	if (!dirname || (len = strlen(dirname)) < 2 || !bit_rate_Mb )
	{
		return (file_status_t*)(-1);
	}

	unsigned char l32_CameraId = 0;
	l32_CameraId = (camera_index >= MAX_SUPPORT_CAM) ? MAX_SUPPORT_CAM: camera_index;
	strcpy(tmpbuf, dirname);

	// if the last char of string is '/' then remove it, else nothing to be done
	if(tmpbuf[len-1] == '/')
	{
		tmpbuf[len-1] = '\0';
	}

	if(access(tmpbuf, F_OK) < 0)  //if dir or file is existent ?
	{
		return (file_status_t*)(-1);
	}

	int rmret = 0;
	/* if tmpbuf is just a symbol link, then get real path */
	rmret = readlink(tmpbuf, real_path, sizeof(real_path));
	if (rmret < 0)  //not a symbol link
	{
		strcpy(real_path, tmpbuf);
	}
	else  //real path
	{
		real_path[rmret] = '\0';
	}

	file_status_t *file_manager = (file_status_t*) malloc(sizeof(file_status_t));
	if(!file_manager)
	{
		goto init_files_dir_error;
	}

	file_manager->cur_max_filenum = 0;
	file_manager->cur_dir_file_num = 0;

	if(0 == l32_CameraId)
	{
		sprintf(tmpbuf, "%s:%s", PROOF_FILE_SEC, DMS_CUR_FILE_INDEX);
	}
	else
	{
		sprintf(tmpbuf, "%s:%s", PROOF_FILE_SEC, MONITOR_CUR_FILE_INDEX);
	}

	read_inifile(INI_CONF_FILE_PATH, tmpbuf, rmbuf);  //get vin code
	file_manager->cur_file_idx = atoi(rmbuf);
	DEBUG_INFO(file_manager->cur_file_idx: %d\n, file_manager->cur_file_idx);
	file_manager->file_size_MB = 0;
	file_manager->file_dir_status = SDCARD_NOT_MOUNT;
	memset(file_manager->cur_filedir, 0, sizeof(file_manager->cur_filedir));

	for(i=0; i<CM_MAX_RECORDFILE_NUM; i++)
	{
		memset(file_manager->cur_filesname+i, 0, CM_MAX_FILE_LEN);
	}

	if (isMounted(real_path))  //judge SD card is mounted or not
    {
		printf("-------sdmmc------mounted \n");
	}
	else
	{
		//sd card is not unmounted, then following is unnecessary to be executed
		printf("-------sdmmc------unmounted \n");
		goto init_files_dir_error;
	}

	file_manager->file_dir_status = FILE_DIR_NOT_CREATE;
	memset(camera_path, 0, sizeof(camera_path));
	sprintf(camera_path, "%s/%s", real_path, pVideoPath[l32_CameraId]);
	printf("camera_path: %s\n", camera_path);

	/* if warnning file path is existent then do nothing, or else create a new directory for files */
	if(createdir(camera_path) != 0)
	{
		printf("create camera_path dir %s fail", camera_path);
		return (file_status_t*)(-1);
	}

	file_manager->file_dir_status = FILE_DIR_NORMAL;
	strcpy(file_manager->cur_filedir, camera_path);
	unsigned long long totMB  = totalSize(real_path);  //MB

	/* round up to an integer according to fomula (M+N-1)/N */
	file_manager->file_size_MB = ((duration_s * bit_rate_Mb + 7) >> 3);  //b to B
	file_manager->file_size_MB += 1; //additional 1MB for jpeg

	if(file_manager->file_size_MB <= 1)
	{
		//file_manager->file_size_MB = (totMB - RESERVED_SIZE) / CM_MAX_RECORDFILE_NUM;
		file_manager->file_size_MB = 10;  //default 10MB
	}

	file_manager->cur_max_filenum = (totMB - RESERVED_SIZE) / file_manager->file_size_MB;

	if(file_manager->cur_max_filenum  > CM_MAX_RECORDFILE_NUM)
	{
		file_manager->cur_max_filenum = CM_MAX_RECORDFILE_NUM;
	}

	/* read files's name stored in specified directory and put specified array according to index */
	if((dir_ptr = opendir(camera_path)))
	{
		while ((direntp = readdir(dir_ptr)) != NULL)
		{
		    if(strcmp(direntp->d_name, ".")==0 || strcmp(direntp->d_name, "..")==0) //current dir OR parrent dir
			{
			    continue;
			}

			if(direntp->d_type & DT_DIR)  //is dir
			{
				if (strstr(direntp->d_name, CM_SAVE_FILE_MARK) != NULL)  //skip dir "save"
					continue;
				if (strstr(direntp->d_name, CM_LOCK_FILE_MARK) != NULL)  //skip dir "lock"
					continue;

				for (i = 0; i < strlen(direntp->d_name); i++)  //file name not empty
				{
					if (direntp->d_name[i] == '-')  //dir name 1-20100101_001013_front
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
								// if rmbuf is not substring of cur_filename[idx], then remove file cur_filename[idx]
								if (strcasecmp(file_manager->cur_filesname[idx], direntp->d_name) == 0)
								{
									printf("find duplicate  so rm old %s idx=%d  name=%s \n", file_manager->cur_filesname[idx], idx, direntp->d_name);
									memset(rmbuf, 0 ,sizeof(rmbuf));
									sprintf(rmbuf, "%s/%s", file_manager->cur_filedir, direntp->d_name);
									sprintf(tmpbuf, "rm -rf %s", rmbuf);
									system(tmpbuf);

	                                //strcpy(p->cur_filesname[idx], camera_path);
	                                //strcat(p->cur_filesname[idx], "/");
									strcat(file_manager->cur_filesname[idx], direntp->d_name);
								}
							}
						}
						else //if index is invalid, delete the file with invalid index
						{
	                        memset(rmbuf, 0 ,sizeof(rmbuf));
	                        sprintf(rmbuf, "%s/%s", file_manager->cur_filedir, direntp->d_name);
	    					sprintf(tmpbuf, "rm -rf %s", rmbuf);
	    					system(tmpbuf);
						}
					}
				}

				/* if filename format is invalid, then delete the file */
				if(i >= strlen(direntp->d_name))
				{
					memset(rmbuf, 0 ,sizeof(rmbuf));
					sprintf(rmbuf, "%s/%s", file_manager->cur_filedir, direntp->d_name);
					sprintf(tmpbuf, "rm -rf %s", rmbuf);
					system(tmpbuf);
				}
			}
		}

		closedir(dir_ptr);
		sync();
	}

	return file_manager;

init_files_dir_error:
    if(file_manager && file_manager != (file_status_t*)(-1))
    {
    	free(file_manager);
    }
	return (file_status_t*)(-1);
}



/*****************************************************************
Function: int genfilename(char *name, file_status_t *file_manager)
Description: allocate video file name according to object file manager
Parameters:
           char *name: allocated file name
           file_status_t *file_manager: file manager object for
           storing and managing video files
******************************************************************/
int genfilename(char *name, file_status_t *file_manager, unsigned char camera_idx, \
        unsigned char bit_rate_Mb, unsigned short duration_s)
{
	char rec_filename[256], new_file_name[256];
	int rmidx = 0, reidx = 0;
	int l32_CameraId = 0;

	if (!name || !file_manager || (file_manager == (file_status_t*)(-1)) ||\
	    (file_manager->cur_max_filenum == 0))
		return -1;

	if(file_manager->file_dir_status != FILE_DIR_NORMAL)
	{
		init_file_manager(SD_MOUNT_DIRECTORY, file_manager, camera_idx, bit_rate_Mb, duration_s);
	}

	if (file_manager->cur_file_idx >= file_manager->cur_max_filenum)
	{
		file_manager->cur_file_idx = 0;
	}

	//unsigned long long as = availSize(p->cur_filedir);  //获得指定路径所在磁盘剩余空间大小，单位MB
	//unsigned long afile_size = ((mDuration * bit_rate) >> 23) + (13 * mDuration) / 60;  //视频文件大小MB
	//int afile_blksize = (mDuration / 60) * gAFileOccupySizePerMinMb;  //即将录下文件的大小，根据记录时间计算

	signed long long as = 0;
	rmidx = file_manager->cur_file_idx;
	reidx = 0;
	int ret;
	unsigned long oldfileSize  = 0;

	do
	{
		as = availSize(file_manager->cur_filedir);  //获得指定路径所在磁盘剩余空间大小，单位MB
		printf("dbg-rec as is %lldM, re is %dM \n", as, (int)(file_manager->file_size_MB + RESERVED_SIZE));

		if (as > (file_manager->file_size_MB + RESERVED_SIZE)) //磁盘剩余空间大于指定录取文件大小，we have enough space
		{
			break;
		}
		else if(as < 0)
		{
			return -1;
		}
		else  //there is no enough space for new video file, old file needs to be deleted
		{
			if(strlen(file_manager->cur_filesname[rmidx]) != 0)  //if file name is not empty
			{
				memset(rec_filename, 0, sizeof(rec_filename));
				sprintf(rec_filename, "%s/%s", file_manager->cur_filedir, file_manager->cur_filesname[rmidx]);

				if(access(rec_filename, F_OK) != 0)  //if file not exist
				{
					printf("file[%d] %s doesn't exist, pass this file.\n", rmidx, file_manager->cur_filesname[rmidx]);
					memset(file_manager->cur_filesname[rmidx], 0, CM_MAX_FILE_LEN);
				}
				else  //if file exist, delete the file
				{

					sprintf(new_file_name, "rm -rf %s", rec_filename);
					system(new_file_name);
				}
			}

			if(rmidx >= file_manager->cur_max_filenum)
			{
				rmidx = 0;
			}
			else
			{
				rmidx++;
			}
		}
	}while (1);

	struct tm *tm = NULL;
	time_t now = getDateTime(&tm); //获取系统当前时间
	sprintf(name, "%s%s%d%c%04d%02d%02d_%02d%02d%02d_%s", file_manager->cur_filedir, \
			"/", file_manager->cur_file_idx, '-', tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, \
			tm->tm_min, tm->tm_sec, ppSuffix[l32_CameraId]);

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

	//strcpy(name, rec_filename);
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

	// write variable cur_file_idx into file
	//config_set_camfileidx(0, p->cur_file_idx);
	memset(rec_filename, 0, sizeof(rec_filename));
	sprintf(rec_filename, "%d", file_manager->cur_file_idx);
	memset(new_file_name, 0, sizeof(new_file_name));

	if(0 == camera_idx)
	{
		sprintf(new_file_name, "%s:%s", PROOF_FILE_SEC, DMS_CUR_FILE_INDEX);
	}
	else
	{
		sprintf(new_file_name, "%s:%s", PROOF_FILE_SEC, MONITOR_CUR_FILE_INDEX);
	}

	change_inifile(INI_CONF_FILE_PATH, new_file_name, rec_filename);
	return 0;
}
