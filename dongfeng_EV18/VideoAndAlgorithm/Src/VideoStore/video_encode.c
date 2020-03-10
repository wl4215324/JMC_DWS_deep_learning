/*
 * video_encode.cpp
 *
 *  Created on: Nov 3, 2019
 *      Author: tony
 */

#include "video_encode.h"
#include "video_encode_attribute.h"


T7_Video_Encode* init_video_encoder(uint32_t src_width, uint32_t src_height, uint32_t dst_width, uint32_t dst_height, \
		uint8_t bit_rate, uint8_t frame_rate, VENC_CODEC_TYPE encoder_type)
{
	T7_Video_Encode *t7_video_encode = (T7_Video_Encode*)malloc(sizeof(T7_Video_Encode));

	if(!t7_video_encode)
	{
		perror("malloc video_encoder error: ");
		goto init_error_exit;
	}

	if(!(t7_video_encode->VideoEnc = VideoEncCreate((VENC_CODEC_TYPE)encoder_type)))  // create encoder error
	{
		perror("VideoEncCreate error: ");
		goto init_error_exit;
	}

	t7_video_encode->encode_param.encode_format = encoder_type;
	t7_video_encode->encode_param.src_width = src_width;
	t7_video_encode->encode_param.src_height = src_height;
	t7_video_encode->encode_param.dst_width = dst_width;
	t7_video_encode->encode_param.dst_height = dst_height;
	t7_video_encode->encode_param.bit_rate = bit_rate;
	t7_video_encode->encode_param.frame_rate = frame_rate;

	if(set_params_into_encoder(t7_video_encode->VideoEnc, &(t7_video_encode->encode_param)) < 0)  //failed to set params for encoder
	{
		perror("set_params_into_encoder error: ");
		goto init_error_exit;
	}

	memset(&(t7_video_encode->baseConfig), 0, sizeof(VencBaseConfig));
	t7_video_encode->baseConfig.memops = MemAdapterGetOpsS();

	if(!t7_video_encode->baseConfig.memops)
	{
		goto init_error_exit;
	}

	CdcMemOpen(t7_video_encode->baseConfig.memops);
	t7_video_encode->baseConfig.nInputWidth = src_width;
	t7_video_encode->baseConfig.nInputHeight = src_height;
	t7_video_encode->baseConfig.nStride = src_width;
	t7_video_encode->baseConfig.nDstWidth = dst_width;
	t7_video_encode->baseConfig.nDstHeight = dst_height;
	VideoEncInit(t7_video_encode->VideoEnc, &t7_video_encode->baseConfig);

    VencAllocateBufferParam bufferParam;
    memset(&bufferParam, 0, sizeof(VencAllocateBufferParam));
    bufferParam.nSizeY = t7_video_encode->baseConfig.nInputWidth * t7_video_encode->baseConfig.nInputHeight; //buffer size
    bufferParam.nSizeC = bufferParam.nSizeY / 2;
    bufferParam.nBufferNum = 1;  // count of buffer

    //apply buffer for input image
    if(AllocInputBuffer(t7_video_encode->VideoEnc, &bufferParam) < 0)
    {
    	goto init_error_exit;
    }


    //VideoEncGetParameter(t7_video_encode->VideoEnc, VENC_IndexParamH264SPSPPS, &(t7_video_encode->sps_pps_data));

    //get sps_pps_data
    if(t7_video_encode->encode_param.encode_format == VENC_CODEC_H264)
    {
        VideoEncGetParameter(t7_video_encode->VideoEnc, VENC_IndexParamH264SPSPPS, &(t7_video_encode->sps_pps_data));
        unsigned char value = 1;
        //VideoEncGetParameter(pVideoEnc, VENC_IndexParamAllParams, &value);
    }
    else if(t7_video_encode->encode_param.encode_format == VENC_CODEC_H265)
    {
        VideoEncGetParameter(t7_video_encode->VideoEnc, VENC_IndexParamH265Header, &(t7_video_encode->sps_pps_data));
        unsigned char value = 1;
        //VideoEncGetParameter(pVideoEnc, VENC_IndexParamAllParams, &value);
    }

	return t7_video_encode;

init_error_exit:
    if(t7_video_encode)
    {
    	if(t7_video_encode->baseConfig.memops)
    	{
	        CdcMemClose(t7_video_encode->baseConfig.memops);
	        t7_video_encode->baseConfig.memops = NULL;
    	}

        VideoEncDestroy(t7_video_encode->VideoEnc);
        t7_video_encode = NULL;
    }

    return (T7_Video_Encode *)(-1);
}



