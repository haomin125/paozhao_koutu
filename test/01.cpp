
#include <sstream>
#include <chrono>

#include <boost/filesystem.hpp>
#include <opencv2/opencv.hpp>

#include "opencv_utils.h"
// #include "running_status.h"
#include "logger.h"
#include "customized_json_config.h"

/////////////////////// MACROS ///////////////////////
////////// Macros for product specific ///////////////
////////// change it with different product //////////

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
/////////////////////// END ///////////////////////////

using namespace cv;
using namespace std;

#define MODEL_HEIGHT 256
#define MODEL_WIDTH 150


enum class ProductSettingFloatMapper : int
{
	SAVE_IMAGE_TYPE = 0, 			// save a image for a given type 0, 1, and 2; 0 means all, 1 means only save defect, and 2 means not save at all
	IMAGE_SAVE_SIZE = 1,
	MODEL_TYPE = 2,
	SENSITIVITY0 = 3, 				// the sensitivity of the model, range (0, 1)
	ALARM_THRESHOLD = 4,     		// the threshold for the alarm to kick defects out, range (0, 1]
	PRESSURE_THRESHOLD = 5,  		// the pressure alarm value, range (0, 1]
    SENSITIVITY1 = 6, 	

    BOARD_COUNT_PER_VIEW = 10,
    TABLET_COUNT_PER_BOARD = 11,
    TABLET_ROI_COL_COUNT = 12,
    TABLET_ROI_ROW_COUNT = 13,
    TABLET_ROI_COL_OFFSET = 14,
    TABLET_ROI_ROW_OFFSET = 15,
    TABLET_CROP_RADIUS = 16,

    X_TEST_OFFSET1 = 17,
    X_TEST_OFFSET2 = 18,

    SPACE_SIZE = 26,
    TEXT_LENGTH1 = 27,
    TEXT_LENGTH2 = 28,

    BLACK_DOT_GRAY_LEVEL_DIFF = 20,
    BLACK_DOT_AREA = 21,
    MISS_CORNER_GRAY_LEVEL_DIFF = 22,
    MISS_CORNER_AREA = 23,
    PVC_GRAY_LEVEL_DIFF = 24,
    PVC_AREA = 25
};



Mat m_workflowImage;
std::vector<cv::Rect> m_tabletRoiRects;

enum class AoiDetectResult : int
{
    GOOD = 1,
    EMPTY_TABLET = 3
};

