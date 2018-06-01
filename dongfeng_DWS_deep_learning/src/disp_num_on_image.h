/*
 * disp_num_on_image.h
 *
 *  Created on: May 27, 2018
 *      Author: tony
 */

#ifndef DISP_NUM_ON_IMAGE_H_
#define DISP_NUM_ON_IMAGE_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define A_IMAGE_WIDTH  720

static FILE *fpHzk = NULL;
static unsigned char hz32Buffer[50][32][4];

static unsigned char hzHead[50][2] =
{
		{0xA3,0xB0},	// 0
		{0xA3,0xb1},	// 1
		{0xA3,0xb2},	// 2
		{0xA3,0xb3},	// 3
		{0xA3,0xb4},	// 4
		{0xA3,0xb5},	// 5
		{0xA3,0xb6},	// 6
		{0xA3,0xb7},	// 7
		{0xA3,0xb8},	// 8
		{0xA3,0xb9},	// 9
};

// 初始化汉字字库32*32
inline void Hz32Init(void)
{
	int hzSize = 10, i;
	unsigned long  offset = 0;

	if(fpHzk == NULL)
	{
		fpHzk = fopen("/home/user/HZK32", "rb");
	}
	else
	{
		return;
	}

	for(i = 0; i < hzSize; i++)
	{
		offset = (94*(unsigned int)(hzHead[i][0]-0xa0-1)+(hzHead[i][1]-0xa0-1))*128;
		fseek(fpHzk, offset, SEEK_SET);
		fread(hz32Buffer[i], 128, 1, fpHzk);
	}

	fclose(fpHzk);
}

// 绘制32*32的汉字
static inline void DrawHz32(int posX, int posY, int width, int hzID, unsigned char* imageBuff)
{
    int i, j, k,  line = width*posY;
    char a = 0;

	for(i=0; i<32; i++)
	{
		for(j=0; j<4; j++)
		{
			for(k=0; k<8; k++)
			{
				if(hz32Buffer[hzID][i][j] & (0x80>>k))
				{
					imageBuff[k+j*8 + posX + line] = 0xFF;
				}
				else
				{
					imageBuff[k+j*8 + posX + line] = 0x00;
				}
			}
			a += 8;
		}

		line += width;
		a = 0;
	}
}


inline void DrawNums32(int posX, int posY, int num, unsigned char* imgBuff)
{
	int hzID = 0;
	num %= 10;
	hzID = num;
	DrawHz32(posX+64, posY, A_IMAGE_WIDTH, hzID, imgBuff);
}


inline void uyvy_2_gray(unsigned char *uyvy,  unsigned char *gray)
{
    unsigned int i = 0;
	unsigned int j = 0;

	j = 720*480;
	/*get Y component from data*/
	for(i = 0; i < j; i++)
	{
		*(gray + i) = *(uyvy + i*2 + 1);
	}

	j = 720*480/2;
	for(i = 0; i < j; i++)
	{
		*(gray+2*j+i) = *(uyvy+4*i+0);  //get U component from UYVY data
		*(gray+3*j+i) = *(uyvy+4*i+2);  //get Y component from UYVY data
	}
}



inline void gray_2_uyvy(unsigned char *gray,unsigned char *uyvy)
{
	unsigned int i = 0;
	unsigned int j = 0;

	j = 720*480;
	/*get Y component from data*/
	for(i = 0; i < j; i++)
	{
		*(uyvy + i*2 + 1)=*(gray + i);
	}

	j = 720*480/2;
	for(i = 0; i < j; i++)
	{
		*(uyvy+4*i+0)=*(gray+2*j+i) ;  //get U component from UYVY data
		 *(uyvy+4*i+2)=*(gray+3*j+i);  //get Y component from UYVY data
	}
}




#endif /* DISP_NUM_ON_IMAGE_H_ */
