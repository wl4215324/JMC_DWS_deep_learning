/*
 * algoFXP.h
 *
 *  Created on: Oct 14, 2019
 *      Author: imx
 */

#ifndef DSMAPP_ALGO_ALGOFXP_H_
#define DSMAPP_ALGO_ALGOFXP_H_



#include "opencv2/opencv.hpp"
#include "wheelDetect/wheelNcnn.h"

using namespace cv;
using namespace std;

//报警类型
enum ALARM_TYPE_FXP {
    WHEEL_ALARM = 0x21
};

//每一帧报警状态
enum{
    HAND_TYPE = 0x01
};

//每一帧检测结果判断
typedef struct {
    float handConf;
    int wheelLevel;     //睡觉时长（单位:秒）
    float wheelDeqConf; //睡觉报警阈值（浮点数：0.0-1.0）
}stFXPParam;

//检测目标
enum ObjectFXPType {
    FXPHAND = 1
};


//算法类
class FXPAlgo {
public:
	FXPAlgo();

    int init();

    int detectFXP(cv::Mat &frame, float speed,stFXPParam algoParam,int flag);

    stFXPParam algoParamFxp;

    typedef struct {
        int dataSize; //队列大小
        int alarmCnt; //报警阈值
        vector<int> action; // 动作标记
    } stFXPCnt;

    //报警队列
    stFXPCnt *pAction;
    stFXPCnt stWheel;    //双手脱离方向盘队列

    //检测算法结果
    vector<Rect> objects;
    vector<int> classes;
    vector<float> confidences;

    DetectNcnnDay detectNcnnFXP;

    void AddAction(int flag, int num);
    int CheckActionResult(int flag);
    void ClearAllResult();
};

#endif //ADAS_DWSAPP_ALGO_H_H
