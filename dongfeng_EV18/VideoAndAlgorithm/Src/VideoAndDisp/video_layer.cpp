#include "video_layer.h"
#include <fstream>
using namespace std;
extern "C"{
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <unistd.h>
#include "sunxi_display2.h"
}

int print_disp_fb_info(struct disp_fb_info *fb)
{

    printf("%s, \n", __func__);
    int i;
    for(i = 0; i < 3; i++){
        printf(" %d addr= %llu, size.w = %d, size.h = %d, align = %u, trd_right_addr = %u\n", \
                i, fb->addr[i], fb->size[i].width, fb->size[i].height,
                fb->align[i], fb->trd_right_addr[i]);
    }
    printf("format = %x,color_space = %x, pre_multiply = %x, flags= %x, scan = %x \n", \
            fb->format, fb->color_space, fb->pre_multiply,fb->flags, fb->scan);
    printf("crop x = %lld, y = %lld, w = %lld, h = %lld\n", fb->crop.x, \
            fb->crop.y, fb->crop.width, fb->crop.height);

    return 0;
}
int print_disp_layer_info (struct disp_layer_info *info)
{
    printf("%s, \n", __func__);

    printf("mode = %x, zorder = %x, alpha_mode = %x, alpha_value = %x,\
            b_trd_out = %x, out_trd_mode = %x,\
            id = %x\n", info->mode, info->zorder, info->alpha_mode, info->alpha_value,\
            info->b_trd_out, info->out_trd_mode, info->id);

    print_disp_fb_info(&info->fb);

    return 0;
}
int print_disp_layer_config(struct disp_layer_config *config, int disp)
{
    printf("%s, disp = %d\n", __func__, disp);

    printf("enable = %x, channel = %x, layer_id = %x\n", config->enable, config->channel, config->layer_id);

    print_disp_layer_info(&config->info);
    return 0;
}

VideoLayer::VideoLayer()
{

    int ret;
    //open disp
    disp_fd = open("/dev/disp", O_RDWR);
    if(disp_fd == -1){
        printf("openf failed\n");
        exit(-1);
    }

    unsigned long buf[4]{};
    buf[0] = 0;//disp
    buf[1] = 1;//enable

    ret = ioctl(disp_fd, DISP_VSYNC_EVENT_EN, &buf[0]);
    if(ret != 0){
        printf("DISP_VSYNC_EVENT_EN failed\n");
        exit(-1);
    }

    buf[0] = 1;//disp
    buf[1] = 2;//output mode
    buf[2] = 14;//output type

    ret = ioctl(disp_fd, DISP_DEVICE_SWITCH, &buf[0]);
    if(ret != 0){
        printf("DISP_DEVICE_SWITCH failed\n");
        exit(-1);
    }

}

#include "sunxiMemInterface.h"
#include "G2dApi.h"

static int alloc_nv41_mem(int w, int h,paramStruct_t*pops)
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


int VideoLayer::set_layer(VideoLayer::LayerCon *layer_config)
{
    int ret;
    unsigned long buf[4]{};
    //set fb0
    struct disp_layer_config config {};

    config.enable = layer_config->enable;
    config.channel = 0;
    config.layer_id = layer_config->layer_id;

    config.info.zorder = layer_config->zorder;

    config.info.screen_win.x = layer_config->dst_x;
    config.info.screen_win.y = layer_config->dst_y;
    config.info.screen_win.width = layer_config->dst_w;
    config.info.screen_win.height = layer_config->dst_h;

#if 1
    config.info.alpha_mode = 1;
    config.info.alpha_value = 255;
    uint32_t phy_addr = (uint32_t)layer_config->phy_addr;

    unsigned int ori_w = layer_config->ori_w;
    unsigned int ori_h = layer_config->ori_h;

    config.info.fb.addr[0] = phy_addr;
    config.info.fb.addr[1] = phy_addr + ori_w * ori_h;
    config.info.fb.addr[2] = phy_addr + ori_w / 2* ori_h /2;
    config.info.fb.size[0].width = ori_w;
    config.info.fb.size[0].height = ori_h;
    config.info.fb.size[1].width = ori_w / 2;
    config.info.fb.size[1].height = ori_h /2;
    config.info.fb.size[2].width = ori_w / 2;
    config.info.fb.size[2].height = ori_h / 2;
    //config.info.fb.color_space = 0;

    config.info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
    config.info.fb.crop.x = 0;
    config.info.fb.crop.y = 0;
#if 1
    config.info.fb.crop.width = ((uint64_t)ori_w)<<32;
    config.info.fb.crop.height = ((uint64_t)ori_h)<<32;
#endif
#endif
#ifdef DISP_DEBUG
    print_disp_layer_config(&config, 0);
#endif


    buf[0] = 1;//disp
    buf[1] = (unsigned long )&config;//output mode
    buf[2] = 1;//output type

    ret = ioctl(disp_fd, DISP_LAYER_SET_CONFIG, &buf[0]);

    if(ret != 0)
    {
        printf("DISP_LAYER_SET_CONFIG failed\n");
        exit(-1);
    }
}


VideoLayer::~VideoLayer()
{

}
