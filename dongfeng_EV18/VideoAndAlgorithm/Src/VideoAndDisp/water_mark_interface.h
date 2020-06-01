/*
 * water_mark_interface.h
 *
 *  Created on: Apr 28, 2020
 *      Author: tony
 */

#ifndef WATER_MARK_INTERFACE_H_
#define WATER_MARK_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define ADD_WATERMARK

#define MAX_WATERMARK_NUM 5
#define MAX_WATERMARK_LENGTH 32

typedef struct {
    int mPositionX;
    int mPositionY;
    char content[MAX_WATERMARK_LENGTH];
}SingleWaterMark;

typedef struct {
    int mWaterMarkNum;
    SingleWaterMark mSingleWaterMark[MAX_WATERMARK_NUM];
}WaterMarkMultiple;

typedef struct {
    unsigned char *y;
    unsigned char *c;
    int posx;
    int posy;
    int width;
    int height;
    float resolution_rate;
    char *display;
} WaterMarkInData;


typedef struct {
	unsigned char mWaterMarkEnable;
	WaterMarkInData mWaterMarkIndata;
	void *mWaterMarkCtrlRec;
	void *mWaterMarkCtrlPrev;
	void *mWaterMarkMultiple;
	pthread_mutex_t mWaterMarkLock;
} WaterMarkUsrInterface;


typedef enum {
	SYSTIME_DATE_IDX = 0,
	GPS_POS_IDX,
	VEH_SPEED_IDX,
	WARN_TYPE_IDX,
	SOFT_VER_IDX,
} ContentListIdx;

extern int doWaterMark(WaterMarkInData *indata, void *ctrl);
extern int doWaterMarkMultiple(WaterMarkInData *indata, void *ctrl, void *multi, char *content, char *time_watermark);
extern void *initialwaterMark(int wm_height);
extern void *initWaterMarkMultiple();
extern int releaseWaterMark(void *ctrl);
extern int releaseWaterMarkMultiple(void *multi);



WaterMarkUsrInterface* waterMarkinit(unsigned int FrameHeight);
int initWaterMark(unsigned int FrameHeight);

void waterMarkDestroy(WaterMarkUsrInterface **usr_watermark);

int addWaterMark(unsigned char *addrVirY, int width, int height, char *content, \
		         WaterMarkUsrInterface *usr_watermark);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WATER_MARK_INTERFACE_H_ */
