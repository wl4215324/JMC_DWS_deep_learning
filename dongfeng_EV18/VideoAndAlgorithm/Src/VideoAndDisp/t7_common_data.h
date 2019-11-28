#ifndef __T7_COMMON_DATA_H__
#define __T7_COMMON_DATA_H__
#include <stdint.h>
#define MAX_VIDEO_CHN 8
struct VideoFrame{
    int size;
    uint64_t pts;
    uint32_t phy_addr;
    int w;
    int h;
    char *vir_buf;
    int chn;
};

struct H264PacketType {
    char *buf;
    int size;
    uint64_t pts;
    int is_key;
    int chn;
};

struct G726PacketSt{
    uint8_t *data;
    int size;
};

struct AudioFrameSt{
    uint8_t *data;
    int size;
};

typedef int (*t7_enc_bit_cb_func)(H264PacketType *pkt, void *op);
typedef int (*t7_dec_frame_cb_func)(VideoFrame *frm, void *op);

typedef int (*t7_audio_packet_cb_func)(G726PacketSt *frm, void *op);
typedef int (*t7_audio_frame_cb_func)(AudioFrameSt *frm, void *op);


struct Rect{
        int x;
        int y;
        int w;
        int h;
};

#endif
