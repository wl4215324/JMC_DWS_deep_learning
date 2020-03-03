/*
 * video_encode.h
 *
 *  Created on: Nov 3, 2019
 *      Author: tony
 */

#ifndef VIDEO_ENCODE_H_
#define VIDEO_ENCODE_H_


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "vencoder.h"

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
    unsigned int bit_rate;
    unsigned int frame_rate;
    unsigned int maxKeyFrame;
}encode_param_t;



typedef struct {
	VideoEncoder *VideoEnc;
	VencBaseConfig baseConfig;
	encode_param_t encode_param;
    VencInputBuffer inputBuffer;
    VencOutputBuffer outputBuffer;
    VencHeaderData sps_pps_data;
} T7_Video_Encode;


#define ROI_NUM 4
#define NO_READ_WRITE 0

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


extern T7_Video_Encode* init_video_encoder(uint32_t src_width, uint32_t src_height, uint32_t dst_width, uint32_t dst_height, \
		uint8_t bit_rate, uint8_t frame_rate, VENC_CODEC_TYPE encoder_type);

void destroy_video_encoder(T7_Video_Encode **video_encoder);

int encode_video_frame_according_to_vir_addr(T7_Video_Encode *video_encoder, uint8_t *AddrVirY, \
		uint8_t *AddrVirC);





#endif /* VIDEO_ENCODE_H_ */
