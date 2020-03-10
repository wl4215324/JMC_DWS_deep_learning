/*
 * video_encode_attribute.c
 *
 *  Created on: Mar 3, 2020
 *      Author: tony
 */

#include "video_encode_attribute.h"
#include "vencoder.h"

jpeg_func_t jpeg_func;
h264_func_t h264_func;
h265_func_t h265_func;
BitMapInfoS bit_map_info[13];


void init_jpeg_exif(EXIFInfo *exifinfo)
{
    exifinfo->ThumbWidth = 640;
    exifinfo->ThumbHeight = 480;

    strcpy((char*)exifinfo->CameraMake,        "allwinner make test");
    strcpy((char*)exifinfo->CameraModel,        "allwinner model test");
    strcpy((char*)exifinfo->DateTime,         "2014:02:21 10:54:05");
    strcpy((char*)exifinfo->gpsProcessingMethod,  "allwinner gps");

    exifinfo->Orientation = 0;

    exifinfo->ExposureTime.num = 2;
    exifinfo->ExposureTime.den = 1000;

    exifinfo->FNumber.num = 20;
    exifinfo->FNumber.den = 10;
    exifinfo->ISOSpeed = 50;

    exifinfo->ExposureBiasValue.num= -4;
    exifinfo->ExposureBiasValue.den= 1;

    exifinfo->MeteringMode = 1;
    exifinfo->FlashUsed = 0;

    exifinfo->FocalLength.num = 1400;
    exifinfo->FocalLength.den = 100;

    exifinfo->DigitalZoomRatio.num = 4;
    exifinfo->DigitalZoomRatio.den = 1;

    exifinfo->WhiteBalance = 1;
    exifinfo->ExposureMode = 1;

    exifinfo->enableGpsInfo = 1;

    exifinfo->gps_latitude = 23.2368;
    exifinfo->gps_longitude = 24.3244;
    exifinfo->gps_altitude = 1234.5;

    exifinfo->gps_timestamp = (long)time(NULL);

    strcpy((char*)exifinfo->CameraSerialNum,  "123456789");
    strcpy((char*)exifinfo->ImageName,  "exif-name-test");
    strcpy((char*)exifinfo->ImageDescription,  "exif-descriptor-test");
}

void init_jpeg_rate_ctrl(jpeg_func_t *jpeg_func)
{
    jpeg_func->jpeg_biteRate = 12*1024*1024;
    jpeg_func->jpeg_frameRate = 30;
    jpeg_func->bitRateRange.bitRateMax = 14*1024*1024;
    jpeg_func->bitRateRange.bitRateMin = 10*1024*1024;
}

