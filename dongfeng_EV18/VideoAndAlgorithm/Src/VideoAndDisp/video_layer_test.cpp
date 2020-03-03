#include "video_layer.h"
#include "sunxiMemInterface.h"
#include "G2dApi.h"

extern "C" {
#include <stdio.h>
}

int alloc_nv41_mem(int w, int h,paramStruct_t*pops)
{
    int iRet;
#if 0
    iRet = allocOpen(MEM_TYPE_DMA, pops, NULL);
    if (iRet < 0) {
        printf("ion_alloc_open failed\r\n");
        return iRet;
    }
    pops->size = w * h * 3 / 2;
    iRet = allocAlloc(MEM_TYPE_DMA, pops, NULL);
    if(iRet < 0) {
        printf("allocAlloc failed\r\n");
        return iRet;
    }
#else
    iRet = allocOpen(MEM_TYPE_CDX_NEW , pops, NULL);
    if (iRet < 0) {
        printf("ion_alloc_open failed\r\n");
        return iRet;
    }
    pops->size = w * h * 3 / 2;
    iRet = allocAlloc(MEM_TYPE_CDX_NEW , pops, NULL);
    if(iRet < 0) {
        printf("allocAlloc failed\r\n");
        return iRet;
    }
#endif
    return 0;
}


int read_yuv_image_file(void *read_buf)
{
	if(read_buf == NULL)
	{
		return -1;
	}

	FILE *fp = fopen("/extp/video2.yuv", "r");
	unsigned int file_len = 0;

	if(fp == NULL)
	{
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);

	if(file_len)
	{
		fseek(fp, 0, SEEK_SET);
		fread(read_buf, file_len, 1, fp);
	}

	fclose(fp);
	return file_len;
}



void* video_layer_test(void *argv)
{
    VideoLayer::LayerCon conf;
#if 0
    VideoLayer::LayerCon config{};
    config.dst_x = disp_rect.x;
    config.dst_y = disp_rect.y;
    config.dst_w = disp_rect.w;
    config.dst_h = disp_rect.h;
    config.ori_w = frm->w;
    config.ori_h = frm->h;
    config.layer_id = disp_chn;
    config.phy_addr =  frm->phy_addr;
    config.enable = 1;
    config.zorder = 2;
#endif
    paramStruct_t pops{};
    alloc_nv41_mem(1280, 720, &pops);
    printf("pops.phy = %x pops.vir = %x\n", pops.phy, pops.vir);

    //memset((void *)pops.vir, 0xff/2, 1280*720*1.5);
    memset((void *)pops.vir, 0xC8, 1280*720*1.5);
    read_yuv_image_file((void *)pops.vir);

    conf.dst_x = 0;
    conf.dst_y = 0;
    conf.dst_w = 720;
    conf.dst_h = 480;
    conf.ori_w = 1280;
    conf.ori_h = 720;
    conf.layer_id = 0;
    conf.phy_addr =  pops.phy;
    conf.enable = 1;
    conf.zorder = 2;

    VideoLayer layer{};
    layer.set_layer(&conf);

    VideoLayer::LayerCon conf2;
    paramStruct_t pops2{};
    alloc_nv41_mem(1280, 720, &pops2);
    conf2.dst_x = 0;
    conf2.dst_y = 0;
    conf2.dst_w = 720;
    conf2.dst_h = 480;
    conf2.ori_w = 1280;
    conf2.ori_h = 720;
    conf2.layer_id = 0;
    conf2.phy_addr =  pops2.phy;
    conf2.enable = 1;
    conf2.zorder = 2;
    memset((void *)pops2.vir, 0xC8, 1280*720*1.5);
    read_yuv_image_file((void *)pops2.vir);

    while(1){
#if 0
    memset((void *)pops.vir, 0xff/2, 1280*720*1.5);
    usleep(10000);
    memset((void *)pops.vir, 0xff, 1280*720*1.5);
    usleep(10000);
#else
    layer.set_layer(&conf);
    usleep(100000);
    layer.set_layer(&conf2);
    usleep(100000);
#endif

    }

    return NULL;
}


VideoLayer *layer{};

int init_video_layer()
{
	layer = new VideoLayer;
	return 0;
}

int dsm_camera_display(unsigned int phy_addr)
{
    VideoLayer::LayerCon conf{};
    conf.dst_x = 0;
    conf.dst_y = 0;
    conf.dst_w = 360;  //720
    conf.dst_h = 482;  //480
    conf.phy_addr = phy_addr;
    //printf("frm->phy_addr %x\n", conf.phy_addr);
#ifdef ROTATE90_DISP
    conf.ori_w = 1280;
    conf.ori_h = 720;
#else
    conf.ori_w = 1280;
    conf.ori_h = 720;
#endif
    conf.layer_id = 0;
    conf.enable = 1;
    conf.zorder = 2;
    layer->set_layer(&conf);
    return 0;
}

int monitor_camera_display(unsigned int phy_addr)
{
    VideoLayer::LayerCon conf{};
    conf.dst_x = 360;
    conf.dst_y = 0;
    conf.dst_w = 360;  //720
    conf.dst_h = 480;  //480
    conf.phy_addr = phy_addr;
    //printf("frm->phy_addr %x\n", conf.phy_addr);
    conf.ori_w = 1280;
    conf.ori_h = 720;
    conf.layer_id = 1;
    conf.enable = 1;
    conf.zorder = 2;
    layer->set_layer(&conf);
    return 0;
}