bool findMaxContour(const Mat &inputImg, vector<vector<Point>> &contours, int &maxAreaIdx, double &maxArea)
{
    maxArea = 0.0;
    maxAreaIdx = -1;

    vector<Vec4i> hierarchy;
    findContours(inputImg, contours, hierarchy, RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    if (hierarchy.size() <= 0)
    {
        return false;
    }

    double tmparea = 0.0;
    for (int i = 0; i < hierarchy.size(); i++)
    {
        tmparea = contourArea(contours[i]);
        if (tmparea >= maxArea)
        {
            maxArea = tmparea;
            maxAreaIdx = i;
        }
    }

    return true;
}


bool imageBinaryByHsv(const Mat &srcImage, const int blurKernelSize, const Scalar &lowerHsv, const Scalar &upperHsv, Mat &dstImage)
{
    try
    {    
        Mat blurImg, hsvImg;
        blur(srcImage, blurImg, Size(blurKernelSize, blurKernelSize));
        // blurImg = srcImage;
        cvtColor(blurImg, hsvImg, CV_BGR2HSV);
        inRange(hsvImg, lowerHsv, upperHsv, dstImage);
        if (upperHsv[0] > 180)
        {
            Mat appendImg;
            inRange(hsvImg, Scalar(0, lowerHsv[1], lowerHsv[2]), Scalar(upperHsv[0] - 180, upperHsv[1], upperHsv[2]), appendImg);
            bitwise_or(dstImage, appendImg, dstImage);
        }

		// Mat cannyImg, openImg;
		// Canny(blurImg, cannyImg, 20, 40);
		// Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
		// morphologyEx(cannyImg, openImg, MORPH_DILATE, kernel);
		// dstImage = (dstImage - openImg) > 0;
    }

    catch(const std::exception& e)
    {
        LogERROR << e.what();
        return false;
    }
    return true;
}


bool locatePillByHsv(const Mat &srcImage, const Scalar &lowerHsv, const Scalar &upperHsv, const int minArea, Mat &resultMask, Point &locatePoint)
{
    //----------------------------hsv-------------------------------
    Mat binaryImg;
    if (!imageBinaryByHsv(srcImage, 9, lowerHsv, upperHsv, binaryImg))
    {
        LogERROR << "imageBinaryByHsv failed";
        return false;
    }


    vector<vector<Point>> contoursTablet;
    int maxAreaIdxTablet(-1);
    double maxAreaTablet(0.0);
    if (!findMaxContour(binaryImg, contoursTablet, maxAreaIdxTablet, maxAreaTablet))
    {
        LogERROR << "locating can't find contours";
        return false;
    }
    if (maxAreaTablet < minArea)
    {
        LogERROR << "locating find the max contour is " << maxAreaTablet << "minArea: " << minArea;
        return false;
    }
    cout<<"----------------minArea-----------------"<<maxAreaTablet<<endl;
    // drawContours(srcImage, contoursTablet, maxAreaIdxTablet, Scalar(255), 2);
    drawContours(resultMask, contoursTablet, maxAreaIdxTablet, Scalar(255), -1);
    Rect box = boundingRect(contoursTablet[maxAreaIdxTablet]);
    locatePoint.x = (box.tl().x + box.br().x) / 2;
    locatePoint.y = (box.tl().y + box.br().y) / 2;

    return true;
}

bool locatePill(cv::Mat &srcImage, cv::Mat &resultMask, cv::Point &locatePoint)
{
	// "lvsu_lowerHsv0" : [140, 10, 67],
    // "lvsu_upperHsv0" : [200, 90, 167],

    // "lvsu_lowerHsv1" : [140, 90, 15],
    // "lvsu_upperHsv1" : [200, 210, 240],
    // cout << "after sealing workflow" << endl;
	Scalar m_lowerHsv(5, 48, 80);
	Scalar m_upperHsv(45, 148, 210);
    return locatePillByHsv(srcImage, m_lowerHsv, m_upperHsv, 5000, resultMask, locatePoint);
}



bool extractImageFrom(const Mat &frame, const vector<cv::Rect> &targetRoiRects, vector<cv::Mat> &croppedImgs, vector<int> &targetResults)
{
	// app need to write test method here
Mat showImg = frame.clone();
  for (int i = 0; i < targetRoiRects.size(); i++)
    {
        Mat cropRoiImg = frame(targetRoiRects[i]);
		rectangle(showImg, targetRoiRects[i], Scalar(255, 0, 0), 2);
        Mat resultMask(cropRoiImg.size(), CV_8UC1, Scalar(0));
        Point locatePoint;
        if (!locatePill(cropRoiImg, resultMask, locatePoint))
        {
            // targetResults.emplace_back(int(AoiDetectResult::EMPTY_TABLET));
            // Mat cropRoiResizeImg;
            // resize(cropRoiImg, cropRoiResizeImg, Size(MODEL_WIDTH, MODEL_HEIGHT));
            // Mat cropImage;
            // cropRoiResizeImg.copyTo(cropImage);
            // croppedImgs.emplace_back(cropImage);
            // continue;
			locatePoint.x = targetRoiRects[i].width/2;
			locatePoint.y = targetRoiRects[i].height/2;
        }
        // opencv_utils::saveImageInTemp(to_string(boardId()) + "_" + to_string(workflowId())+"locateMask", resultMask);
        int left = targetRoiRects[i].x + locatePoint.x - MODEL_WIDTH / 2;
        int right = targetRoiRects[i].x + locatePoint.x + MODEL_WIDTH / 2;
        int top = targetRoiRects[i].y + locatePoint.y - MODEL_HEIGHT / 2;
        int bot = targetRoiRects[i].y + locatePoint.y + MODEL_HEIGHT / 2;
        Mat cropImage;
        Mat cropResizeImg;
        if(left < 0  || right < 0 || right >= m_workflowImage.cols || top < 0 || bot < 0 || bot >= m_workflowImage.rows)
        {
            resize(cropRoiImg, cropResizeImg, Size(MODEL_WIDTH, MODEL_HEIGHT));
			continue;
        }else
        {
            cropImage = m_workflowImage(Rect(Point(left, top), Point(right, bot)));
            resize(cropImage, cropResizeImg, Size(MODEL_WIDTH, MODEL_HEIGHT));

        }

        // Mat cropImage = m_workflowImage(Rect(Point(targetRoiRects[i].x + locatePoint.x - MODEL_WIDTH / 2, targetRoiRects[i].y + locatePoint.y - MODEL_HEIGHT / 2),
        //                                      Point(targetRoiRects[i].x + locatePoint.x + MODEL_WIDTH / 2, targetRoiRects[i].y + locatePoint.y + MODEL_HEIGHT / 2)));
        
        
        // Mat cropResizeImg;
        // resize(m_workflowImage, cropResizeImg, Size(MODEL_WIDTH, MODEL_HEIGHT));

        
        
        croppedImgs.emplace_back(cropResizeImg);
        targetResults.emplace_back(int(AoiDetectResult::GOOD));
    }
	// imshow("frame", frame);
	// imshow("showImg", showImg);
	// waitKey(0);
    return true;
}

int data(int argc, char const *argv[])
{
	// // in c++17 we can use std::filesystem::exists directly, now we are using c++14
	// if (argc < 3 || !boost::filesystem::exists(argv[1]) || !boost::filesystem::exists(argv[2]))
	// {
	// 	cout << "Usage: app_test image_file_path cropped_image_file_path" << endl;
	// 	return 0;
	// }

    int m_iBoardCount = 2;//RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::BOARD_COUNT_PER_VIEW);
    int m_iTabletCountPerBoard = 10;//RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_COUNT_PER_BOARD);
    int m_iColCount = 5;//RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_ROI_COL_COUNT);
    int m_iRowCount = 2;//RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_ROI_ROW_COUNT);
    int m_iColOffset = 20;//RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_ROI_COL_OFFSET);
    int m_iRowOffset = 20;//RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_ROI_ROW_OFFSET);
    int m_iCropRadius = 20;//RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_CROP_RADIUS);


	if (argc < 2 || !boost::filesystem::exists(argv[1]))
	{
		cout << "Usage: app_test image_file_path" << endl;
		return 0;
	}

	std::cout << "App test start...\n";

	// //create log file
	// Logger::instance().setFileName("./app_test.log");
	// Logger::instance().setLoggerLevel(LogLevel::logINFO);

	// // add code to test image processing (cropped part)
	String images_path = argv[1];
	String image_name = argv[2];
	vector<String> images;
	// Mat src, dst;
	// int bottom, top, left, right;
	int crop_width = 200;
	int crop_height = 200;

	cv::glob(images_path, images, false);

	for (size_t i = 0; i < images.size(); i++)
	{			
		try
		{
			Mat workflowImage = imread(images[i]);
			
			int width = workflowImage.cols/2;
			int height = workflowImage.rows/2;
			Mat cropImage = workflowImage(Rect(Point(width-100, height-100),
												Point(width+100, height+100)));
			resize(cropImage,cropImage,Size(256,256));
			imwrite("/opt/history/" + image_name +"/"+ to_string(i)+ ".png",cropImage);
		}
		catch(const std::exception& e)
		{
			cout<< images[i]<<endl;
			std::cerr << e.what() << '\n';
			continue;
		}
		
		
	}
	














	// for (int i = 0; i < images.size(); i++)
	// {
	// 	stringstream iss;
	// 	iss << argv[2] << "/" << i << ".png";

	// 	m_tabletRoiRects.clear();
	// 	string strProdName = "lvsu";
	// 	std::vector<int> m_boardTop = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_" + "boardTop_" + images[i].substr(images[i].find_last_of('/')+1, 3));
	// 	std::vector<int> m_boardBot = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_" + "boardBot_" + images[i].substr(images[i].find_last_of('/')+1, 3));
	// 	std::vector<int> m_boardLeft = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_" + "boardLeft_" + images[i].substr(images[i].find_last_of('/')+1, 3));
	// 	std::vector<int> m_boardRight = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_" + "boardRight_" + images[i].substr(images[i].find_last_of('/')+1, 3));
	// 	std::vector<int> m_tabletRoiEnable = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_TABLET_ROI_ENABLE_" + images[i].substr(images[i].find_last_of('/')+1, 3));
		
	// 	int frameRows = CustomizedJsonConfig::instance().get<int>("FRAME_ROWS");
	// 	int frameCols = CustomizedJsonConfig::instance().get<int>("FRAME_COLS");

	// 	for (int boardIdx = 0; boardIdx < m_iBoardCount; boardIdx++)
	// 	{
	// 		int left = m_boardLeft[boardIdx];
	// 		int top = m_boardTop[boardIdx];
	// 		int right = m_boardRight[boardIdx];
	// 		int bottom = m_boardBot[boardIdx];

	// 		if (top >= bottom || left >= right || top < 0 || left < 0 || bottom >= frameRows || right >= frameCols)
	// 		{
	// 			cout << "board roi is out of range, top: " << top << " ,bottom: " << bottom << " ,left: " << left << " ,right: " << right << endl;
	// 			LogERROR << "board roi is out of range, top: " << top << " ,bottom: " << bottom << " ,left: " << left << " ,right: " << right;
	// 			return false;
	// 		}

	// 		int tabletHeight = (bottom-top + m_iRowOffset * (m_iRowCount - 1)) / m_iRowCount;
	// 		int tabletWidth = (right-left + m_iColOffset * (m_iColCount - 1)) / m_iColCount;

	// 		for (int r = 0; r < m_iRowCount; r++)
	// 		{
	// 			for (int c = 0; c < m_iColCount; c++)
	// 			{
	// 				if (m_tabletRoiEnable[boardIdx * m_iTabletCountPerBoard + r * m_iColCount + c] == 0)
	// 				{
	// 					continue;
	// 				}
	// 				int tabletTop = (tabletHeight - m_iRowOffset) * r;
	// 				int tabletBot = (tabletHeight - m_iRowOffset) * r + tabletHeight;
	// 				int tabletLeft = (tabletWidth - m_iColOffset) * c;
	// 				int tabletRight = (tabletWidth - m_iColOffset) * c + tabletWidth;
	// 				m_tabletRoiRects.emplace_back(Rect(Point(tabletLeft + left, tabletTop + top), Point(tabletRight + left, tabletBot + top)));
	// 			}
	// 		}
	// 	}



	// 	src = cv::imread(images[i]);
	// 	if (!src.empty())
	// 	{
	// 		m_workflowImage = src.clone();
	// 		auto startTime = chrono::system_clock::now();
		
		
	// 		vector<Mat> targetImgs;
	// 		vector<int> targetResults;

	// 		if (!extractImageFrom(m_workflowImage, m_tabletRoiRects, targetImgs, targetResults))
	// 		{
	// 			LogERROR << "Extract target object failed!";
	// 			return false;
	// 		}
	// 		for (int targetIdx = 0; targetIdx < targetImgs.size(); targetIdx++)
	// 		{
	// 			opencv_utils::saveImageInTemp("cropedImg", targetImgs[targetIdx]);
	// 		}
	// 			// auto endTime = chrono::system_clock::now();
	// 			// imwrite(iss.str(), dst);
	// 			// LogINFO << "process time in ms: " << chrono::duration_cast<chrono::microseconds>(endTime - startTime).count();
	// 	}
	// }

	// std::cout << "App test end...\n";
	// return true;



/////////////////////////////////////////////////////////////////////////////////////////////////////////

	std::cout << "App test start...\n";
	Mat srcImage = imread(argv[1]);
	// Mat dstImage;
	// imageBinaryByHsv(srcImage, 9, Scalar(6,22,atoi(argv[2])), Scalar(46,119,239), dstImage);
	// imwrite("0.png",dstImage);
	string windowName = "get HSV";
    namedWindow(windowName, 0);
    setWindowProperty(windowName, WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
    resizeWindow(windowName, 612, 512);
    Rect roi = selectROI(windowName, srcImage, 1);

	Mat selectedRoiImg = srcImage(roi);
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
	blur(srcImage, srcBlurImg, Size(5, 5));
	cvtColor(srcBlurImg, srcHsvImg, COLOR_BGR2HSV);
	// imshow(windowName, srcHsvImg);
	// waitKey(0);

	// vector<Mat> hsvImages;
	// split(srcHsvImg, hsvImages);
	// Mat subImg = hsvImages[2] - hsvImages[1];

	// vector<Mat> rgbImages;
	// split(srcBlurImg, rgbImages);

	// Mat binaryImg;
	// threshold(rgbImages[2], binaryImg, 135, 255, THRESH_BINARY);
	// imshow(windowName, binaryImg);
	// waitKey(0);


    Scalar hsvLower(hsvMean[0] - 20, hsvMean[1] - 20, hsvMean[2] - 80);
    Scalar hsvUpper(hsvMean[0] + 20, hsvMean[1] + 80, hsvMean[2] + 80);

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
	addWeighted(srcImage, 1, colorMask, 0.3, 0, mask_img);

	imshow(windowName, mask_img);
	waitKey(0);




    Mat dstImage;
    imageBinaryByHsv(srcBlurImg, 9, hsvLower, hsvUpper, dstImage);

	Mat colorMask_;
	vector<Mat> mergeImgs_({dstImage*0.4, dstImage, dstImage*0.4});
	merge(mergeImgs_, colorMask_);
    Mat mask_img_;
	addWeighted(srcImage, 1, colorMask_, 0.3, 0, mask_img_);

	imshow(windowName, mask_img_);
	waitKey(0);

	imwrite("binaryImg.png", binaryImg);

	return 0;
}

