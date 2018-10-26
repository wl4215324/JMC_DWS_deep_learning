//一体机V1.3：
//		1.修改日期2017/09/05: 吸烟，还原成DSPv4.8版
//一体机V1.4：
//		1.修改日期2017/09/17: 增加了算法函数输入参数——framesPerSecond，可自行设定算法每秒处理帧数
//		2.修改日期2017/09/17: 增加了算法函数输出参数——smokingGestureFlag，brightPointFlag
//一体机V1.5：
//		1.修改日期2017/09/17: 增加了算法函数输出参数——telephoneCallFlag
//一体机V1.6：
//		1.修改日期2017/09/17: 放松了打手机动作的检测要求
//一体机V2.0：
//		1.修改日期2017/11/18: 江铃定制版，增加了cal_perclos_eye4函数，闭眼分为一级报警和二级报警，二级报警时长为一级报警的两倍
//		2.修改日期2017/11/18: 江铃定制版，ARITH_OUTPUT结构体增加了数据成员eyeCloseEventTime-闭眼持续时间，单位：帧数
//		3.修改日期2017/11/18: 江铃定制版，在眼睛睁开后会输出闭眼时长eyeCloseEventTime
//		4.修改日期2017/11/18: 江铃定制版，闭眼、打哈欠、吸烟、打手机均连续输出报警信号
//		5.修改日期2017/12/04: 江铃定制版，更新了人脸分类器
//一体机V2.1：
//		1.修改日期2018/02/05: 江铃定制版，更新了睁眼／打哈欠检测
//		2.修改日期2018/02/05: 江铃定制版，调整了打哈欠检测的限制
//		3.修改日期2018/02/05: 江铃定制版，调整了闭眼检测的限制
//一体机V2.2：
//		1.修改日期2018/02/07: 江铃定制版，放宽了打哈欠检测
//		2.修改日期2018/02/07: 江铃定制版，离岗时的算法处理帧率是有人脸目标情况下的1.3倍(原2倍)
//一体机V2.3：
//		1.修改日期2018/03/03: 江铃定制版，修改了一二级闭眼报警和闭眼时长输出的逻辑
//一体机V2.4：
//		1.修改日期2018/04/16: 江铃定制版，算法输出参数增加了clearBuf：0——not clear buffer;1——clear buffer


//报警阈值接口开放

#ifndef _ALGOLIB_H_
#define _ALGOLIB_H_


#define Uint32 unsigned int
#define Uint16 unsigned short
#define Uint8 unsigned char

#define VER_ARITH    0x0203



/***************************
       Resizer
****************************/
#define INHEIGHT    (480)
#define INWIDTH     (720)
#define OUTHEIGHT   (240)
#define OUTWIDTH    (360)

#define mpy704(x) ((x)*720)
#define mpy352(x) ((x)*360)

#define TIMEWINDOWS 280
#define TIMEWINDOWLENGTH 280

//int FACE_DEQUE = 0;
//int CALL_SIZE = 0;
//int SMOKE_SIZE = 0;
//int MOUTH_SIZE = 0;
//int LEFT_RIGHT_SIZE = 0;
//int EYE_SIZE = 0;

typedef struct _Warning_Seconds
{
	float faceSec;
	float phoneSec;
	float smokeSec;
	float mouthSec;
	float distractSec;
	float eyeSec;
} WarningSeconds;

typedef struct _Object_Confidence
{
	float faceConf;
	float phoneConf;
	float smokeConf;
	float mouthConf;
	float distractConf;
	float closedEyeConf;
	float brightPointConf;
}Object_Confidence;


//static Uint8 framesPerSecond = 8; //14;

