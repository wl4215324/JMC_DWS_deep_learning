/*
 * file_operation.c
 *
 *  Created on: Nov 13, 2019
 *      Author: tony
 */

#include "file_operation.h"



 //total size, in bytes 文件大小, 以字节计算
unsigned long get_file_size(const char *path)
{
    unsigned long filesize = -1;
    struct stat statbuff;

    if(stat(path, &statbuff) < 0)
    {
        return filesize;
    }
    else
    {
        filesize = statbuff.st_size;
    }

    return filesize;
}


/*
 * function:
 * description: get total size (unit MB) of specificied path
 */
unsigned long long totalSize(const char *path)
{
    struct statfs diskinfo;

    if(statfs(path, &diskinfo) != -1)
    {
        unsigned long long blocksize = diskinfo.f_bsize;    //每个block里包含的字节数
        unsigned long long totalsize = diskinfo.f_blocks * blocksize;   //总的字节数，f_blocks为block的数目
        return (unsigned long long)(totalsize >> 20); //MB
    }

    return 0;
}


/*
 * function:
 * description: get free size (unit MB) of specificied path
 */
unsigned long long availSize(const char *path)
{
    struct statfs diskinfo;

    if(statfs(path, &diskinfo) != -1)
    {
        unsigned long long blocksize = diskinfo.f_bsize;    //每个block里包含的字节数
        unsigned long long freeDisk = diskinfo.f_bfree * blocksize; //剩余空间的大小
        return (unsigned long long)(freeDisk >> 20); //MB
    }
    else
    {
    	return (unsigned long long)(-1);
    }
}


int needDeleteFiles()
{
    unsigned long long as = availSize(MOUNT_PATH);
    unsigned long long ts = totalSize(MOUNT_PATH);

    if (as <= RESERVED_SIZE)
    {
//        ALOGE("!Disk FULL");
        return RET_DISK_FULL;
    }

    if (as <= LOOP_COVERAGE_SIZE)
    {
//        ALOGE("!Loop Coverage");
        return RET_NEED_LOOP_COVERAGE;
    }

    return 0;
}


int isMounted(const char *checkPath)
{
    const char *path = "/proc/mounts";
    FILE *fp;
    char line[255];
    const char *delim = " \t";
    printf(" isMount checkPath=%s \n",checkPath);

    if (!(fp = fopen(path, "r")))
    {
    	printf(" isMount fopen fail,path=%s\n",path);
        return 0;
    }

    memset(line, '\0' ,sizeof(line));

    while(fgets(line, sizeof(line), fp))
    {
        char *save_ptr = NULL;
        char *p = NULL;

        if (line[0] != '/' || (strncmp("/dev/mmcblk", line, strlen("/dev/mmcblk")) != 0 &&
			 strncmp("/dev/sd",line,strlen("/dev/sd")) != 0) )
        {
            memset(line,'\0',sizeof(line));
            continue;
        }

        if ((p = strtok_r(line, delim, &save_ptr)) != NULL)
        {
            if ((p = strtok_r(NULL, delim, &save_ptr)) != NULL)
            {
            	printf(" isMount p=%s \n", p);

                if(strncmp(checkPath, p, strlen(checkPath)) == 0)
                {
                    fclose(fp);
                    printf(" isMount return 1\n");
                    return 1;
                }
            }
            //printf("line=%s",lineback);
            if(strlen(p)>1)
            {
                if(strstr(checkPath,p))
                {
                    fclose(fp);
                    printf(" isMount dd return 1\n");
                    return 1;
                }
            }
        }

    }

    if(fp)
    {
        fclose(fp);
    }

    return 0;
}


#define MAXSIZE 256
int createdir(const char *path)
{
    char DirName[256];
    int i, len;

    strcpy(DirName, path);
    len = strlen(DirName);

    if('/' != DirName[len-1])
    {
        strcat(DirName, "/");
        len++;
    }

    if(access(DirName, F_OK) == 0)
    {
    	printf("dir %s already exit\n", DirName);
    	return 0;
    }

    for(i=1; i<len; i++)
    {
        if('/' == DirName[i])
        {
            DirName[i] = '\0';

            if(access(DirName, F_OK) != 0)
            {
                if(mkdir(DirName, 0777) == -1)
                {
                    perror("mkdir() failed!");
                    return -1;
                }
            }

            DirName[i] = '/';
         }
    }

  return 0;
}


void DvrRecordManagerInit(void)
{
#if 0
	int i = 0;
	gFilemanger.cur_file_idx = 0;
	gFilemanger.cur_max_filenum = 0;
	gFilemanger.timeout_second = 0;

	for (i = 0; i < CM_MAX_RECORDFILE_NUM; i++) //CM_MAX_RECORDFILE_NUM 400
	{
		memset(gFilemanger.cur_filename[i], 0, CM_MAX_FILE_LEN);  //CM_MAX_FILE_LEN 1024
		memset(gFilemanger.cur_file_thumbname[i], 0, CM_MAX_FILE_LEN);	//temp no use
	}

	for (i = 0; i < MAX_SUPPORT_CAM; i++) //MAX_SUPPORT_CAM 10
	{
		memset(&gFilemanger.cur_filedir[i][0], 0, CM_MAX_FILE_LEN);
	}

	gFilemanger.cur_dir_file_num = 0;
	gAFileOccupySizePerMinMb = 64;	//bit_rate=7M
#endif
}



unsigned int getFileSize(char *filename)
{
	struct stat statbuf;

	if(!filename)
	{
		return 0;
	}

	stat(filename, &statbuf);
	unsigned int size = statbuf.st_size >> 20;
	return size;
}


/*
 *
 */
unsigned long getFileOccupySizeMb(char *filename)
{
	struct stat statbuf;

	if(!filename)
	{
		return 0;
	}

	stat(filename, &statbuf);

	//number of blocks allocated 占用文件区块的个数, 每一区块大小为512 个字节
	unsigned long blksize = statbuf.st_blocks >> 11;
	return blksize;
}


