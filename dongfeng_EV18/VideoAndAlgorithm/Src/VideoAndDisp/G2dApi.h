#ifndef __G2DAPI_H__
#define __G2DAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ROTATE90_DISP

typedef struct RECT
{
    int left;
    int top;
    int width;            // preview width
    int height;            // preview height
}stRECT;

typedef enum {
    G2D_ROTATE90,
    G2D_ROTATE180,
    G2D_ROTATE270,
    G2D_FLIP_HORIZONTAL,
    G2D_FLIP_VERTICAL,
    G2D_MIRROR45,
    G2D_MIRROR135,
}g2dRotateAngle;


struct G2dOpsS{
    int (*fpG2dInit)();
    int (*fpG2dUnit)(int g2dHandle);
    int (*fpG2dScale)(int g2dHandle,unsigned char *src, int src_width, int src_height, unsigned char *dst, int dst_width, int dst_height);
    int (*fpG2dClip)(int g2dHandle,void* psrc, int src_w, int src_h, int src_x, int src_y, int width, int height, void* pdst, int dst_w, int dst_h, int dst_x, int dst_y);
    int (*fpG2dRotate)(int g2dHandle,g2dRotateAngle angle, unsigned char *src, int src_width, int src_height, unsigned char *dst, int dst_width, int dst_height);
};

struct G2dOpsS* GetG2dOpsS();
int g2dInit();
int g2dUnit(int g2dHandle);
int g2dClip(int g2dHandle,void* psrc, int src_w, int src_h, int src_x, int src_y, int width, int height, void* pdst, int dst_w, int dst_h, int dst_x, int dst_y);
int g2dScale(int g2dHandle,unsigned char *src, int src_width, int src_height, unsigned char *dst, int dst_width, int dst_height);
int g2dRotate(int g2dHandle,g2dRotateAngle angle, unsigned char *src, int src_width, int src_height, unsigned char *dst, int dst_width, int dst_height);

#ifdef __cplusplus
}
#endif


#endif
