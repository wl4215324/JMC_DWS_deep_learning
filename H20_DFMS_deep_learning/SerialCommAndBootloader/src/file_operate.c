/*
 * file_operate.c
 *
 *  Created on: Dec 5, 2018
 *      Author: tony
 */

#include "file_operate.h"

bool is_dir_existent(const char *directory_name)
{
	if( strlen(directory_name) <= 0 )
		return false;

    return (access(directory_name, 0) < 0) ? false:true;
}


int is_dir(const char * filename)
{
    struct stat buf;
    int ret = stat(filename, &buf);

    if(0 == ret)
    {
        return (buf.st_mode & S_IFDIR) ? 0:1;
    }

    return -1;
}


bool make_dir(const char *dir_name)
{
	if( strlen(dir_name) <= 0)
		return false;

	return (mkdir(dir_name, 0755) < 0) ? false:true;
}


bool rm_empty_dir(const char *rm_dir)
{
	if(!is_dir_existent(rm_dir))
	{
		printf("%s is not existent, you don't need to remove dir!\n", rm_dir);
		return true;
	}

	if(rmdir(rm_dir) == 0)
	{
		return true;
	}
	else
	{
		perror("rmdir:");
		return false;
	}
}


int delete_dir(const char * dirname)
{
    char chBuf[256];
    DIR * dir = NULL;
    struct dirent *ptr;
    int ret = 0;

    dir = opendir(dirname);

    if(NULL == dir)
    {
        return -1;
    }

    while((ptr = readdir(dir)) != NULL)
    {
        if( (strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0) )
            continue;

        snprintf(chBuf, sizeof(chBuf), "%s/%s", dirname, ptr->d_name);
        ret = is_dir(chBuf);

        if(0 == ret)
        {
            //printf("%s is dir\n", chBuf);
            ret = delete_dir(chBuf);

            if(0 != ret)
            {
                return -1;
            }
        }
        else if(1 == ret)
        {
            //printf("%s is file\n", chBuf);
            ret = remove(chBuf);
            if(0 != ret)
            {
                return -1;
            }
        }
    }

    closedir(dir);
    ret = remove(dirname);
    return (0 != ret) ? -1:0;
}


int createMultiLevelDir(char* sPathName)
{
    int i, len;
    char DirName[256];

    if(!sPathName)
    {
    	return -1;
    }

    memset(DirName, '\0', sizeof(DirName));
    strcpy(DirName, sPathName);
    len = strlen(DirName);

    if('/' != DirName[len-1])
    {
	    strcat(DirName, "/");
	    len++;
    }

    printf("DirName is: %s\n", DirName);

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

		    printf("DirName is: %s\n", DirName);

		    DirName[i] = '/';
	    }
    }

   return 0;
}