void destroy_video_encoder(T7_Video_Encode **video_encoder)
{
	if(*video_encoder)
	{
		if((*video_encoder)->baseConfig.memops)
		{
	        CdcMemClose((*video_encoder)->baseConfig.memops);
	        (*video_encoder)->baseConfig.memops = NULL;
		}

        ReleaseAllocInputBuffer((*video_encoder)->VideoEnc);
        VideoEncDestroy((*video_encoder)->VideoEnc);
		*video_encoder = NULL;
	}
}


int encode_video_frame_according_to_vir_addr(T7_Video_Encode *t7_video_encode, uint8_t *AddrVirY, \
		uint8_t *AddrVirC)
{
	unsigned int i = 0;
    unsigned int size  = 0;

    if(GetOneAllocInputBuffer(t7_video_encode->VideoEnc, &t7_video_encode->inputBuffer) == -1)
    {
    	perror("GetOneAllocInputBuffer error: ");
    	return -1;
    }

    size = t7_video_encode->baseConfig.nInputWidth * t7_video_encode->baseConfig.nInputHeight;
    memcpy(t7_video_encode->inputBuffer.pAddrVirY, AddrVirY, size);
    memcpy(t7_video_encode->inputBuffer.pAddrVirC, AddrVirC, size / 2);

    t7_video_encode->inputBuffer.bEnableCorp = 0;
    t7_video_encode->inputBuffer.sCropInfo.nLeft = 240;
    t7_video_encode->inputBuffer.sCropInfo.nTop = 240;
    t7_video_encode->inputBuffer.sCropInfo.nWidth = 240;
    t7_video_encode->inputBuffer.sCropInfo.nHeight = 240;
    FlushCacheAllocInputBuffer(t7_video_encode->VideoEnc, &t7_video_encode->inputBuffer);
    AddOneInputBuffer(t7_video_encode->VideoEnc, &t7_video_encode->inputBuffer);
    (t7_video_encode->inputBuffer).nPts += 1;

    if (VENC_RESULT_OK != VideoEncodeOneFrame(t7_video_encode->VideoEnc))
    {
        perror("VideoEncodeOneFrame failed ");
        return -1;
    }

    AlreadyUsedInputBuffer(t7_video_encode->VideoEnc, &t7_video_encode->inputBuffer);
    ReturnOneAllocInputBuffer(t7_video_encode->VideoEnc, &t7_video_encode->inputBuffer);
    GetOneBitstreamFrame(t7_video_encode->VideoEnc, &t7_video_encode->outputBuffer);

#if 0
    if (t7_video_encode->outputBuffer.nSize0 > 0)
    {
        printf("write pData0\n");

        for(i=0; i<200; i++)
        {
        	printf("%02X ", *(t7_video_encode->outputBuffer.pData0 + i));
        }

        printf("\n");
        //fwrite(outputBuffer.pData0, 1, outputBuffer.nSize0, fpH264);
    }

    if (t7_video_encode->outputBuffer.nSize1 > 0)
    {
        printf("write pData1\n");
        //fwrite(outputBuffer.pData1, 1, outputBuffer.nSize1, fpH264);
    }
#endif

    //printf("nPts: %d, pData0 %02X \n", (t7_video_encode->outputBuffer).nPts, *(t7_video_encode->outputBuffer.pData0));
    FreeOneBitStreamFrame(t7_video_encode->VideoEnc, &t7_video_encode->outputBuffer);
    //return 0;
    return t7_video_encode->outputBuffer.nSize0 ;
}



int encode_video_frame_according_to_phy_addr(T7_Video_Encode *video_encoder, uint32_t image_phy_addr)
{
	return 0;
}


