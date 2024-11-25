#include <opencv2/opencv.hpp>
#include <iostream>
// #include "myApi.h"
 
using namespace std;
using namespace cv;
 
Mat g_srcImg; //原图片
Mat g_beautyFgImg;  //美颜后的前景图
Mat g_vagueBeImg;  //模糊后背景图
Mat g_roiImg;  //前景和后景组合后图片
Mat g_rectImg;  //绘制矩形边框
Mat g_brightImg;  //调整亮度
 
 
// 调整模糊程度
int g_iVagueValue = 3;
int g_iVagueMaxValue = 10;
 
// 调整美颜程度
int g_iBeautyValue = 9;
int g_iBeautyMaxValue = 20;
 
// 绘制矩形，设置相框
int g_iRectValue = 5;
int g_iRectMaxValue = 30;
 
//调整亮度范围
int g_iBrightValue = 6;
int g_iBrightMaxValue = 20;
 
//HSV-H
int g_iHSV_H = 25;
int g_iHSV_H_max = 255;
int g_iHSV_H1 = 25;
int g_iHSV_H_max1 = 255;

int g_iHSV_S = 25;
int g_iHSV_S_max = 255;
int g_iHSV_S1 = 25;
int g_iHSV_S_max1 = 255;

int g_iHSV_V = 25;
int g_iHSV_V_max = 255;
int g_iHSV_V1 = 25;
int g_iHSV_V_max1 = 255;

int g_Canny_low = 0;
int g_Canny_upper = 40;
int g_Canny_kerner = 3;
int g_Canny_blur = 5;
int g_Canny_dilate = 7;
int g_Canny_max = 255;


//调整模糊程度
void TrackbarVague(int, void*);
 
//调整美颜程度
void TrackbarBeauty(int, void*);
 
 
//调整亮度程度
void TrackbarCanny(int, void*);
void TrackbarHSV_H(int, void*);
void TrackbarHSV_S(int, void*);
void TrackbarHSV_V(int, void*);
 
//将美颜前景图调到模糊背景图层上
void SetRoi(void);

