#ifndef __GL_DISPLAY_H__
#define __GL_DISPLAY_H__
#include <stdint.h>
class FbDisplay{
public:
    enum DIS_FMT{
        FB_DIS_GRAY,
        FB_DIS_RGB32,
        FB_DIS_NV21,
    };
    FbDisplay(DIS_FMT fmt);
    ~FbDisplay();
    int display_1280_720_nv21(char *buffer);
    struct BufData{
        char *buf;
        int w;
        int h;
    };
    struct DisRect{
        float x;
        float y;
        float w;
        float h;
    };
    int draw_pic_nv21(BufData *buf, DisRect *rect);
    int flush_pic_nv21();
private:
    DIS_FMT dis_fmt;
    unsigned int LoadTexture( char *fileName );
    int display_init();
    int setVertices(FbDisplay::DisRect *rect);

    uint32_t program;
};
#endif
