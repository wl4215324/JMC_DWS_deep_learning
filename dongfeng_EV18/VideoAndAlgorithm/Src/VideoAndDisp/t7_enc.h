#ifndef __T7_ENC_H__
#define __T7_ENC_H__
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <string>
#include <vector>
#include <stdint.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "v4l2_video.h"


extern "C"{
#include <memoryAdapter.h>
#include "vencoder.h"
#include "t7_common_data.h"
}
#include <chrono>
using namespace chrono;
using namespace std;

#define ROI_NUM 4
#define NO_READ_WRITE 0
//#define YU12_NV12

//#define USE_SVC
//#define USE_VIDEO_SIGNAL
//#define USE_ASPECT_RATIO
//#define USE_SUPER_FRAME

//#define GET_MB_INFO
//#define SET_MB_INFO
//#define SET_SMART

#define ALIGN_XXB(y, x) (((x) + ((y)-1)) & ~((y)-1))

#define logd printf
#define logw printf
#define loge printf
#define logv printf
#define my_printf() logd("func:%s, line:%d\n", __func__, __LINE__)


typedef struct {
    unsigned int  encode_format;
    unsigned int src_width;
    unsigned int src_height;
    unsigned int dst_width;
    unsigned int dst_height;

    int bit_rate;
    int frame_rate;
    int maxKeyFrame;
}encode_param_t;


typedef enum {
    INPUT,
    HELP,
    ENCODE_FRAME_NUM,
    ENCODE_FORMAT,
    OUTPUT,
    SRC_SIZE,
    DST_SIZE,
    COMPARE_FILE,
    BIT_RATE,
    INVALID
}ARGUMENT_T;


typedef struct {
    char Short[8];
    char Name[128];
    ARGUMENT_T argument;
    char Description[512];
}argument_t;

//gl_display_test venc -i ./720p.yuv  -n 300 -f 0 -o ./720.h264 -s 720 -d 720

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int width_aligh16;
    unsigned int height_aligh16;
    unsigned char* argb_addr;
    unsigned int size;
}BitMapInfoS;

typedef struct {
    EXIFInfo                exifinfo;
    int                     quality;
    int                     jpeg_mode;
    VencJpegVideoSignal     vs;
    int                     jpeg_biteRate;
    int                     jpeg_frameRate;
    VencBitRateRange        bitRateRange;
    VencOverlayInfoS        sOverlayInfo;
}jpeg_func_t;


typedef struct {
    VencHeaderData          sps_pps_data;
    VencH264Param           h264Param;
    VencMBModeCtrl          h264MBMode;
    VencMBInfo              MBInfo;
    VencH264FixQP           fixQP;
    VencSuperFrameConfig    sSuperFrameCfg;
    VencH264SVCSkip         SVCSkip; // set SVC and skip_frame
    VencH264AspectRatio     sAspectRatio;
    VencH264VideoSignal     sVideoSignal;
    VencCyclicIntraRefresh  sIntraRefresh;
    VencROIConfig           sRoiConfig[ROI_NUM];
    VeProcSet               sVeProcInfo;
    VencOverlayInfoS        sOverlayInfo;
    VencSmartFun            sH264Smart;
}h264_func_t;


typedef struct {
    VencH265Param               h265Param;
    VencH265GopStruct           h265Gop;
    VencHVS                     h265Hvs;
    VencH265TendRatioCoef       h265Trc;
    VencSmartFun                h265Smart;
    VencMBModeCtrl              h265MBMode;
    VencMBInfo                  MBInfo;
    VencH264FixQP               fixQP;
    VencSuperFrameConfig        sSuperFrameCfg;
    VencH264SVCSkip             SVCSkip; // set SVC and skip_frame
    VencH264AspectRatio         sAspectRatio;
    VencH264VideoSignal         sVideoSignal;
    VencCyclicIntraRefresh      sIntraRefresh;
    VencROIConfig               sRoiConfig[ROI_NUM];
    VencAlterFrameRateInfo sAlterFrameRateInfo;
    int                         h265_rc_frame_total;
    VeProcSet               sVeProcInfo;
    VencOverlayInfoS        sOverlayInfo;
}h265_func_t;

typedef  int (*bs_cb_func) (VencOutputBuffer *outputBuffer, void *op);
class T7Venc{
public:
    T7Venc(encode_param_t parm);
    ~T7Venc();
    int encode_a_frame(uint8_t *buf, int size);

    int encode_a_frame_phy(VideoFrame *frame);
    int set_bs_cb_func(bs_cb_func func, void *op){bs_cb = func; callback_op = op;}
    int get_sps_pps(vector<int8_t> &buf);
    bool stop{false};
private:
    vector<int8_t> sps_pps_buf;
    bs_cb_func bs_cb;
    void *callback_op;
    encode_param_t    encode_param;
    VencBaseConfig baseConfig;
    VencAllocateBufferParam bufferParam;
    VencHeaderData sps_pps_data;
    VencInputBuffer inputBuffer{};
    VencOutputBuffer outputBuffer;
    VideoEncoder* pVideoEnc;

    void init_mb_mode(VencMBModeCtrl *pMBMode, encode_param_t *encode_param);
    void init_mb_info(VencMBInfo *MBInfo, encode_param_t *encode_param);
    int initH264Func(h264_func_t *h264_func, encode_param_t *encode_param);
    int initH265Func(h265_func_t *h265_func, encode_param_t *encode_param);
    int initJpegFunc(jpeg_func_t *jpeg_func, encode_param_t *encode_param);
    int setEncParam(VideoEncoder *pVideoEnc ,encode_param_t *encode_param);
    void setMbMode(VideoEncoder *pVideoEnc, encode_param_t *encode_param);
    void getMbMinfo(VideoEncoder *pVideoEnc, encode_param_t *encode_param);
    void releaseMb(encode_param_t *encode_param);
    void init_overlay_info(VencOverlayInfoS *pOverlayInfo);

    jpeg_func_t jpeg_func;
    h264_func_t h264_func;
    h265_func_t h265_func;
    BitMapInfoS bit_map_info[13]{};
};


#endif