bool find_HSV(Mat &srcimg,Scalar &hsvLower, Scalar &hsvUpper)
{
	if (!srcimg.data)
		return false;
	namedWindow("img", WINDOW_FREERATIO);
	// namedWindow(windowName, 0);
    setWindowProperty("img", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    resizeWindow("img", 612, 512);
	// Rect roi1 = selectROI("img", g_srcImg, 1);
	// cout<<roi1.x<<" "<<roi1.y<<" "<<roi1.width<<" "<<roi1.height<<endl;
	 
	imshow("img", srcimg);

   
	Rect roi = selectROI("img", srcimg, 1);
	Mat selectedRoiImg = srcimg(roi);
	Mat blurImg, hsvImg;
	blur(selectedRoiImg, blurImg, Size(5, 5));
	Scalar rgbMean, rgbStdDev;
	meanStdDev(blurImg, rgbMean, rgbStdDev);
	cout << "rgbMean: " << rgbMean << endl;
	cout << "rgbStdDev: " << rgbStdDev << endl;

	cvtColor(blurImg, hsvImg, COLOR_BGR2HSV);
	Scalar hsvMean, hsvStdDev;
	meanStdDev(hsvImg, hsvMean, hsvStdDev);
	cout << "hsvMean: " << hsvMean << endl;
	cout << "hsvStdDev: " << hsvStdDev << endl;

	if (hsvStdDev[0] > 20)
		hsvMean[0] = 170;

	Mat srcBlurImg, srcHsvImg;
	blur(srcimg, srcBlurImg, Size(5, 5));
	cvtColor(srcBlurImg, srcHsvImg, COLOR_BGR2HSV);

	hsvLower = Scalar(hsvMean[0] - 20, hsvMean[1] - 20, hsvMean[2] - 80);
	hsvUpper = Scalar(hsvMean[0] + 20, hsvMean[1] + 80, hsvMean[2] + 80);
	Mat binaryImg;
	inRange(srcHsvImg, hsvLower, hsvUpper, binaryImg);
	if (hsvMean[0]  > 180)
	{
		Mat appendImg;
		inRange(srcHsvImg, Scalar(0, hsvLower[1], hsvLower[2]), Scalar(hsvUpper[0] - 180, hsvUpper[1], hsvUpper[2]), appendImg);
		bitwise_or(binaryImg, appendImg, binaryImg);
	}

	cout << "lower: " << hsvLower << " upper: " << hsvUpper << endl;

	Mat colorMask;
	vector<Mat> mergeImgs({binaryImg*0.2, binaryImg, binaryImg*0.2});
	merge(mergeImgs, colorMask);
    Mat mask_img;
	addWeighted(g_srcImg, 1, colorMask, 0.3, 0, mask_img);
	imshow("img", mask_img);
	waitKey(0);
	destroyAllWindows();
}
 
int main(int argc, char const *argv[])
{
	// g_srcImg = imread(argv[1]);

	g_srcImg = imread(argv[1]);
	Scalar hsvLower,hsvUpper;
	find_HSV(g_srcImg,hsvLower,hsvUpper);

	if (!g_srcImg.data)
		return -1;
	namedWindow("img", WINDOW_FREERATIO);
	// namedWindow(windowName, 0);
    setWindowProperty("img", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    resizeWindow("img", 612, 512);
	imshow("img", g_srcImg);
 
	/*
	功能：创建滚动条并将其附加到指定的窗口
	参数：trackbarname:创建的滚动条名称
		  winname：滚动条父窗口的名称
		  value：滚动条所在位置的值
		  count：滚动条的最大值，最小值总是0
		  onChange：指向滚动条改变位置时要调用的函数的指针（回调函数），函数的原型应该是void Foo（int， void*）;
					其中第一个参数是滚动条所在的位置，第二个参数是用户数据（见下一个参数）；如果回调函数传空，则不调用
					回调，但只更新滚动条值
		  userdata:按原样传递给回调的用户数据，它可以用来处理滚动条不使用全局变量事件
	返回：成功返回0
	*/
 

	g_iHSV_H = int(hsvLower[0]);
	g_iHSV_S = int(hsvLower[1]);
	g_iHSV_V = int(hsvLower[2]);
	g_iHSV_H1 = int(hsvUpper[0]);
	g_iHSV_S1 = int(hsvUpper[1]);
	g_iHSV_V1 = int(hsvUpper[2]);


	/* HSV 检测 */
	createTrackbar("HSV-H", "img", &g_iHSV_H, g_iHSV_H_max1, TrackbarHSV_H);
	TrackbarHSV_H(0,0);
	createTrackbar("HSV-S", "img", &g_iHSV_S, g_iHSV_H_max1, TrackbarHSV_H);
	TrackbarHSV_H(0,0);
	createTrackbar("HSV-V", "img", &g_iHSV_V, g_iHSV_H_max1, TrackbarHSV_H);
	TrackbarHSV_H(0,0);
	createTrackbar("HSV-H1", "img", &g_iHSV_H1, g_iHSV_H_max1, TrackbarHSV_H);
	TrackbarHSV_H(0,0);
	createTrackbar("HSV-S1", "img", &g_iHSV_S1, g_iHSV_H_max1, TrackbarHSV_H);
	TrackbarHSV_H(0,0);
	createTrackbar("HSV-V1", "img", &g_iHSV_V1, g_iHSV_H_max1, TrackbarHSV_H);
	TrackbarHSV_H(0,0);

	/* Canny 检测 */
	// createTrackbar("Canny-Low", "img", &g_Canny_low, g_Canny_max, TrackbarCanny);
	// TrackbarCanny(0,0);
	// createTrackbar("Canny-upper", "img", &g_Canny_upper, g_Canny_max, TrackbarCanny);
	// TrackbarCanny(0,0);
	// createTrackbar("Canny-blur", "img", &g_Canny_blur, 30, TrackbarCanny);
	// TrackbarCanny(0,0);
	// createTrackbar("Canny-dilate", "img", &g_Canny_dilate, 30, TrackbarCanny);
	// TrackbarCanny(0,0);
	waitKey(0);
	cout<<g_iHSV_H<<"=============="<<endl;
	if(g_iHSV_H<0)
	{
		g_iHSV_H = g_iHSV_H + 180;
		g_iHSV_H1 = g_iHSV_H1 + 180;
	}
	// int latColCount_ = viewIdx==0 ? lastColCount: getColCount -lastColCount;
	g_iHSV_S = g_iHSV_S < 0 ? 0 : g_iHSV_S;
	g_iHSV_V = g_iHSV_V < 0 ? 0 : g_iHSV_V;
	g_iHSV_S1 = g_iHSV_S1 > 255 ? 255 : g_iHSV_S1;
	g_iHSV_V1 = g_iHSV_V1 > 255 ? 255 : g_iHSV_V1;
	cout<<"low: "<<g_iHSV_H<<", "<<g_iHSV_S<<", "<<g_iHSV_V<<" ,upper: "<<g_iHSV_H1<<", "<<g_iHSV_S1<<", "<<g_iHSV_V1<<endl;;

	
	return 0;
}
 
void TrackbarHSV_H(int , void*)
{

	Mat blurImg, hsvImg,dstImage,colorMask;
	blur(g_srcImg, blurImg, Size(9, 9));
	cvtColor(blurImg, hsvImg, CV_BGR2HSV);
	inRange(hsvImg, Scalar(g_iHSV_H,g_iHSV_S,g_iHSV_V), Scalar(g_iHSV_H1,g_iHSV_S1,g_iHSV_V1), dstImage);
	if (g_iHSV_H1 > 180)
	{
		Mat appendImg;
		inRange(hsvImg, Scalar(0, g_iHSV_S, g_iHSV_V), Scalar(g_iHSV_H1- 180, g_iHSV_S1, g_iHSV_S1), appendImg);
		bitwise_or(dstImage, appendImg, dstImage);
	}
	Mat kernel = getStructuringElement(MORPH_RECT, Size(7, 7));//保证是奇数
	morphologyEx(dstImage, dstImage, MORPH_DILATE, kernel);
		// dstImage = (dstImage - openImg) > 0;

	vector<Mat> mergeImgs({dstImage*0.2, dstImage, dstImage*0.2});
	merge(mergeImgs, colorMask);
    Mat mask_img;
	addWeighted(g_srcImg, 1, colorMask, 0.3, 0, g_vagueBeImg);
	imshow("img",g_vagueBeImg);

   	// if (g_beautyFgImg.data && g_vagueBeImg.data)
	// {
		
	// }
  
}
void TrackbarCanny(int ,void*)
{
	Mat grayImg, blurImg, cannyImg, dilateImg, dilateInvImg;
	cvtColor(g_srcImg, grayImg, COLOR_RGB2GRAY);
	blur(grayImg, blurImg, Size(g_Canny_blur, g_Canny_blur));
	cv::Canny(blurImg, cannyImg, g_Canny_low, g_Canny_upper, 3);
	Mat element = getStructuringElement(MORPH_RECT, Size(g_Canny_dilate, g_Canny_dilate));
	dilate(cannyImg, dilateImg, element);
	bitwise_not(dilateImg, dilateInvImg);
	imshow("img",dilateImg);
}


 
