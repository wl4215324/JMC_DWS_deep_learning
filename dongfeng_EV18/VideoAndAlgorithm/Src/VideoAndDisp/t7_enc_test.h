#ifndef __T7_ENC_TEST_H__
#define __T7_ENC_TEST_H__

#include <stdio.h>
#include <stdlib.h>

int venc_test();

int init_h264_encode();
int free_h264_encode();
int H264EncodeOneFrame(unsigned char *AddrVirY, unsigned char *AddrVirC, FILE *fpH264);
int encode_one_frame_h264(unsigned char *AddrVirY, unsigned char *AddrVirC);

#endif
