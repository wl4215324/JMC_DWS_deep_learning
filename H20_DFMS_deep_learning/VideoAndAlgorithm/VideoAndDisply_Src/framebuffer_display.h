/*
 * framebuffer_display.h
 *
 *  Created on: Jun 14, 2019
 *      Author: tony
 */

#ifndef FRAMEBUFFER_DISPLAY_H_
#define FRAMEBUFFER_DISPLAY_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>


#define  DISPLAY_DEV  "/dev/fb0"

#define  DISPLAY_IMAGE_WIDTH  1280
#define  DISPLAY_IMAGE_HEIGHT  720

typedef struct {
	int fb;
	char *fb_mem;
	unsigned int screen_size;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
} FBDEV;


static inline int open_display_dev(FBDEV *fb_dev)
{
	long screensize = 0;
    char *pfb = NULL;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

	int fb = open(DISPLAY_DEV, O_RDWR);

	if(fb < 0)
	{
		return -1;
	}

    if(ioctl(fb, FBIOGET_FSCREENINFO, &finfo))
    {
        printf("Error reading fixed information/n");
        return -1;
    }

    if(ioctl(fb, FBIOGET_VSCREENINFO, &vinfo))
    {
        printf("Error reading variable information/n");
        return -1;
    }

    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    printf("The phy mem = 0x%x, total size = %d(byte)\n", finfo.smem_start, finfo.smem_len);
    printf("xres =  %d, yres =  %d, bits_per_pixel = %d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    printf("So the screensize = %d(byte), using %d frame\n", screensize, finfo.smem_len/screensize);
    printf("vinfo.xoffset = %d, vinfo.yoffset = %d\n", vinfo.xoffset, vinfo.yoffset);
    printf("vinfo.vmode is :%d\n", vinfo.vmode);
    printf("finfo.ypanstep is :%d\n", finfo.ypanstep);
    printf("vinfo.red.offset=0x%x\n", vinfo.red.offset);
    printf("vinfo.red.length=0x%x\n", vinfo.red.length);
    printf("vinfo.green.offset=0x%x\n", vinfo.green.offset);
    printf("vinfo.green.length=0x%x\n", vinfo.green.length);
    printf("vinfo.blue.offset=0x%x\n", vinfo.blue.offset);
    printf("vinfo.blue.length=0x%x\n", vinfo.blue.length);
    printf("vinfo.transp.offset=0x%x\n", vinfo.transp.offset);
    printf("vinfo.transp.length=0x%x\n", vinfo.transp.length);

    pfb = (char*)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);

    if (NULL == pfb)
    {
        perror("mmap error!");
        return -1;
    }

    fb_dev->fb = fb;
    fb_dev->fb_mem = pfb;
    fb_dev->screen_size = screensize;
    memcpy(&fb_dev->finfo, &finfo, sizeof(finfo));
    memcpy(&fb_dev->vinfo, &vinfo, sizeof(vinfo));

	return 0;
}

//static inline void fb_update(int fb, struct fb_var_screeninfo *vi)   //将要渲染的图形缓冲区的内容绘制到设备显示屏来
static inline void update_framebuffer(FBDEV *fb_dev)
{
	fb_dev->vinfo.yoffset = 1;
    ioctl(fb_dev->fb, FBIOPUT_VSCREENINFO, fb_dev->vinfo);
    fb_dev->vinfo.yoffset = 0;
    ioctl(fb_dev->fb, FBIOPUT_VSCREENINFO, fb_dev->vinfo);
}

static inline int close_display_dev(FBDEV *fb_dev)
{
	munmap(fb_dev->fb_mem, fb_dev->screen_size);
	close(fb_dev->fb);
	return 0;
}

static inline int update_display_dev(int fb, char *new_data, int data_len)
{
	int ret_val = 0;

	if((!new_data) || (data_len <= 0))
	{
		return -1;
	}

	return write(fb, new_data, data_len);
}


