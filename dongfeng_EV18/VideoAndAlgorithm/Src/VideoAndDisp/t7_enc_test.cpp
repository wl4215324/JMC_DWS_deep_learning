#include "t7_enc_test.h"
#include <iostream>
#include "t7_enc.h"
#include "spdlog/spdlog.h"

using namespace  std;


//ofstream out_f{"test.h264"};
ofstream *out_f[4]{};
int current_frame[4]{};
int total_time[4]{};
T7Venc *encs[4]{};


int bit_stream_write(VencOutputBuffer *outputBuffer, void *op)
{
    //spdlog::info("bit_stream_write size {}, pts = {}", outputBuffer->nSize0, outputBuffer->nPts);
    int i = *(int *)op;
    out_f[i]->write(outputBuffer->pData0, outputBuffer->nSize0);
    return 0;
}


int test_func(int8_t **bufs, int buf_size, int index)
{
    T7Venc *enc = encs[index];
    enc->set_bs_cb_func(&bit_stream_write, (void *)index);
    //enc->get_sps_pps();

    //for(int i = 0; i < 30*20 && !enc->stop; i++){
    for(int i = 0; !enc->stop; i++)
    {
        auto t = steady_clock::now();

#if 0
        enc->encode_a_frame(bufs[i % 10], buf_size);
#else
        VideoFrame frame;
        frame.phy_addr = 0;
        frame.size = buf_size;
        frame.vir_buf = bufs[i % 10];
        frame.pts = 0;
        enc->encode_a_frame_phy(&frame);
#endif
        auto d = steady_clock::now()- t;
        //cout<<index<<" encode_a_frame took " << duration_cast<milliseconds>(d).count() << "ms "<<endl; // print as
        current_frame[index]++;
        total_time[index] += duration_cast<milliseconds>(d).count();
        if(current_frame[index]% 60 == 59)
        {
            cout<<index<<" current fps = "<<1000/(total_time[index]/ current_frame[index])<<endl;

        }
    }

}

void testStop(int index)
{

    for(int i= 0; i < 4; i++)
    {
        encs[i]->stop = true;
    }
}


int venc_test()
{
    signal(SIGINT, testStop);
#if 1
    int buf_size = 1280*720*1.5;
    int8_t *bufs[10]{};

    //read file set buf
    ifstream in_f{"720p.yuv"};
    for(int i = 0; i < 10; i++)
    {
        bufs[i] = new int8_t[buf_size];
        in_f.read(bufs[i], buf_size);  //read yuv raw data into buffers
        //out_f[i] = new ofstream{"test.h264"};
    }

    //start thread and open out file
    thread *t[4]{};

    encode_param_t encode_param;
    encode_param.bit_rate = 1*1024*1024;
    encode_param.src_width = 1280;
    encode_param.src_height = 720;
    encode_param.dst_width = 1280;
    encode_param.dst_height = 720;
    encode_param.frame_rate = 30;
    encode_param.maxKeyFrame = 30;
    encode_param.encode_format = VENC_CODEC_H264;

    //for(int i = 0; i < 4; i++){
    for(int i = 0; i < 1; i++)
    {
        //if(i ==0){

            out_f[i] = new ofstream{string("test") +to_string(i) + ".h264", ios_base::trunc};
            encs[i] = new T7Venc(encode_param);
            vector<int8_t> buf;
            encs[i]->get_sps_pps(buf);
            spdlog::info("get_sps_pps size = {}", buf.size());
            out_f[i]->write(buf.data(), buf.size());
            t[i] = new thread{test_func, bufs, buf_size, i};
        //}
    }
    //join all thread
    for(int i = 0; i < 4; i++)
    {
        //if(i ==0){
            t[i]->join();
            out_f[i]->close();
        //}
    }

#endif
    return 0;
}

int t7_jpeg_test()
{

}


static VencBaseConfig baseConfig;
static int gWidth = 1280;
static int gHeight = 720;
static VideoEncoder *gVideoEnc = NULL;
FILE *fpH264 = NULL;


