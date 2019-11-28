#include "video_layer.h"
#include "sunxiMemInterface.h"
#include "G2dApi.h"

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

int video_layer_test()
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
    memset((void *)pops.vir, 0, 1280*720*1.5);

    conf.dst_x = 0;
    conf.dst_y = 0;
    conf.dst_w = 1280;
    conf.dst_h = 720;
    conf.ori_w = 1280;
    conf.ori_h = 720;
    conf.layer_id = 0;
    conf.phy_addr =  pops.phy;
    conf.enable = 1;
    conf.zorder = 2;

    VideoLayer layer{};
    layer.set_layer(&conf);


    VideoLayer::LayerCon conf2;

    alloc_nv41_mem(1280, 720, &pops);
    conf2.dst_x = 0;
    conf2.dst_y = 0;
    conf2.dst_w = 1280;
    conf2.dst_h = 720;
    conf2.ori_w = 1280;
    conf2.ori_h = 720;
    conf2.layer_id = 0;
    conf2.phy_addr =  pops.phy;
    conf2.enable = 1;
    conf2.zorder = 2;
    memset((void *)pops.vir, 0xff, 1280*720*1.5);

    while(1){
#if 0
    memset((void *)pops.vir, 0xff/2, 1280*720*1.5);
    usleep(10000);
    memset((void *)pops.vir, 0xff, 1280*720*1.5);
    usleep(10000);
#else
    layer.set_layer(&conf);
    usleep(22000);
    layer.set_layer(&conf2);
    usleep(22000);
#endif

    }
    return 0;
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
    conf.dst_h = 480;  //480
    conf.phy_addr = phy_addr;
    //printf("frm->phy_addr %x\n", conf.phy_addr);
    conf.ori_w = 1280;
    conf.ori_h = 720;
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