void init_overlay_info(VencOverlayInfoS *pOverlayInfo)
{
    int i;
    unsigned char num_bitMap = 13;
    BitMapInfoS* pBitMapInfo;
    unsigned int time_id_list[19];
    unsigned int start_mb_x;
    unsigned int start_mb_y;

    memset(pOverlayInfo, 0, sizeof(VencOverlayInfoS));

#if 0
    char filename[64];
    int ret;
    for(i = 0; i < num_bitMap; i++)
    {
        FILE* icon_hdle = NULL;
        int width  = 0;
        int height = 0;

        sprintf(filename, "%s%d.bmp", "/mnt/libcedarc/bitmap/icon_720p_",i);

        icon_hdle   = fopen(filename, "r");
        if (icon_hdle == NULL) {
            printf("get wartermark %s error\n", filename);
            return;
        }

        //get watermark picture size
        fseek(icon_hdle, 18, SEEK_SET);
        fread(&width, 1, 4, icon_hdle);
        fread(&height, 1, 4, icon_hdle);

        fseek(icon_hdle, 54, SEEK_SET);

        bit_map_info[i].argb_addr = NULL;
        bit_map_info[i].width = 0;
        bit_map_info[i].height = 0;

        bit_map_info[i].width = width;
        bit_map_info[i].height = height*(-1);

        bit_map_info[i].width_aligh16 = ALIGN_XXB(16, bit_map_info[i].width);
        bit_map_info[i].height_aligh16 = ALIGN_XXB(16, bit_map_info[i].height);
        if(bit_map_info[i].argb_addr == NULL) {
            bit_map_info[i].argb_addr =
            (unsigned char*)malloc(bit_map_info[i].width_aligh16*bit_map_info[i].height_aligh16*4);

            if(bit_map_info[i].argb_addr == NULL)
            {
                loge("malloc bit_map_info[%d].argb_addr fail\n", i);
                return;
            }
        }
        logd("bitMap[%d] size[%d,%d], size_align16[%d, %d], argb_addr:%p\n", i,
                                    bit_map_info[i].width,
                                    bit_map_info[i].height,
                                    bit_map_info[i].width_aligh16,
                                    bit_map_info[i].height_aligh16,
                                    bit_map_info[i].argb_addr);

        ret = fread(bit_map_info[i].argb_addr, 1,
            bit_map_info[i].width*bit_map_info[i].height*4, icon_hdle);
        if(ret != bit_map_info[i].width*bit_map_info[i].height*4)
            loge("read bitMap[%d] error, ret value:%d\n", i, ret);

        bit_map_info[i].size = bit_map_info[i].width_aligh16 * bit_map_info[i].height_aligh16 * 4;

        if (icon_hdle) {
            fclose(icon_hdle);
            icon_hdle = NULL;
        }
    }

    //time 2017-04-27 18:28:26
    time_id_list[0] = 2;
    time_id_list[1] = 0;
    time_id_list[2] = 1;
    time_id_list[3] = 7;
    time_id_list[4] = 11;
    time_id_list[5] = 0;
    time_id_list[6] = 4;
    time_id_list[7] = 11;
    time_id_list[8] = 2;
    time_id_list[9] = 7;
    time_id_list[10] = 10;
    time_id_list[11] = 1;
    time_id_list[12] = 8;
    time_id_list[13] = 12;
    time_id_list[14] = 2;
    time_id_list[15] = 8;
    time_id_list[16] = 12;
    time_id_list[17] = 2;
    time_id_list[18] = 6;

    logd("pOverlayInfo:%p\n", pOverlayInfo);
    pOverlayInfo->blk_num = 19;
#else
        FILE* icon_hdle = NULL;
        int width  = 96;
        int height = 48;
        memset(time_id_list, 0 ,sizeof(time_id_list));
#if 0
        icon_hdle = fopen("/mnt/libcedarc/data_argb.dat", "r");
        if (icon_hdle == NULL) {
            printf("get icon_hdle error\n");
            return;
        }
#endif

        for(i = 0; i < num_bitMap; i++)
        {
            bit_map_info[i].argb_addr = NULL;
            bit_map_info[i].width = width;
            bit_map_info[i].height = height;

            bit_map_info[i].width_aligh16 = ALIGN_XXB(16, bit_map_info[i].width);
            bit_map_info[i].height_aligh16 = ALIGN_XXB(16, bit_map_info[i].height);
            if(bit_map_info[i].argb_addr == NULL) {
                bit_map_info[i].argb_addr =
            (unsigned char*)malloc(bit_map_info[i].width_aligh16*bit_map_info[i].height_aligh16*4);

                if(bit_map_info[i].argb_addr == NULL)
                {
                    loge("malloc bit_map_info[%d].argb_addr fail\n", i);
                    if (icon_hdle) {
                        fclose(icon_hdle);
                        icon_hdle = NULL;
                    }

                    return;
                }
            }
            logv("bitMap[%d] size[%d,%d], size_align16[%d, %d], argb_addr:%p\n", i,
                                                        bit_map_info[i].width,
                                                        bit_map_info[i].height,
                                                        bit_map_info[i].width_aligh16,
                                                        bit_map_info[i].height_aligh16,
                                                        bit_map_info[i].argb_addr);
            memset(bit_map_info[i].argb_addr, 1, bit_map_info[i].width*bit_map_info[i].height*4);
#if 0
            int ret;
            ret = fread(bit_map_info[i].argb_addr, 1,
                        bit_map_info[i].width*bit_map_info[i].height*4, icon_hdle);
            if(ret != bit_map_info[i].width*bit_map_info[i].height*4)
            loge("read bitMap[%d] error, ret value:%d\n", i, ret);
#endif

            bit_map_info[i].size
                = bit_map_info[i].width_aligh16 * bit_map_info[i].height_aligh16 * 4;
        }
        if (icon_hdle) {
            fclose(icon_hdle);
            icon_hdle = NULL;
        }
#endif
        pOverlayInfo->argb_type = VENC_OVERLAY_ARGB8888;
        pOverlayInfo->blk_num = 12;
        logd("blk_num:%d, argb_type:%d\n", pOverlayInfo->blk_num, pOverlayInfo->argb_type);

        start_mb_x = 0;
        start_mb_y = 0;
        for(i=0; i<pOverlayInfo->blk_num; i++)
        {
            //id = time_id_list[i];
            //pBitMapInfo = &bit_map_info[id];
            pBitMapInfo = &bit_map_info[i];

            pOverlayInfo->overlayHeaderList[i].start_mb_x = start_mb_x;
            pOverlayInfo->overlayHeaderList[i].start_mb_y = start_mb_y;
            pOverlayInfo->overlayHeaderList[i].end_mb_x = start_mb_x
                                        + (pBitMapInfo->width_aligh16 / 16 - 1);
            pOverlayInfo->overlayHeaderList[i].end_mb_y = start_mb_y
                                        + (pBitMapInfo->height_aligh16 / 16 -1);

            pOverlayInfo->overlayHeaderList[i].extra_alpha_flag = 0;
            pOverlayInfo->overlayHeaderList[i].extra_alpha = 8;
            if(i%3 == 0)
                pOverlayInfo->overlayHeaderList[i].overlay_type = LUMA_REVERSE_OVERLAY;
            else if(i%2 == 0 && i!=0)
                pOverlayInfo->overlayHeaderList[i].overlay_type = COVER_OVERLAY;
            else
                pOverlayInfo->overlayHeaderList[i].overlay_type = NORMAL_OVERLAY;


            if(pOverlayInfo->overlayHeaderList[i].overlay_type == COVER_OVERLAY)
            {
                pOverlayInfo->overlayHeaderList[i].cover_yuv.cover_y = 0xa0;
                pOverlayInfo->overlayHeaderList[i].cover_yuv.cover_u = 0xa0;
                pOverlayInfo->overlayHeaderList[i].cover_yuv.cover_v = 0xa0;
            }

            pOverlayInfo->overlayHeaderList[i].overlay_blk_addr = pBitMapInfo->argb_addr;
            pOverlayInfo->overlayHeaderList[i].bitmap_size = pBitMapInfo->size;

            logv("blk_%d[%d], start_mb[%d,%d], end_mb[%d,%d],extra_alpha_flag:%d, extra_alpha:%d\n",
                                i,
                                time_id_list[i],
                                pOverlayInfo->overlayHeaderList[i].start_mb_x,
                                pOverlayInfo->overlayHeaderList[i].start_mb_y,
                                pOverlayInfo->overlayHeaderList[i].end_mb_x,
                                pOverlayInfo->overlayHeaderList[i].end_mb_y,
                                pOverlayInfo->overlayHeaderList[i].extra_alpha_flag,
                                pOverlayInfo->overlayHeaderList[i].extra_alpha);
            logv("overlay_type:%d, cover_yuv[%d,%d,%d], overlay_blk_addr:%p, bitmap_size:%d\n",
                                pOverlayInfo->overlayHeaderList[i].overlay_type,
                                pOverlayInfo->overlayHeaderList[i].cover_yuv.cover_y,
                                pOverlayInfo->overlayHeaderList[i].cover_yuv.cover_u,
                                pOverlayInfo->overlayHeaderList[i].cover_yuv.cover_v,
                                pOverlayInfo->overlayHeaderList[i].overlay_blk_addr,
                                pOverlayInfo->overlayHeaderList[i].bitmap_size);
            //if(i != 5)
            {
                start_mb_x += pBitMapInfo->width_aligh16 / 16;
                start_mb_y += pBitMapInfo->height_aligh16 / 16;
            }
        }

    return;
}

