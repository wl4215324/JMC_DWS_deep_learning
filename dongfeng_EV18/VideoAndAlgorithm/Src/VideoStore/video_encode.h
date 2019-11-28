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


extern T7_Video_Encode* init_video_encoder(uint32_t src_width, uint32_t src_height, uint32_t dst_width, uint32_t dst_height, \
		uint8_t bit_rate, uint8_t frame_rate, VENC_CODEC_TYPE encoder_type);

void destroy_video_encoder(T7_Video_Encode **video_encoder);

int encode_video_frame_according_to_vir_addr(T7_Video_Encode *video_encoder, uint8_t *AddrVirY, \
		uint8_t *AddrVirC);





#endif /* VIDEO_ENCODE_H_ */