int init_h264_encode()
{
	int value;
	fpH264 = fopen("/extp/video.h264", "w");

    VencH264Param h264Param;
    //* h264 param
    h264Param.bEntropyCodingCABAC = 1;  // 0:CAVLC 1:CABAC
    h264Param.nBitrate = 3 * 1024 * 1024;  //3MB
    h264Param.nFramerate = 25;  //25fps
    h264Param.nCodingMode = VENC_FRAME_CODING;
    //h264Param.nCodingMode = VENC_FIELD_CODING;
    h264Param.nMaxKeyInterval = 30;
    h264Param.sProfileLevel.nProfile = VENC_H264ProfileMain;
    h264Param.sProfileLevel.nLevel = VENC_H264Level31;
    h264Param.sQPRange.nMinqp = 10;
    h264Param.sQPRange.nMaxqp = 40;
    memset(&baseConfig, 0, sizeof(VencBaseConfig));

    if (baseConfig.memops == NULL)
    {
        baseConfig.memops = MemAdapterGetOpsS();

        if (baseConfig.memops == NULL)
        {
            printf("MemAdapterGetOpsS failed\n");
            return -1;
        }

        CdcMemOpen(baseConfig.memops);
    }

    baseConfig.nInputWidth = gWidth;  //width of input original image
    baseConfig.nInputHeight = gHeight;  //height of input original image
    baseConfig.nStride = gWidth;
    baseConfig.nDstWidth = 720; //gWidth;  //width of image to be encoded
    baseConfig.nDstHeight = 576; //gHeight;  //height of image to be encoded
    baseConfig.eInputFormat = VENC_PIXEL_YVU420SP;  //color format of input image

    if (gVideoEnc == NULL)
    {
        printf("get SPS PPS\n");
        gVideoEnc = VideoEncCreate((VENC_CODEC_TYPE)VENC_CODEC_H264);  //create a video coder
        VideoEncSetParameter(gVideoEnc, VENC_IndexParamH264Param, &h264Param);
        value = 0;
        VideoEncSetParameter(gVideoEnc, VENC_IndexParamIfilter, &value);
        value = 0; //degree
        VideoEncSetParameter(gVideoEnc, VENC_IndexParamRotation, &value);
        value = 0;
        VideoEncSetParameter(gVideoEnc, VENC_IndexParamSetPSkip, &value);
        VideoEncInit(gVideoEnc, &baseConfig);  //init video coder
    }

    VencHeaderData sps_pps_data;
    VideoEncGetParameter(gVideoEnc, VENC_IndexParamH264SPSPPS, &sps_pps_data);
    fwrite(sps_pps_data.pBuffer, 1, sps_pps_data.nLength, fpH264);

    VencAllocateBufferParam bufferParam;
    VencInputBuffer inputBuffer;
    memset(&bufferParam, 0, sizeof(VencAllocateBufferParam));
    memset(&inputBuffer, 0, sizeof(VencInputBuffer));

    bufferParam.nSizeY = baseConfig.nInputWidth * baseConfig.nInputHeight;  //buffer size
    bufferParam.nSizeC = baseConfig.nInputWidth * baseConfig.nInputHeight / 2;
    bufferParam.nBufferNum = 1;  // count of buffer
    AllocInputBuffer(gVideoEnc, &bufferParam); //apply buffer for input image

	return 0;
}


int free_h264_encode()
{
    if (baseConfig.memops != NULL)
    {
        CdcMemClose(baseConfig.memops);
        baseConfig.memops = NULL;
    }

    if(!gVideoEnc)
    {
        ReleaseAllocInputBuffer(gVideoEnc);
        VideoEncDestroy(gVideoEnc);
        gVideoEnc = NULL;
    }

    return 0;
}


int H264EncodeOneFrame(unsigned char *AddrVirY, unsigned char *AddrVirC, FILE *fpH264)
{
    int result = 0;
    VencInputBuffer inputBuffer;
    VencOutputBuffer outputBuffer;
    unsigned int head_num = 0;

    if(GetOneAllocInputBuffer(gVideoEnc, &inputBuffer) == -1)
    {
    	perror("GetOneAllocInputBuffer error: ");
    	return -1;
    }

    memcpy(inputBuffer.pAddrVirY, AddrVirY, baseConfig.nInputWidth * baseConfig.nInputHeight);
    memcpy(inputBuffer.pAddrVirC, AddrVirC, baseConfig.nInputWidth * baseConfig.nInputHeight / 2);

    inputBuffer.bEnableCorp = 0;
    inputBuffer.sCropInfo.nLeft = 240;
    inputBuffer.sCropInfo.nTop = 240;
    inputBuffer.sCropInfo.nWidth = 240;
    inputBuffer.sCropInfo.nHeight = 240;
    FlushCacheAllocInputBuffer(gVideoEnc, &inputBuffer);
    AddOneInputBuffer(gVideoEnc, &inputBuffer);
    inputBuffer.nPts++;

    if (VENC_RESULT_OK != VideoEncodeOneFrame(gVideoEnc))
    {
        printf("VideoEncodeOneFrame failed.\n");
        return -1;
    }

    AlreadyUsedInputBuffer(gVideoEnc, &inputBuffer);
    ReturnOneAllocInputBuffer(gVideoEnc, &inputBuffer);
    GetOneBitstreamFrame(gVideoEnc, &outputBuffer);

    if (outputBuffer.nSize0 > 0)
    {
        printf("write pData0\n");
        fwrite(outputBuffer.pData0, 1, outputBuffer.nSize0, fpH264);
    }

    if (outputBuffer.nSize1 > 0)
    {
        printf("write pData1\n");
        fwrite(outputBuffer.pData1, 1, outputBuffer.nSize1, fpH264);
    }

    FreeOneBitStreamFrame(gVideoEnc, &outputBuffer);
    return 0;
}


int encode_one_frame_h264(unsigned char *AddrVirY, unsigned char *AddrVirC)
{
	return H264EncodeOneFrame(AddrVirY, AddrVirC, fpH264);
}