int initJpegFunc(jpeg_func_t *jpeg_func, encode_param_t *encode_param)
{
    memset(jpeg_func, 0, sizeof(jpeg_func_t));

    jpeg_func->quality = 95;
#if 0// ln change
    if(encode_param->encode_frame_num > 1)
        jpeg_func->jpeg_mode = 1;
    else
#endif
        jpeg_func->jpeg_mode = 0;

    if(0 == jpeg_func->jpeg_mode)
        init_jpeg_exif(&jpeg_func->exifinfo);
    else if(1 == jpeg_func->jpeg_mode)
        init_jpeg_rate_ctrl(jpeg_func);
    else
    {
        loge("encoder do not support the jpeg_mode:%d\n", jpeg_func->jpeg_mode);
        return -1;
    }

     //init VencOverlayConfig
    init_overlay_info(&jpeg_func->sOverlayInfo);

    return 0;
}


void init_mb_mode(VencMBModeCtrl *pMBMode, encode_param_t *encode_param)
{
    unsigned int mb_num;
    unsigned int j;

    mb_num = (ALIGN_XXB(16, encode_param->dst_width) >> 4)
                * (ALIGN_XXB(16, encode_param->dst_height) >> 4);
    pMBMode->p_info = (VencMBModeCtrlInfo *)malloc(sizeof(VencMBModeCtrlInfo) * mb_num);
    pMBMode->mode_ctrl_en = 1;

    for (j = 0; j < mb_num / 2; j++)
    {
        pMBMode->p_info[j].mb_en = 1;
        pMBMode->p_info[j].mb_skip_flag = 0;
        pMBMode->p_info[j].mb_qp = 22;
    }
    for (; j < mb_num; j++)
    {
        pMBMode->p_info[j].mb_en = 1;
        pMBMode->p_info[j].mb_skip_flag = 0;
        pMBMode->p_info[j].mb_qp = 32;
    }
}