typedef struct _ARITH_INPUT
{
	WarningSeconds    objSec;
	Object_Confidence objConf;

	Uint8    arithModuleEn;			// 算法功能模块使能:设置为1
	Uint8    distractEnable;		// 左顾右看算法使能:0-失效；1-使能
	Uint8    ctnECEnable;			// 持续闭眼报警使能:0-失效；1-使能
	Uint8    eyeCloseTime;			// 闭眼检测时间:最小单位为0.5秒
	Uint8    yawnTime;				// 打哈欠检测时间:最小单位为1秒
	Uint8    distractTime;			// 左顾右盼检测时间:最小单位为1秒
	short    noFaceTime;			// 无人像检测时间:最小单位为1秒
	Uint8    telephoneCallTime;		// 打电话检测时间:最小单位为1秒
	Uint8    smokingTime;			// 吸烟检测时间:最小单位为1秒
	Uint8    runMode;				// 系统运行模式:设置为1
	Uint8    ctnNFEnable;			// 无人像连续报警使能:0-失效；1-使能
	Uint8 	 framesPerSecond;		// 算法每秒处理帧数
	Uint8    clearBuf;              // 0: not clear buffer, 1: clear buffer
}ARITH_INPUT;

//#define XIAOMING_ARITH_INPUT_DEFAULT  {1, 1, 1, 3, 2, 3, 10, 4, 1, 1, 1, 7}
#define XIAOMING_ARITH_INPUT_DEFAULT  {1, 1, 1, 2, 1, 3, 10, 4, 1, 1, 1, 7, 0}

typedef struct _ARITH_OUTPUT
{
	Uint8    drowsyLevel;		// 疲劳报警:0无报警，8一级闭眼，9二级闭眼，2打哈欠，3注意力分散，4打手机，5抽烟，6离岗，7危险驾驶，100提示音
	Uint8    faceFlag;			// 人脸标识:0未检测到人脸，1检测到人脸
	Uint8	 imageOutput[INHEIGHT*INWIDTH];   	//报警图片输出
	Uint8    smokingGestureFlag;	// 吸烟手势标识:0未检测到，1检测到吸烟手势
	Uint8    brightPointFlag;		// 烟头亮点标识:0未检测到，1检测到烟头亮点
	Uint8    telephoneCallFlag;		// 打电话手势标识:0未检测到，1检测到打电话手势
	Uint8    eyeCloseEventTime;		// 闭眼持续时间，单位：帧数
}ARITH_OUTPUT;



// //报警阈值设置
// //algorithm input & output variable
// ARITH_INPUT algorithmInput;

// //algorithm time deque
// algorithmInput.objSec.faceSec = 10;
// algorithmInput.objSec.phoneSec = 3;
// algorithmInput.objSec.smokeSec = 2;
// algorithmInput.objSec.mouthSec = 2;
// algorithmInput.objSec.distractSec = 3;
// algorithmInput.objSec.eyeSec = 2;

// //algorithm warning threshold
// algorithmInput.objConf.faceConf = 0.9;
// algorithmInput.objConf.phoneConf = 0.45;
// algorithmInput.objConf.smokeConf = 0.45;
// algorithmInput.objConf.mouthConf = 0.45;
// algorithmInput.objConf.distractConf = 0.45;
// algorithmInput.objConf.closedEyeConf = 0.45;

#define OBJSEC_INITIALIZER  {80,10,3,2,3,3}
#define OBJCONF_INITIALIZER  {0.95,0.85,0.85,0.45,0.45,0.65}


#define OBJSEC_INITIALIZER_FOR_DAY  {80,10,2,2,3,2}
#define OBJCONF_INITIALIZER_FOR_DAY  {0.98,0.85,0.85,0.45,0.65,0.65}


/**************************************
函数名：InitParams
函数说明：疲劳检测算法初始化函数，
		  系统上电后仅需初始化一次。
***************************************/
void InitParams(ARITH_INPUT* initInput);



/**************************************
函数名：ImagePorcessing
返回值：int型，函数处理异常信息:0-正常；1-异常
参数：pDisp-图像缓冲区指针(Y分量)；
	  ArithInput-输入参数,包括灵敏度、工作模式等；
      ArithOutput-输出参数,包括疲劳结果，人脸标识等。
***************************************/
int ImageProcessing(unsigned char * pDisp, ARITH_INPUT* ArithInput, ARITH_OUTPUT* ArithOuput);
//void uyvy_2_gray(unsigned char *uyvy, unsigned char *gray);
#endif
