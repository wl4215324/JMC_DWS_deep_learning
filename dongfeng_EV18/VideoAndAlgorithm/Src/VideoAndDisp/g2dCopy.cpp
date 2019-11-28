#include "g2dCopy.h"
#include "G2dApi.h"
G2dCopy::G2dCopy()
{
    handle = g2dInit();


}

G2dCopy::~G2dCopy()
{
    g2dUnit(handle);

}

int G2dCopy::scale(unsigned char *src, int src_width, int src_height, unsigned char *dst, int dst_width, int dst_height)
{

    g2dScale(handle,src, src_width, src_height, dst, dst_width, dst_height);
    return 0;
}

int G2dCopy::clip(void *psrc, int src_w, int src_h, int src_x, int src_y, int width, int height, void *pdst, int dst_w, int dst_h, int dst_x, int dst_y)
{

    g2dClip(handle, psrc, src_w, src_h, src_x, src_y, width, height, pdst, dst_w, dst_h, dst_x, dst_y);
    return 0;
}

int G2dCopy::alloc_nv41_mem(int w, int h,paramStruct_t*pops)
{
    int iRet;
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
    return 0;
}

int G2dCopy::alloc_free(paramStruct_t *pops)
{
    allocFree(MEM_TYPE_DMA, pops, NULL);
    return 0;
}

int G2dCopy::flush_cache(paramStruct_t*pops)
{
    flushCache(MEM_TYPE_DMA,pops, NULL);
    return 0;
}