void init_mb_info(VencMBInfo *MBInfo, encode_param_t *encode_param)
{
    if(encode_param->encode_format == VENC_CODEC_H265)
    {
        MBInfo->num_mb = (ALIGN_XXB(32, encode_param->dst_width) *
                            ALIGN_XXB(32, encode_param->dst_height)) >> 10;
    }
    else
    {
        MBInfo->num_mb = (ALIGN_XXB(16, encode_param->dst_width) *
                            ALIGN_XXB(16, encode_param->dst_height)) >> 8;
    }

    MBInfo->p_para = (VencMBInfoPara *)malloc(sizeof(VencMBInfoPara) * MBInfo->num_mb);

    if(MBInfo->p_para == NULL)
    {
        loge("malloc MBInfo->p_para error\n");
        return;
    }
}

void init_fix_qp(VencH264FixQP *fixQP)
{
    fixQP->bEnable = 1;
    fixQP->nIQp = 35;
    fixQP->nPQp = 35;
}

void init_super_frame_cfg(VencSuperFrameConfig *sSuperFrameCfg)
{
    sSuperFrameCfg->eSuperFrameMode = VENC_SUPERFRAME_NONE;
    sSuperFrameCfg->nMaxIFrameBits = 30000*8;
    sSuperFrameCfg->nMaxPFrameBits = 15000*8;
}

void init_svc_skip(VencH264SVCSkip *SVCSkip)
{
    SVCSkip->nTemporalSVC = T_LAYER_4;
    switch(SVCSkip->nTemporalSVC)
    {
        case T_LAYER_4:
            SVCSkip->nSkipFrame = SKIP_8;
            break;
        case T_LAYER_3:
            SVCSkip->nSkipFrame = SKIP_4;
            break;
        case T_LAYER_2:
            SVCSkip->nSkipFrame = SKIP_2;
            break;
        default:
            SVCSkip->nSkipFrame = NO_SKIP;
            break;
    }
}

void init_aspect_ratio(VencH264AspectRatio *sAspectRatio)
{
    sAspectRatio->aspect_ratio_idc = 255;
    sAspectRatio->sar_width = 4;
    sAspectRatio->sar_height = 3;
}

void init_video_signal(VencH264VideoSignal *sVideoSignal)
{
    sVideoSignal->video_format = (VENC_VIDEO_FORMAT)5;
    sVideoSignal->src_colour_primaries = (VENC_COLOR_SPACE)0;
    sVideoSignal->dst_colour_primaries = (VENC_COLOR_SPACE)1;
}

void init_intra_refresh(VencCyclicIntraRefresh *sIntraRefresh)
{
    sIntraRefresh->bEnable = 1;
    sIntraRefresh->nBlockNumber = 10;
}

void init_roi(VencROIConfig *sRoiConfig)
{
    sRoiConfig[0].bEnable = 1;
    sRoiConfig[0].index = 0;
    sRoiConfig[0].nQPoffset = 10;
    sRoiConfig[0].sRect.nLeft = 0;
    sRoiConfig[0].sRect.nTop = 0;
    sRoiConfig[0].sRect.nWidth = 1280;
    sRoiConfig[0].sRect.nHeight = 320;

    sRoiConfig[1].bEnable = 1;
    sRoiConfig[1].index = 1;
    sRoiConfig[1].nQPoffset = 10;
    sRoiConfig[1].sRect.nLeft = 320;
    sRoiConfig[1].sRect.nTop = 180;
    sRoiConfig[1].sRect.nWidth = 320;
    sRoiConfig[1].sRect.nHeight = 180;

    sRoiConfig[2].bEnable = 1;
    sRoiConfig[2].index = 2;
    sRoiConfig[2].nQPoffset = 10;
    sRoiConfig[2].sRect.nLeft = 320;
    sRoiConfig[2].sRect.nTop = 180;
    sRoiConfig[2].sRect.nWidth = 320;
    sRoiConfig[2].sRect.nHeight = 180;

    sRoiConfig[3].bEnable = 1;
    sRoiConfig[3].index = 3;
    sRoiConfig[3].nQPoffset = 10;
    sRoiConfig[3].sRect.nLeft = 320;
    sRoiConfig[3].sRect.nTop = 180;
    sRoiConfig[3].sRect.nWidth = 320;
    sRoiConfig[3].sRect.nHeight = 180;
}

