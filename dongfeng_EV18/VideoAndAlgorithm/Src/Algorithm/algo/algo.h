//
// Created by admin on 2018/3/19.
//

#ifndef ADAS_DWSAPP_ALGO_H_H
#define ADAS_DWSAPP_ALGO_H_H

#include <iostream>
#include <string>
#include <vector>
#include "opencv2/opencv.hpp"
#include "mtcnn/mtcnn.h"
#include "kcf/kcftracker.hpp"
#include "classifier.h"
#include "objectDetect.hpp"
#include "detect.h"



using namespace cv;
using namespace std;

#define NV21 0x11
#define RGB  0x0
#define Y8 0x20203859


//报警类型
enum ALARM_TYPE {
    NRL_FACE = 0x00, //检测到人脸
    NO_FACE = 0x01, //未检测到脸
    EYES_ALARM = 0x10,//闭眼报警
    YAWN_ALARM = 0x11,//打哈欠报警
    CALL_ALARM = 0x12,//打电话报警
    SMOKE_ALARM = 0x13,//抽烟报警
    ATTEN_ALARM = 0x14, //分神
    ABNORMAL_ALARM = 0x15,//驾驶员异常
    CIGARETTE_ALARM = 0x16,  //手+烟物品报警
    BLINK_ALARM = 0x17,      //眨眼报警
    BLOCKGLASS_ALARM = 0x18,  //红外阻断墨镜报警
    PHONE_ALARM = 0x19,      //手+手机物品报警
    HAND_OFF_ALARM = 0x1A, //脱离方向盘
	COVER_ALARM = 0x1B
};

//报警类型，用于报警队列
enum {
    EYE_ACTION_TYPE = 0x01, //闭眼
    YAWN_ACTION_TYPE = 0x02, // 打哈欠
    ATTEN_ACTION_TYPE = 0x03, // 注意力分散
    SMOKE_CLASSIFY_TYPE = 0x04, // 脱离方向盘
    CALL_CLASSIFY_TYPE = 0x05, // 打手机
    FACE_ACTION_TYPE = 0x06,
    PHONE_TYPE = 0x07,         //手+手机
    CIGARETTE_TYPE = 0x08,     //手+烟
    BLINK_TYPE = 0x09,         //眨眼
    BLOCKGLASS_TYPE = 0x10,    //红外阻断
	COVER_TYPE = 0x11
};

//检测类别
enum ObjectType {
    EYE = 1,
    CLOSED = 2,
    HAND = 3,
    PHONE = 4,
    CIGARETTE = 5,
    MOUTH = 6,
    BELT = 7
};

typedef struct {
    //单帧图像分类阈值
    float callClassifyScore;
    float smokeClassifyScore;
    float yawnClassifyScore;
    float attenRightScore;
    float attenLeftScore;
    float attenUpScore;
    float attenDownScore;
    float eyeClassifyScore;
    float normalScore;

    //检测阈值
    float callConfidence;
    float smokeConfidence;
    float yawnConfidence;
    float eyeConfidence;
    int smokeDot;

    //报警队列统计时长（单位：秒）
    int smokeClassifyLevel;
    int callClassifyLevel;
    int attenLevel;
    int yawnLevel;
    int eyeLevel;
    int nopLevel;//没有脸

    //报警队列报警阈值
    float smokeClassifyDeqConf;
    float callClassifyDeqConf;
    float attenDeqConf;
    float yawnDeqConf;
    float eyeDeqConf;
} stAlgoParam;

//int demoMode = 0;//演示模式(1),正常模式(0)

class Algo {
public:
    Algo();
    void setAlgoParam(stAlgoParam algoParam);
    int init(int fomat, int width, int height);
    int detectFrame(cv::Mat &frame, float speed, int& demoMode);
    int release();

    int FaceCheckNcnn(Mat frame, float confidence);
    int SmokeCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect);
    int CallCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect);
    int MouthCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect);
    int LRCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect);
    int CloseEyeCheckNcnn(Mat frame, float confidence, cv::Rect ROIRect);

    void AddAction(int flag, int num);
    int CheckActionResult(int flag);
    void ClearActionResult(int flag);
    void ClearAllResult();

    stAlgoParam algoParam;   //param

//private:
    int frameIdx = 0;
    int ERR_CNT = 25;
    int errCnt = 0;

    int tmpWidth = 0;

    int frameWidth;
    int frameHeight;

    int pixelFormat;
    Mat smallFrame;     //小图，算法处理
    int smallTimes = 4; //大图缩小倍数
    int rstFlag = 0;

    Mat detectSquareImg;//检测图片
    Rect detectSquareRect;//检测图片框

    MTCNN faceDetector;
    int minSize = 40; //MTCNN最小变化尺度
    float factor = 0.709f;
    float threshold[3] = {0.7f, 0.6f, 0.6f};
    Rect faceRect; //检测人脸目标

    KCFTracker tracker;
    int trackFlag = 0;
    Rect kcfResult; //跟踪结果

    int tmpHeight = 0;

    typedef struct {
        int dataSize; //队列大小
        int alarmCnt; //报警阈值
        vector<int> action; // 动作标记
    } stActionCnt;

    //报警队列
    stActionCnt *pAction;
    stActionCnt stEyeAction;
    stActionCnt stYawnAction;
    stActionCnt stAttenAction;
    stActionCnt stSmokeClassify;
    stActionCnt stCallClassify;
    stActionCnt stFaceAction;
    stActionCnt stPhone; //手+手机
    stActionCnt stCigarette;//手+烟
    stActionCnt stBlink; //眨眼
    stActionCnt stBlockGlass;//红外阻断
    stActionCnt stCover;

    Mat classifyFrame;
    float predictScore;
    string predictText;
    Classifier classifierEye;//睁闭眼分类器
    Classifier classifierCallSmoke; //抽烟打电话分类器
    Classifier classifierMouth; //嘴巴分类器
    Classifier classifierLR;
    Classifier classifierLRUD;

    //检测算法结果
    vector<Rect> objects;
    vector<int> classes;
    vector<float> confidences;
    //ObjectDetector objectDetect;    //caffe检测方法

//ncnn
    DetectNcnn detectNcnn;
};

#endif //ADAS_DWSAPP_ALGO_H_H
