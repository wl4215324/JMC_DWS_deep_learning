#ifndef DETECT_H_
#define DETECT_H_

#include <stdio.h>
#include <vector>
#include <opencv2/opencv.hpp>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc/imgproc.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include "net.h"

using namespace std;
using namespace cv;

class DetectNcnn {
public:
    DetectNcnn();
	~DetectNcnn()
	{}
	DetectNcnn(const string& model_file, const string& weights_file);

    void detectObjNcnn(const Mat& img, vector<Rect>& objects, vector<int>& classes, vector<float>& confidences);

private:
	ncnn::Net squeezenet;
};


#endif // !CLASSIFY_H_