void init_alter_frame_rate_info(VencAlterFrameRateInfo *pAlterFrameRateInfo)
{
    memset(pAlterFrameRateInfo, 0 , sizeof(VencAlterFrameRateInfo));
    pAlterFrameRateInfo->bEnable = 1;
    pAlterFrameRateInfo->bUseUserSetRoiInfo = 1;
    pAlterFrameRateInfo->sRoiBgFrameRate.nSrcFrameRate = 25;
    pAlterFrameRateInfo->sRoiBgFrameRate.nDstFrameRate = 5;

    pAlterFrameRateInfo->roi_param[0].bEnable = 1;
    pAlterFrameRateInfo->roi_param[0].index = 0;
    pAlterFrameRateInfo->roi_param[0].nQPoffset = 10;
    pAlterFrameRateInfo->roi_param[0].roi_abs_flag = 1;
    pAlterFrameRateInfo->roi_param[0].sRect.nLeft = 0;
    pAlterFrameRateInfo->roi_param[0].sRect.nTop = 0;
    pAlterFrameRateInfo->roi_param[0].sRect.nWidth = 320;
    pAlterFrameRateInfo->roi_param[0].sRect.nHeight = 320;

    pAlterFrameRateInfo->roi_param[1].bEnable = 1;
    pAlterFrameRateInfo->roi_param[1].index = 0;
    pAlterFrameRateInfo->roi_param[1].nQPoffset = 10;
    pAlterFrameRateInfo->roi_param[1].roi_abs_flag = 1;
    pAlterFrameRateInfo->roi_param[1].sRect.nLeft = 320;
    pAlterFrameRateInfo->roi_param[1].sRect.nTop = 320;
    pAlterFrameRateInfo->roi_param[1].sRect.nWidth = 320;
    pAlterFrameRateInfo->roi_param[1].sRect.nHeight = 320;
}

void init_enc_proc_info(VeProcSet *ve_proc_set)
{
    ve_proc_set->bProcEnable = 1;
    ve_proc_set->nProcFreq = 3;
}

int initH264Func(h264_func_t *h264_func, encode_param_t *encode_param)
{
    memset(h264_func, 0, sizeof(h264_func_t));

    //init h264Param
    h264_func->h264Param.bEntropyCodingCABAC = 1;
    h264_func->h264Param.nBitrate = encode_param->bit_rate;
    h264_func->h264Param.nFramerate = encode_param->frame_rate;
    h264_func->h264Param.nCodingMode = VENC_FRAME_CODING;
    h264_func->h264Param.nMaxKeyInterval = encode_param->maxKeyFrame;
    h264_func->h264Param.sProfileLevel.nProfile = VENC_H264ProfileHigh;
    h264_func->h264Param.sProfileLevel.nLevel = VENC_H264Level51;
    h264_func->h264Param.sQPRange.nMinqp = 10;
    h264_func->h264Param.sQPRange.nMaxqp = 50;
    h264_func->h264Param.bLongRefEnable = 1;
    h264_func->h264Param.nLongRefPoc = 0;

#if 1
    h264_func->sH264Smart.img_bin_en = 1;
    h264_func->sH264Smart.img_bin_th = 27;
    h264_func->sH264Smart.shift_bits = 2;
    h264_func->sH264Smart.smart_fun_en = 1;
#endif

    //init VencMBModeCtrl
    init_mb_mode(&h264_func->h264MBMode, encode_param);

    //init VencMBInfo
    init_mb_info(&h264_func->MBInfo, encode_param);

    //init VencH264FixQP
    init_fix_qp(&h264_func->fixQP);

    //init VencSuperFrameConfig
    init_super_frame_cfg(&h264_func->sSuperFrameCfg);

    //init VencH264SVCSkip
    init_svc_skip(&h264_func->SVCSkip);

    //init VencH264AspectRatio
    init_aspect_ratio(&h264_func->sAspectRatio);

    //init VencH264AspectRatio
    init_video_signal(&h264_func->sVideoSignal);

    //init CyclicIntraRefresh
    init_intra_refresh(&h264_func->sIntraRefresh);

    //init VencROIConfig
    init_roi(h264_func->sRoiConfig);

    //init proc info
    init_enc_proc_info(&h264_func->sVeProcInfo);

    //init VencOverlayConfig
    init_overlay_info(&h264_func->sOverlayInfo);

    return 0;
}

