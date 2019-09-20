#ifndef __VIDEO_LAYER_H__
#define __VIDEO_LAYER_H__

class VideoLayer{

public:
    struct LayerCon{
        unsigned int phy_addr;
        int ori_w;
        int ori_h;
        int dst_x;
        int dst_y;
        int dst_w;
        int dst_h;
        int layer_id;
        int enable;
        int zorder;
    };
    VideoLayer();
    int set_layer(LayerCon *config2);
    ~VideoLayer();
private:
    int disp_fd{};
};
#endif
