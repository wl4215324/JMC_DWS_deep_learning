#ifndef __G2D_COPY_H__
#define __G2D_COPY_H__
#include "sunxiMemInterface.h"
class G2dCopy{
int handle;
public:
    G2dCopy();
    ~G2dCopy();
    int scale(unsigned char *src, int src_width, int src_height, unsigned char *dst, int dst_width, int dst_height);
    int clip(void* psrc, int src_w, int src_h, int src_x, int src_y, int width, int height, void* pdst, int dst_w, int dst_h, int dst_x, int dst_y);

    int alloc_nv41_mem(int w, int h,paramStruct_t*pops);
    int alloc_free(paramStruct_t*pops);
    int flush_cache(paramStruct_t*pops);



};

#endif