void init_h265_gop(VencH265GopStruct *h265Gop)
{
    h265Gop->gop_size = 8;
    h265Gop->intra_period = 16;

    h265Gop->use_lt_ref_flag = 1;
    if(h265Gop->use_lt_ref_flag)
    {
        h265Gop->max_num_ref_pics = 2;
        h265Gop->num_ref_idx_l0_default_active = 2;
        h265Gop->num_ref_idx_l1_default_active = 2;
        h265Gop->use_sps_rps_flag = 0;
    }
    else
    {
        h265Gop->max_num_ref_pics = 1;
        h265Gop->num_ref_idx_l0_default_active = 1;
        h265Gop->num_ref_idx_l1_default_active = 1;
        h265Gop->use_sps_rps_flag = 1;
    }
    //1:user config the reference info; 0:encoder config the reference info
    h265Gop->custom_rps_flag = 0;
    if(1 == h265Gop->custom_rps_flag)
    {
        int pos = 0;
        for (pos = 0; pos < h265Gop->gop_size; pos++)
        {
            h265Gop->ref_str[pos].slice_type = H265_P_SLICE;
            h265Gop->ref_str[pos].poc = pos + 1;
            h265Gop->ref_str[pos].qp_offset = 0;
            h265Gop->ref_str[pos].tc_offset_div2 = 0;
            h265Gop->ref_str[pos].beta_offset_div2 = 0;
            h265Gop->ref_str[pos].num_ref_pics = 2;
            h265Gop->ref_str[pos].reference_pics[0] = -1;
            h265Gop->ref_str[pos].reference_pics[1] = -(pos + 1);
            h265Gop->ref_str[pos].discard_pics[0] = -2;
            h265Gop->ref_str[pos].lt_ref_flag = 1;
            h265Gop->ref_str[pos].lt_ref_poc = 0;
            h265Gop->ref_str[pos].predict = 1;
            h265Gop->ref_str[pos].delta_rps_idx = 1;
            h265Gop->ref_str[pos].delta_rps = -1;
            h265Gop->ref_str[pos].num_ref_idcs = 3;
            h265Gop->ref_str[pos].reference_idcs[0][0] = REF_IDC_DISCARD;
            h265Gop->ref_str[pos].reference_idcs[0][1] = REF_IDC_CURRENT_USE;
            h265Gop->ref_str[pos].reference_idcs[0][2] = REF_IDC_LONG_TERM;
        }

        h265Gop->ref_str[0].num_ref_pics = 1;
        h265Gop->ref_str[0].reference_pics[1] = -1;
        h265Gop->ref_str[0].discard_pics[1] = -(h265Gop->gop_size + 1);
        h265Gop->ref_str[0].lt_ref_flag = 0;
        h265Gop->ref_str[0].predict = 0;
        h265Gop->ref_str[0].num_ref_idcs = 3;
        h265Gop->ref_str[0].reference_idcs[0][1] = REF_IDC_CURRENT_USE;
        h265Gop->ref_str[0].reference_idcs[0][2] = REF_IDC_DISCARD;

        h265Gop->ref_str[1].num_ref_idcs = 2;
        h265Gop->ref_str[1].reference_idcs[0][0] = REF_IDC_CURRENT_USE;
        h265Gop->ref_str[1].reference_idcs[0][1] = REF_IDC_LONG_TERM;

        h265Gop->ref_str[h265Gop->gop_size].slice_type = H265_IDR_SLICE;
        h265Gop->ref_str[h265Gop->gop_size].poc = 0;
        h265Gop->ref_str[h265Gop->gop_size].predict = 0;

        if (h265Gop->use_lt_ref_flag)
        {
            h265Gop->ref_str[h265Gop->gop_size + 1].num_ref_pics = 1;
            h265Gop->ref_str[h265Gop->gop_size + 1].reference_pics[0] = -1;
            h265Gop->ref_str[h265Gop->gop_size + 1].delta_rps_idx = 1;
            h265Gop->ref_str[h265Gop->gop_size + 1].delta_rps = -1;
            h265Gop->ref_str[h265Gop->gop_size + 1].num_ref_idcs = 1;
            h265Gop->ref_str[h265Gop->gop_size + 1].reference_idcs[0][0] = REF_IDC_CURRENT_USE;
        }
    }
}