static inline int YUV420p2RGBA(char *yuv_buf, char *rgba_buf, int width, int height)
{
	char *ybuf = yuv_buf;
	char *ubuf = yuv_buf + width*height;
	char *vbuf = ubuf +  width*height/4;

	unsigned char Y, U, V;
	unsigned char R, G, B;

	int i, j;
	int nWidth = width>>1; //色度信号宽度

	for(i=0; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
		    Y = *(ybuf + i*width + j);
			U = *(ubuf + ((i>>1)*nWidth) + (j>>1));
			V = *(vbuf + ((i>>1)*nWidth) + (j>>1));

		    R = ((Y << 8) + ((V << 8) + (V << 5) + (V << 2))) >> 8;
		    G = ((Y << 8) - ((U << 6) + (U << 5) + (U << 2)) - ((V << 7) + (V << 4) + (V << 2) + V)) >> 8;
		    B = ((Y << 8) + (U << 9) + (U << 3)) >> 8;

		    //防止越界
		    if (R>255)R=255;
		    if (R<0)R=0;
		    if (G>255)G=255;
		    if (G<0)G=0;
		    if (B>255)B=255;
		    if (B<0)B=0;

		   *(rgba_buf + (i*width + j)*4) = B;
		   *(rgba_buf + (i*width + j)*4 + 1) = G;
		   *(rgba_buf + (i*width + j)*4 + 2) = R;
		   *(rgba_buf + (i*width + j)*4 + 3) = 0xff;
		}
	}

	return 0;
}


static inline void yuv420ToRgb(char *yuv420, int width, int height, char *rgba)
{
//    unsigned char *pBufy = new unsigned char[w*h];
//    unsigned char *pBufu = new unsigned char[w*h/4];
//    unsigned char *pBufv = new unsigned char[w*h/4];



//    memcpy(pBufy,yuv,w*h);
//    memcpy(pBufu,yuv+w*h,w*h/4);
//    memcpy(pBufv,yuv+w*h*5/4,w*h/4);
//
//    for(int i = 0; i<w*h/4;i++)
//    {
//        rgb[i*3+2] = pBufy[i]+1.772*(pBufu[i]-128);  //B = Y +1.779*(U-128)
//        rgb[i*3+1] = pBufy[i]-0.34413*(pBufu[i]-128)-0.71414*(pBufv[i]-128);//G = Y-0.3455*(U-128)-0.7169*(V-128)
//        rgb[i*3+0] = pBufy[i]+1.402*(pBufv[i]-128);//R = Y+1.4075*(V-128)
//    }

//    free(pBufy);
//    free(pBufu);
//    free(pBufv);
//    return true;

	char *ybuf = yuv420;
	char *ubuf = ybuf + width*height;
	char *vbuf = ubuf +  width*height/4;

	unsigned char Y, U, V;
	unsigned char R, G, B;
	int nWidth = width>>1; //色度信号宽度

	int i, j;

	for(i=0; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			Y = *(ybuf + i*width + j);
			U = *(ubuf + ((i>>1)*nWidth) + (j>>1));
			V = *(vbuf + ((i>>1)*nWidth) + (j>>1));

		   R = Y + 1.402*(V-128);
		   G = Y - 0.34414*(U-128) - 0.71414*(V-128);
		   B = Y + 1.772*(U-128);

		   //防止越界
		   if (R>255)R=255;
		   if (R<0)R=0;
		   if (G>255)G=255;
		   if (G<0)G=0;
		   if (B>255)B=255;
		   if (B<0)B=0;

		   *(rgba + (i*width + j)*4) = B;
		   *(rgba + (i*width + j)*4 + 1) = G;
		   *(rgba + (i*width + j)*4 + 2) = R;
		   *(rgba + (i*width + j)*4 + 3) = 0xff;
		}
	}
}





#endif /* FRAMEBUFFER_DISPLAY_H_ */