int initH265Func(h265_func_t *h265_func, encode_param_t *encode_param)
{
    memset(h265_func, 0, sizeof(h264_func_t));

    //init h265Param
    h265_func->h265Param.nBitrate = encode_param->bit_rate;
    h265_func->h265Param.nFramerate = encode_param->frame_rate;
    h265_func->h265Param.sProfileLevel.nProfile = VENC_H265ProfileMain;
    h265_func->h265Param.sProfileLevel.nLevel = VENC_H265Level41;
    h265_func->h265Param.sQPRange.nMaxqp = 52;
    h265_func->h265Param.sQPRange.nMinqp = 10;
    h265_func->h265Param.nQPInit = 30;
    h265_func->h265Param.idr_period = 30;
    h265_func->h265Param.nGopSize = h265_func->h265Param.idr_period;
    h265_func->h265Param.nIntraPeriod = h265_func->h265Param.idr_period;
    h265_func->h265Param.bLongTermRef = 1;

#if 1
    h265_func->h265Hvs.hvs_en = 1;
    h265_func->h265Hvs.th_dir = 24;
    h265_func->h265Hvs.th_coef_shift = 4;

    h265_func->h265Trc.inter_tend = 63;
    h265_func->h265Trc.skip_tend = 3;
    h265_func->h265Trc.merge_tend = 0;

    h265_func->h265Smart.img_bin_en = 1;
    h265_func->h265Smart.img_bin_th = 27;
    h265_func->h265Smart.shift_bits = 2;
    h265_func->h265Smart.smart_fun_en = 1;
#endif

    h265_func->h265_rc_frame_total = 20*h265_func->h265Param.nGopSize;

    //init H265Gop
    init_h265_gop(&h265_func->h265Gop);

    //init VencMBModeCtrl
    init_mb_mode(&h265_func->h265MBMode, encode_param);

    //init VencMBInfo
    init_mb_info(&h265_func->MBInfo, encode_param);

    //init VencH264FixQP
    init_fix_qp(&h265_func->fixQP);

    //init VencSuperFrameConfig
    init_super_frame_cfg(&h265_func->sSuperFrameCfg);

    //init VencH264SVCSkip
    init_svc_skip(&h265_func->SVCSkip);

    //init VencH264AspectRatio
    init_aspect_ratio(&h265_func->sAspectRatio);

    //init VencH264AspectRatio
    init_video_signal(&h265_func->sVideoSignal);

    //init CyclicIntraRefresh
    init_intra_refresh(&h265_func->sIntraRefresh);

    //init VencROIConfig
    init_roi(h265_func->sRoiConfig);

    //init alter frameRate info
    init_alter_frame_rate_info(&h265_func->sAlterFrameRateInfo);

    //init proc info
    init_enc_proc_info(&h265_func->sVeProcInfo);

    //init VencOverlayConfig
    init_overlay_info(&h265_func->sOverlayInfo);

    return 0;
}

int set_params_into_encoder(VideoEncoder *VideoEnc, encode_param_t *encode_param)
{
	int result = 0;

    if(encode_param->encode_format == VENC_CODEC_JPEG)
    {
        result = initJpegFunc(&jpeg_func, encode_param);

        if(result)
        {
            loge("initJpegFunc error, return \n");
            return -1;
        }

        if(1 == jpeg_func.jpeg_mode)
        {
            VideoEncSetParameter(VideoEnc, VENC_IndexParamJpegEncMode, &jpeg_func.jpeg_mode);
            VideoEncSetParameter(VideoEnc, VENC_IndexParamBitrate, &jpeg_func.jpeg_biteRate);
            VideoEncSetParameter(VideoEnc, VENC_IndexParamFramerate, &jpeg_func.jpeg_frameRate);
            VideoEncSetParameter(VideoEnc, VENC_IndexParamSetBitRateRange, &jpeg_func.bitRateRange);
        }
        else
        {
            unsigned int vbv_size = 4*1024*1024;
            VideoEncSetParameter(VideoEnc, VENC_IndexParamSetVbvSize, &vbv_size);
            VideoEncSetParameter(VideoEnc, VENC_IndexParamJpegQuality, &jpeg_func.quality);
            VideoEncSetParameter(VideoEnc, VENC_IndexParamJpegExifInfo, &jpeg_func.exifinfo);
            VideoEncSetParameter(VideoEnc, VENC_IndexParamSetOverlay, &jpeg_func.sOverlayInfo);
        }
    }
    else if(encode_param->encode_format == VENC_CODEC_H264)
	{
#ifdef H264_ORIGINAL
		//* h264 param
	    VencH264Param h264Param;
	    int value = 0;

	    h264Param.bEntropyCodingCABAC = 1;  // 0:CAVLC 1:CABAC
	    h264Param.nBitrate = encode_param.bit_rate * 1024 * 1024;  //3MB
	    h264Param.nFramerate = encode_param.frame_rate;  //30fps
	    h264Param.nCodingMode = VENC_FRAME_CODING;
	    //h264Param.nCodingMode = VENC_FIELD_CODING;
	    h264Param.nMaxKeyInterval = 30;
	    h264Param.sProfileLevel.nProfile = VENC_H264ProfileMain;
	    h264Param.sProfileLevel.nLevel = VENC_H264Level31;
	    h264Param.sQPRange.nMinqp = 10;
	    h264Param.sQPRange.nMaxqp = 40;

        VideoEncSetParameter(VideoEnc, VENC_IndexParamH264Param, &h264Param);
        value = 0;
        VideoEncSetParameter(VideoEnc, VENC_IndexParamIfilter, &value);
        value = 0; //degree
        VideoEncSetParameter(VideoEnc, VENC_IndexParamRotation, &value);
//      value = 0;
//      VideoEncSetParameter(VideoEnc, VENC_IndexParamSetPSkip, &value);
#else
        result = initH264Func(&h264_func, encode_param);
        if(result)
        {
            loge("initH264Func error, return \n");
            return -1;
        }
        unsigned int vbv_size = 12*1024*1024;
        VideoEncSetParameter(VideoEnc, VENC_IndexParamH264Param, &h264_func.h264Param);
        VideoEncSetParameter(VideoEnc, VENC_IndexParamSetVbvSize, &vbv_size);
        //VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264FixQP, &h264_func.fixQP);

        VideoEncSetParameter(VideoEnc, VENC_IndexParamSetOverlay, &h264_func.sOverlayInfo);

#ifdef GET_MB_INFO
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamMBInfoOutput, &h264_func.MBInfo);
#endif
#if 0
        unsigned char value = 1;
        //set the specify func
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264SVCSkip, &h264_func.SVCSkip);
        value = 0;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamIfilter, &value);
        value = 0; //degree
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamRotation, &value);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264FixQP, &h264_func.fixQP);
        VideoEncSetParameter(pVideoEnc,
            VENC_IndexParamH264CyclicIntraRefresh, &h264_func.sIntraRefresh);
        value = 720/4;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamSliceHeight, &value);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &h264_func.sRoiConfig[0]);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &h264_func.sRoiConfig[1]);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &h264_func.sRoiConfig[2]);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &h264_func.sRoiConfig[3]);
        value = 0;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamSetPSkip, &value);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264AspectRatio, &h264_func.sAspectRatio);
        value = 0;
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamFastEnc, &value);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264VideoSignal, &h264_func.sVideoSignal);
        VideoEncSetParameter(pVideoEnc, VENC_IndexParamSuperFrameConfig, &h264_func.sSuperFrameCfg);
#endif

#endif
	}
    else if(encode_param->encode_format == VENC_CODEC_H265)
    {
	    result = initH265Func(&h265_func, encode_param);

		if(result)
		{
			loge("initH265Func error, return \n");
			return -1;
		}

		unsigned int vbv_size = 12*1024*1024;
		VideoEncSetParameter(VideoEnc, VENC_IndexParamSetVbvSize, &vbv_size);
		VideoEncSetParameter(VideoEnc, VENC_IndexParamH265Param, &h265_func.h265Param);

		unsigned int value = 1;
		//VideoEncSetParameter(pVideoEnc,
		//VENC_IndexParamAlterFrame, &h265_func.sAlterFrameRateInfo);
		VideoEncSetParameter(VideoEnc, VENC_IndexParamChannelNum, &value);
		//VideoEncSetParameter(pVideoEnc, VENC_IndexParamProcSet, &h265_func.sVeProcInfo);
		//VideoEncSetParameter(pVideoEnc, VENC_IndexParamSetOverlay, &h265_func.sOverlayInfo);
		//VideoEncSetParameter(pVideoEnc, VENC_IndexParamVirtualIFrame, &encode_param->frame_rate);
		//value = 0;
		//VideoEncSetParameter(pVideoEnc, VENC_IndexParamPFrameIntraEn, &value);
		//value = 1;
		//VideoEncSetParameter(pVideoEnc, VENC_IndexParamEncodeTimeEn, &value);
		//VideoEncSetParameter(pVideoEnc,
		//VENC_IndexParamH265ToalFramesNum,  &h265_func.h265_rc_frame_total);
		//VideoEncSetParameter(pVideoEnc, VENC_IndexParamH265Gop, &h265_func.h265Gop);

		//VideoEncSetParameter(pVideoEnc, VENC_IndexParamROIConfig, &h265_func.sRoiConfig[0]);
		//VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264FixQP, &h265_func.fixQP);
		//VideoEncSetParameter(pVideoEnc, VENC_IndexParamH265HVS, &h265_func.h265Hvs);
		//VideoEncSetParameter(pVideoEnc, VENC_IndexParamH265TendRatioCoef, &h265_func.h265Trc);
#ifdef GET_MB_INFO
		VideoEncSetParameter(pVideoEnc, VENC_IndexParamMBInfoOutput, &h265_func.MBInfo);
#endif
    }

	return 0;
}

