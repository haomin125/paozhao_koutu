#include "xj_app_workflow.h"
#include "xj_app_config.h"
#include "running_status.h"
#include "customized_json_config.h"
#include "opencv_utils.h"
#include "shared_utils.h"
#include "timer_utils.hpp"


using namespace std;
using namespace cv;
 
#define FRAME_ROWS 2048
#define FRAME_COLS 2448
// #define FRAME_ROWS 1050
// #define FRAME_COLS 2300

// #define MODEL_CATEGORY 23

enum class AoiDetectResult : int
{
    UNKNOWN = 0,
    GOOD = 1,
    EMPTY_TABLET = 4
};

AppWorkflow::AppWorkflow(const int workflowId, const std::shared_ptr<WorkflowConfig> &pConfig) : BaseWorkflow(workflowId, pConfig)
{
    m_fSensitivity = RunningInfo::instance().GetProductSetting().GetFloatSetting((int)ProductSettingFloatMapper::SENSITIVITY);
    cout<<"m_fSensitivity: "<<m_fSensitivity<<endl;
}

bool AppWorkflow::loadTensorrtModel(const std::string engineName, const TensorrtOutputType tensorrtOutputType, const int batchsize, const int category, const int width, const int height, const int channel)
{
    m_tensorrtDL = make_shared<TensorrtClassifier>(engineName, TensorrtOutputType::CATEGORY_SEGMENT, batchsize, category, width, height, channel);
    if(!m_tensorrtDL->loadModel())
    {
        LogERROR << "load model failed";
        return false;
    }
    // dlWarmUp();

    return true;
}

bool AppWorkflow::loadTensorrtModel_koutu(const std::string engineName, const int batchsize, const int category, const int width, const int height, const int channel)
{
    m_tensorrtDL_seg = make_shared<TensorrtClassifier>(engineName, TensorrtOutputType::SEGMENT, batchsize, category, width, height, channel);
    if(!m_tensorrtDL_seg->loadModel())
    {
        LogERROR << "load seg_model failed";
        return false;
    }
    // dlWarmUp();

    return true;
}

bool AppWorkflow::dlWarmUp()
{
    Mat testImg = Mat::zeros(Size(256, 256), CV_8UC3);
    vector<int> vClsIndex; 
	vector<Mat> vMaskImg;
    vector<float> good_score;
    m_tensorrtDL->getClassifyAndSegmentResult({testImg}, {m_fSensitivity}, vClsIndex, vMaskImg, good_score);
    
    return true;
}



int AppWorkflow::mergeSegmentResult(const int originalResult, const int segmentResult)
{	
	// cout<<"--------------"<<endl;
    // cout<<"segmentResult: "<<segmentResult<<",originalResult:"<<originalResult<<endl;
	if (segmentResult != 1)
	{
        // cout<<segmentResult<<"  ======="<<endl;
		return segmentResult;
	}
    else
	{
		for (int i = 0; i < m_segParameters.size(); i+=2)
		{
			if (originalResult - 1 == m_segParameters[i])
			{
				return (int)ClassifierResultConstant::Good;
			}
		}
	}
    // cout<<"originalResult"<<originalResult+1<<endl;
	return originalResult+1;
}
int AppWorkflow::segMaskResult(const cv::Mat & mask,const int Index,const double area,bool new_model)
{
    // cout<<"==================="<<endl;/
    int segIndex = m_segParameters[Index];
    Mat binaryImg;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
    int segIndex_;
    // cout<<"============== "<<new_model<<" ============"<<Index<<endl;
    if(new_model)
        segIndex_ = Index/2+1;
    else
        segIndex_ = segIndex;

    
    // cout<<"segIndex: "<<segIndex<<" ,segIndex_: "<<segIndex_<<endl;//cgc
 
	inRange(mask,Scalar(segIndex_),Scalar(segIndex_),binaryImg);
	findContours(binaryImg,contours,hierarchy,RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);
	if (contours.size() > 0)
	{
		int sumArea = 0;
		for (int i = 0; i < contours.size(); i++)
		{
			int defectArea = contourArea(contours[i]);
			// cout<<defectArea<<endl;
			if (defectArea > 2)
			{
				sumArea += defectArea;
			}
			if (sumArea > area)
			{
				cout<<"-------------"<<segIndex<<" "<<contours.size()<<"	sumArea:"<<sumArea<<" ,area:"<<area<<endl;
				return ClassifierResult::getResult("defect"+to_string(segIndex));
			}
		}
	}
    // cout<<"=====================  "<< (int)ClassifierResultConstant::Good<<endl;
	return (int)ClassifierResultConstant::Good;
}

bool AppWorkflow::result_handle(std::vector<cv::Mat> &srcImage,std::vector<int> &class_result,std::vector<int> &result)
{
    // cout<<"image_number: "<<srcImage.size()<<endl;

    

    for (size_t i = 0; i < srcImage.size(); i++)
    {
        
        int segIndex=0;
        for (size_t class_index = 0; class_index < m_segParameters.size(); class_index++)
        {
            segIndex = segMaskResult(srcImage[i],class_index,m_segParameters[class_index+1],m_jiaonang);
            if(segIndex>12)
                break;
            class_index++;
        }
        // cout<<"result["<<i<<"]: "<<class_result[i]<<endl;
        int result_1 = mergeSegmentResult(class_result[i],segIndex);
        // cout<<"result["<<i<<"]: "<<class_result[i]<<endl;
        result.emplace_back(result_1); 
            
    }
    
}
bool AppWorkflow::extractImageFrom(const Mat &frame, const vector<cv::Rect> &targetRoiRects, vector<cv::Mat> &croppedImgs, vector<int> &targetResults)
{
    for (int i = 0; i < targetRoiRects.size(); i++)
    {                   
        opencv_utils::saveImageInTemp("/test1/" + to_string(boardId()) + "_" + to_string(workflowId()), frame(targetRoiRects[i]));
        int targetResult(int(AoiDetectResult::UNKNOWN)); 
        if(m_is_pihao)
            if (i >= targetRoiRects.size() - m_lotNumberRoiRects.size())
            {
                Mat textRoiImg = frame(Rect(Point(targetRoiRects[i].tl().x,targetRoiRects[i].tl().y),Point(targetRoiRects[i].br().x,targetRoiRects[i].br().y))).clone();
                Mat cropResizeImg;
                CharacterFilling(textRoiImg,croppedImgs);
                targetResults.emplace_back(1);
                opencv_utils::saveImageInTemp("/test/" + to_string(boardId()) + "_" + to_string(workflowId()), croppedImgs[i]);
                continue;
            }
        
        //模型扣图
        if (m_is_koutu)
        {
            targetResults.emplace_back(1);
            Mat saas;
            resize(frame(targetRoiRects[i]), saas, Size(m_iCropWidth, m_iCropHeight));
            croppedImgs.emplace_back(saas);
            opencv_utils::saveImageInTemp("/test/" + to_string(boardId()) + "_" + to_string(workflowId()), croppedImgs[i]);
            continue;
        }

        Mat cropRoiImg = frame(targetRoiRects[i]);
        vector<Point> maxContour;
        Point locatePoint;
        if (!locatePill(cropRoiImg, maxContour, locatePoint))
        {
            locatePoint.x = cropRoiImg.cols/2;
            locatePoint.y = cropRoiImg.rows/2;
            targetResult = int(AoiDetectResult::EMPTY_TABLET);
        }
        
        int cropLeft = targetRoiRects[i].x + locatePoint.x - m_width / 2;
        int cropRight = targetRoiRects[i].x + locatePoint.x + m_width / 2;
        int cropTop = targetRoiRects[i].y + locatePoint.y - m_height / 2;
        int cropBot = targetRoiRects[i].y + locatePoint.y + m_height / 2;
        if (cropLeft < 0)
        {
            cropLeft = 0;
            cropRight = m_width;
        }
        if (cropRight >= m_workflowImage.cols)
        {
            cropLeft = m_workflowImage.cols - 1 - m_width;
            cropRight = m_workflowImage.cols - 1;
        }
        

        Mat cropImage = m_workflowImage(Rect(Point(cropLeft, cropTop),
                                             Point(cropRight, cropBot)));

        Mat cropResizeImg;
        resize(cropImage, cropResizeImg, Size(m_iCropWidth, m_iCropHeight));
        // m_tabletRoiRects.emplace_back(targetRoiRects[i]);
        croppedImgs.emplace_back(cropResizeImg);
        targetResults.emplace_back(targetResult);
    }

    return true;
}
void AppWorkflow::CharacterFilling(const Mat & handleImg,vector<Mat> & croppedImgs)
{

    // opencv_utils::saveImageInTemp("/test/" + to_string(boardId()) + "_" + to_string(workflowId()), handleImg);
	Rect rect1(Point(0 , 0),Point( handleImg.cols,handleImg.rows ));
    int tb1 = (256*2) - rect1.height; 
    Mat charImg1 = handleImg(rect1).clone();
    if (tb1 < 0)
    {
        tb1 = 0;
    }
    copyMakeBorder(charImg1,charImg1,0,tb1,0,256 * 2 - charImg1.cols ,BORDER_CONSTANT,Scalar(0,0,0));
    // opencv_utils::saveImageInTemp("/test/" + to_string(boardId()) + "_" + to_string(workflowId()), charImg1);
    resize(charImg1,charImg1,Size(m_iCropWidth, m_iCropHeight));
    // opencv_utils::saveImageInTemp("/test/" + to_string(boardId()) + "_" + to_string(workflowId()), charImg1);
	croppedImgs.emplace_back(charImg1);
}

bool AppWorkflow::findBox1(const vector<vector<Point>> contours, const int SetArea, vector<Rect> &box)
{    
    for (int j = 0; j < contours.size(); j++)
    {
        int defectArea = contourArea(contours.at(j));
        cout<< "defectArea================" << defectArea<<endl;
        cout<< "v_Area================" << SetArea <<endl;
        if (defectArea > SetArea)
        {
            Rect box_ = boundingRect(contours[j]);
            box.emplace_back(box_);
        }
    }
    return true;
}

bool AppWorkflow::findBox(const Mat img, const int num, vector<vector<Rect>> &box)
{ 
    vector<Mat> vbinaryImg;
    for (int i = 1; i < num; i++)  //num:背景0，药板1、药粒2、批号3
    {
        Mat binaryImg;
        inRange(img,Scalar(i),Scalar(i),binaryImg);
        opencv_utils::saveImageInTemp("/out/" + to_string(boardId()) + "_aaaa" + to_string(workflowId()), binaryImg);
        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;
        findContours(binaryImg,contours,hierarchy,RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);
        vector<int> v_Area = {1000, 300, 3};
        vector<Rect> v_box;
        findBox1(contours, v_Area.at(i-1), v_box);  //根据设定的面积大小获取药板、药粒、批号的ROI

        // 药板按X轴进行排序
        if (i == 1)
        {            
            sort(v_box.begin(), v_box.end(), [=](auto p1, auto p2){
                // if(workflowId() == 1)
                // {
                //     return p1.x > p2.x;
                // }
                return p1.x < p2.x;
            });
        }      
        box.emplace_back(v_box);        
    }
    return true;
}

bool AppWorkflow::tabletRoi(std::vector<cv::Rect> &boardRoiRects, std::vector<cv::Rect> &yaoliRects, const int &tabletId, std::vector<cv::Rect> &v_yaoliRoi, std::vector<int> &targetResults)
{
    int m_iTabletCountPerBoard_ = m_iColCount * m_iRowCount;
    if (m_lastColCount != 0 && tabletId == 2)  //处理第二版药不全版面的药粒
    {   
        if(workflowId() == 0)
        {
            sort(yaoliRects.begin(), yaoliRects.end(), [=](auto p1, auto p2){return p1.x < p2.x;});
        }else
        {
            sort(yaoliRects.begin(), yaoliRects.end(), [=](auto p1, auto p2){return p1.x > p2.x;});
        }
    }
    int num(0);
    for (int i = 0; i < yaoliRects.size(); i++)
    {
        Rect intersection = boardRoiRects.at(tabletId) & yaoliRects.at(i); //药粒与药板是否相交
        if(!intersection.empty())
        {
            num++;
            if(num > m_iTabletCountPerBoard_)
            {
                if (m_lastColCount != 0 && tabletId == 2) //不全药板
                {
                    break;
                }
                cout << "药粒分割数量多" << endl;
                cout << "boardid_ " << workflowId() << " UI设置数量 " << m_iTabletCountPerBoard_ << " 模型检出数量： " << num << endl;
                return false;
            }
            v_yaoliRoi.emplace_back(yaoliRects.at(i));
            targetResults.emplace_back(1);
        }
    }
    if(num < m_iTabletCountPerBoard_)
    {
        cout << "药粒分割数量少" << endl;
        cout << "boardid_ " << workflowId() << " UI设置数量 " << m_iTabletCountPerBoard_ << " 模型检出数量： " << num << endl;
        return false;
    }
    return true;
}

bool AppWorkflow::piHaoRoi(std::vector<cv::Rect> &boardRoiRects, std::vector<cv::Rect> &pihaoRects, const int &tabletId, std::vector<cv::Rect> &v_pihaoRoi, std::vector<int> &targetResults)
{
    int num(0);
    int lotNumCount = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::LOTNUM_COUNT_PER_BOARD);
    for (int i = 0; i < pihaoRects.size(); i++)
    {
        Rect intersection = boardRoiRects.at(tabletId) & pihaoRects.at(i); //批号与药板是否相交
        if(!intersection.empty())
        {
            num++;
            if (num > lotNumCount)
            {
                cout << "批号分割数量多" << endl;
                return false;
            }                    
            v_pihaoRoi.emplace_back(pihaoRects.at(i));
            targetResults.emplace_back(1);
        }
    }
    if (num < lotNumCount)
    {
        cout << "批号分割数量少" << endl;
        return false;
    }
    return true;
}

bool AppWorkflow::boardRoi(std::vector<cv::Rect> &boardRoiRects, std::vector<cv::Rect> &boardRects, std::vector<cv::Rect> &v_boardRoi)
{
    for (int i = 0; i < boardRoiRects.size(); i++)
    {
        Rect intersection, maxRect;
        for (int j = 0; j < boardRects.size(); j++)
        {
            cout << "m_boardRoiRects_size_" << m_boardRoiRects.size() << "____" << endl;
            intersection = boardRoiRects.at(i) & boardRects.at(j); //UI药板与model药板是否相交
            if(intersection.area() > maxRect.area())
            {
                maxRect = boardRects.at(j);
            }
        }
        if(!maxRect.empty())
        {
            v_boardRoi.emplace_back(maxRect);
        }
    }
    if (boardRoiRects.size() != v_boardRoi.size())
    {
        return false;
    }    
    return true;
}
bool AppWorkflow::position(const Mat &frame, const vector<cv::Rect> &targetRoiRects, vector<cv::Mat> &croppedImgs, vector<int> &targetResults)
{
    vector<Mat> testMaskImg;   
    vector<Mat> vMaskImg_seg, m_workflowImage_mask;
    vector<Rect> v_rect;
    m_tabletRoiRects.clear();
    Mat m_workflowImage_;
    // Mat test = frame.clone();
    // Mat test = imread("test.png");
    resize(frame, m_workflowImage_, Size(320,256));
    testMaskImg.emplace_back(m_workflowImage_);
    timer_utils::Timer<chrono::microseconds> detectTimer;
    detectTimer.Reset();    
    m_tensorrtDL_seg->getSegmentResult(testMaskImg, vMaskImg_seg);
	opencv_utils::saveImageInTemp("/vMaskImg_seg/" + to_string(boardId()) + "_" + to_string(workflowId()), vMaskImg_seg[0]);
    cout << "vMaskImg_seg________________" << vMaskImg_seg.size() << endl;
    // cout << "模型推理 time: " <<  "]   " << detectTimer.Elapsed().count() << endl;
    // opencv_utils::saveImageInTemp("/out/" + to_string(boardId()) + "_aa_" + to_string(workflowId()), vMaskImg_seg[0]);
    
    //step1:计算模型分割数量
    MatND hist;
    int histSize = 256;
    float range[] = {0, 256};
    const float* hisRange = {range};
    calcHist(&vMaskImg_seg.at(0), 1, 0, Mat(), hist, 1, &histSize, &hisRange);
    int nonZeroCount = 0;
    for (int i = 0; i < histSize; i++)
    {
        if(hist.at<float>(i) > 0)
        {
            ++nonZeroCount;
        }
    }
    //step2:将模型分割出来的结果存到数组里面，按药板、药粒、批号顺序。  
    vector<vector<Rect>> box;
    findBox(vMaskImg_seg.at(0), nonZeroCount, box);
    

    if (box.size() < 2 || box.size() > 3)   //分割数量只能是二或三
    {
        cout << "模型分割数量不对，药板药粒或批号异常" << box.size() << endl;        
        return false;
    }    

    //step3: 找药板,药粒和批号
    vector<vector<Rect>> v_v_tabletRoi;
    vector<Rect> v_yaoliRoi_, v_pihaoRoi_, v_boardRoi;
    if(!boardRoi(m_boardRoiRects, box.at(0), v_boardRoi))
    {
        cout << "药板数量不对" << v_boardRoi.size() << endl;
        return false;
    }

    for (int tabletId = 0; tabletId < v_boardRoi.size(); tabletId++)  
    {    
        vector<Rect> v_yaoliRoi, v_pihaoRoi;
        if (m_lastColCount != 0)
        {
            int m_lastColCount_ = workflowId()==0 ? m_lastColCount : m_iColCount - m_lastColCount; //列数是单数的时候需要打开
            m_iColCount = tabletId==0 ? m_iColCount : m_lastColCount_;          //UI上第二个药板框是属于不全版面的
        }   

        cout << "m_boardRoiRects__" << m_boardRoiRects[tabletId] << "____" << tabletId << endl;
        cout << "v_boardRoi" << v_boardRoi[tabletId] << "____" << tabletId << endl;
        cout << "box.at()" << box.at(0).at(tabletId) << "____" << tabletId << endl;
        
        //step3-1 找药粒,按药板顺序进行排列。分割数量不对的需要补充相应数量是NG的ROI
        if(!tabletRoi(v_boardRoi, box.at(1), tabletId, v_yaoliRoi, targetResults))
        {
            int m_iTabletCountPerBoard = m_iColCount * m_iRowCount;  //每板药的药粒数
            v_yaoliRoi.clear();
            for (size_t i = 0; i < m_iTabletCountPerBoard; i++)
            {
                v_yaoliRoi.emplace_back(Rect(v_boardRoi[tabletId].x, v_boardRoi[tabletId].y,30,30));
            }
        }

        //step3-2 找批号
        if(m_is_pihao)  
        {
            int lotNumCount = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::LOTNUM_COUNT_PER_BOARD);
            if (box.size() == 3)
            {
                if (!piHaoRoi(v_boardRoi, box.at(2), tabletId, v_pihaoRoi, targetResults))
                {
                    v_pihaoRoi.clear();
                    for (size_t i = 0; i < lotNumCount; i++)
                    {
                        v_pihaoRoi.emplace_back(Rect(v_boardRoi[tabletId].x, v_boardRoi[tabletId].y,30,30));
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < lotNumCount; i++)
                {
                    v_pihaoRoi.emplace_back(Rect(v_boardRoi[tabletId].x, v_boardRoi[tabletId].y,30,30));
                }
                cout << "模型分割输出里没有批号" << box.size() << endl; 
            }
        }

        for (size_t i = 0; i < v_yaoliRoi.size(); i++)
        {
            v_yaoliRoi_.emplace_back(v_yaoliRoi[i]);
        }
        for (size_t i = 0; i < v_pihaoRoi.size(); i++)
        {
            v_pihaoRoi_.emplace_back(v_pihaoRoi[i]);
        }
    }
    v_v_tabletRoi.emplace_back(v_yaoliRoi_);
    v_v_tabletRoi.emplace_back(v_pihaoRoi_);
            
    // double scale_x = m_workflowImage.cols/320.0; 
    // double scale_y = m_workflowImage.rows/256.0;
    double scale_x = IMAGE_WIDTH/KOUTU_WIDTH;
    double scale_y = IMAGE_HEIGHT/KOUTU_HEIGHT;
    for (int i = 0; i < v_v_tabletRoi.size(); i++)
    {
        for (int j = 0; j < v_v_tabletRoi.at(i).size(); j++)
        {
            // opencv_utils::saveImageInTemp("/out/" + to_string(boardId()) + "_" + to_string(workflowId()), m_workflowImage_(v_v_tabletRoi.at(i).at(j)));
            // 药粒或批号中心
            Point locatePoint;
            locatePoint.x = (v_v_tabletRoi.at(i).at(j).tl().x + v_v_tabletRoi.at(i).at(j).width/2) * scale_x;
            locatePoint.y = (v_v_tabletRoi.at(i).at(j).tl().y + v_v_tabletRoi.at(i).at(j).height/2) * scale_y;
            // cout << "locatePoint-------------" << locatePoint << endl;
            // cout << "locatePoint-------------" << v_v_tabletRoi.at(i).at(j).tl().y * scale_y << endl;

            int v_width, v_height;
            int rectStartIdx = int(ProductSettingFloatMapper::BOX_START_INDEX) + (boardId() + workflowId()) * 48 + 4 * 4;
            //根据中心找扣图框
            if (i == 0)
            {                    
                v_width = m_width;
                v_height = m_height;
            }
            if (i == 1)
            {
                v_width = RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + 2);;
                v_height = RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + 3);;
                // cout << "v_width-------------" << v_width << endl;
                // cout << "v_height-------------" << v_height << endl;
            }
            
            int cropLeft = locatePoint.x - v_width / 2;
            int cropRight =locatePoint.x + v_width / 2;
            int cropTop =locatePoint.y - v_height / 2;
            int cropBot =locatePoint.y + v_height / 2;
            if (cropLeft < 0)
            {
                cropLeft = 0;
                cropRight = v_width;
            }
            if (cropRight >= m_workflowImage.cols)
            {
                cropLeft = m_workflowImage.cols - 1 - v_width;
                cropRight = m_workflowImage.cols - 1;
            }
            m_tabletRoiRects.emplace_back(Rect(Point(cropLeft, cropTop),Point(cropRight, cropBot)));
            // m_tabletRoiRects.emplace_back(Rect(Point(box.tl().x * scale_x, box.tl().y * scale_y), Point(box.br().x * scale_x, box.br().y * scale_y)));
            Mat cropImage = m_workflowImage(Rect(Point(cropLeft, cropTop),Point(cropRight, cropBot)));
            Mat cropResizeImg;
            if (i == 0)
            {
                resize(cropImage, cropResizeImg, Size(m_iCropWidth, m_iCropHeight));
                croppedImgs.emplace_back(cropResizeImg);
            }                   
            if (i == 1)
            {
                CharacterFilling(cropImage,croppedImgs);
            }       
            // rectangle(m_workflowImage_, box, Scalar(0, 0, 255), 2);
            opencv_utils::saveImageInTemp("/out/" + to_string(boardId()) + "_v" + to_string(workflowId()), cropImage);            
        }                     
    }

    // opencv_utils::saveImageInTemp("/out/" + to_string(boardId()) + "_" + to_string(workflowId()), test);
   /*
    //批号
    m_lotNumberRoiRects.clear();
    int lotNumCount = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::LOTNUM_COUNT_PER_BOARD);
    int rectStartIdx = int(ProductSettingFloatMapper::BOX_START_INDEX) + (2*boardId() + workflowId()) * 48 + 4 * 4;
    for (int boardIdx = 0; boardIdx < m_iBoardCount; boardIdx++)
    {        
        // if(workflowId()==0 and boardIdx ==1)
            //     continue;
        int left1 = v_rect[boardIdx].tl().x;
        int top1 = v_rect[boardIdx].tl().y;
        int right1 = left1 + RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + 2);
        int bottom1 = top1 + RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + 3);
        cout << "top1: " << top1 << " ,bottom1: " << bottom1 << " ,left1: " << left1 << " ,right1: " << right1 << endl;
        // if (top >= bottom || left >= right || top < 0 || left < 0 || bottom >= FRAME_ROWS || right >= FRAME_COLS)
        // {
        //     LogERROR << "board roi is out of range, top: " << top << " ,bottom: " << bottom << " ,left: " << left << " ,right: " << right;
        //     return false;
        // }
        m_lotNumberRoiRects.emplace_back(Rect(Point(left1, top1), Point(right1, bottom1)));
        // cout << left << " " << top << " " << right << " " << bottom << endl;
        if(m_is_pihao)
            m_tabletRoiRects.emplace_back(Rect(Point(left1, top1), Point(right1, bottom1)));
    }*/
      
    cout << "模型推理 time: " <<  "]   " << detectTimer.Elapsed().count() << endl;
    return true;

}

bool AppWorkflow::imagePreProcess()
{
	// extract frame into image in targets of the view
	shared_ptr<View> pView = getView();
	if (pView == nullptr || !pView->isValid() || m_workflowImage.empty())
	{
		return false;
	}

	try
	{
        if (workflowId()==0)
        {
            m_workflowImage = imread("/opt/videos/0.png");
            // return true;
        }   
        if (workflowId()==1)
        {
            m_workflowImage = imread("/opt/videos/1.png");
            // return true;
        }   
		opencv_utils::saveImageInTemp("/origin/" + to_string(boardId()) + "_" + to_string(workflowId()), m_workflowImage);
            // return true;

		vector<Mat> targetImgs;
        vector<int> targetResults;

        //扣图分割推理        /*
        if (m_is_koutu)
        {
            if(!position(m_workflowImage, m_tabletRoiRects, targetImgs, targetResults))
            {
                LogERROR << "Extract target object failed!";
		    	return false;
            }
        }
        
        
        if(m_pngconvert)
            rotate(m_workflowImage,m_workflowImage,ROTATE_180);
            ROTATE_90_CLOCKWISE;

        // if (boardId() == 1)
        //      m_workflowImage = imread("./" + to_string(boardId()) + "_" + to_string(workflowId()) + "_test.png");
        // if (workflowId() == 1)
        //     return false;

		// if (!extractImageFrom(m_workflowImage, m_tabletRoiRects, targetImgs, targetResults))
		// {
		// 	LogERROR << "Extract target object failed!";
		// 	return false;
		// }
        vector<int> result;

        //模型推理
        if(m_usemodel)
        {
            m_score.clear();
            vector<int> vClsIndex; 
            vector<Mat> vMaskImg;
            for (size_t i = 0; i < targetImgs.size(); i++)
            {
                opencv_utils::saveImageInTemp("/targets/" + to_string(boardId()) + to_string(workflowId()), targetImgs[i]);
            }
            m_tensorrtDL->getClassifyAndSegmentResult(targetImgs, {m_fSensitivity}, vClsIndex, vMaskImg, m_score);

            result_handle(vMaskImg,vClsIndex,result);

            cout<<"result"<<workflowId()<<": ";
            for (size_t i = 0; i < result.size(); i++)
            {
                cout<<result[i]<<" ";
            }
            cout<<endl;
        }
        else
            for (size_t i = 0; i < targetResults.size(); i++)
            {
                // cout<<"================================="<<endl;
                if(targetResults[i]==0)
                    result.emplace_back(1);
                else
                    result.emplace_back(targetResults[i]);
            }
        
        // saveTargetObjs(result, m_score,targetImgs);
        
		for (int targetIdx = 0; targetIdx < pView->targetsSize(); targetIdx++)
		{
			if (pView->getTarget(targetIdx) != nullptr)
			{
				if (targetIdx < m_tabletRoiRects.size())
				{
                    if(targetResults[targetIdx]==4)
                    {
                        result[targetIdx]=4;
                    }

					pView->getTarget(targetIdx)->setImage(targetImgs[targetIdx]);
                    // pView->getTarget(targetIdx)->setImage(img);
					pView->getTarget(targetIdx)->setBoundMargin(m_tabletRoiRects[targetIdx].tl().y, m_tabletRoiRects[targetIdx].br().y, m_tabletRoiRects[targetIdx].tl().x, m_tabletRoiRects[targetIdx].br().x);
                    pView->getTarget(targetIdx)->setResult(result[targetIdx]);
				}
				else
				{
					pView->getTarget(targetIdx)->setImage(m_fillerimg);
                    pView->getTarget(targetIdx)->setBoundMargin(0, 0, 0, 0);
					pView->getTarget(targetIdx)->setResult(6);
				}
			}
		}
	}
	catch (const Exception &e)
	{
		LogERROR << "Catch opencv exception: " << e.what();
		return false;
	}
	catch (const exception &e)
	{
		LogERROR << "Catch standard exception: " << e.what();
		return false;
	}

	return true;
}
bool AppWorkflow::saveTargetObjs(const std::vector<int> &result, const std::vector<float> &good_score,const std::vector<cv::Mat> &targetImages)
{
    
    string strProdName = RunningInfo::instance().GetProductionInfo().GetCurrentProd();
    string strProdLot = RunningInfo::instance().GetProductionInfo().GetCurrentLot();
    for (int targetIdx = 0; targetIdx < result.size(); ++targetIdx)
    {
        string str = to_string(good_score[targetIdx]);
        // save image
        std::string curTime = shared_utils::getTime();
        curTime.erase(std::remove(curTime.begin(), curTime.end(), ':'), curTime.end());
        std::string imageName = "C" + to_string(result[targetIdx]) + "-TM" + curTime + "-B" + std::to_string(boardId()) + "-V" + 
                    std::to_string(getView()->viewId()) + "-T" + std::to_string(targetIdx) + "-" + strProdName + "-" + strProdLot 
                    + "-GS"+str.substr(str.find(".")+1,6)
                    + ".png";
        Mat saveImg = targetImages.at(targetIdx);
        // if (saveImg.channels() == 3)
        // {
        //     cvtColor(saveImg, saveImg, cv::COLOR_BGR2GRAY);
        // }
        // if(result[targetIdx]==4)
        //     continue;

        if (result[targetIdx] != (int)AoiDetectResult::GOOD)
        {
            // getView()->getTarget(0)->setResult(targetObj.result);
            cv::imwrite("/opt/history/bad/" + imageName, saveImg);
            LogINFO << "===>save image to /opt/history/bad/" << imageName;
        }
        else
        {
            cv::imwrite("/opt/history/good/" + imageName, saveImg);
            LogINFO << "===>save image to /opt/history/good/" << imageName;

        }
    }
}
bool AppWorkflow::computerVisionProcess()
{

	// app need to write its own code to handle computer vision related process properly
	// this method only works with framework 5.0.0.0 and up
    // checkBroken();
    Mat result = Mat::zeros(m_workflowImage.rows, m_workflowImage.cols, CV_8UC1);
    // checkBoardDefect(m_workflowImage, result);//cgc
    // checkBlackSpot();
	return true;
}

void AppWorkflow::drawDesignedTargets(const double scale)
{
	// app need to write its own draw target method
	return;
}

bool AppWorkflow::reconfigParameters()
{
	string strProdName = RunningInfo::instance().GetProductionInfo().GetCurrentProd();
    m_strProdName = strProdName;
    if (!RunningInfo::instance().GetProductSetting().UpdateCurrentProductSettingFromDB(strProdName))
    {
        LogDEBUG << "Read " << strProdName << " setting from database failed!";
        return false;
    }

    m_iBoardCount = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::BOARD_COUNT_PER_VIEW);
    m_iTabletCountPerBoard = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_COUNT_PER_BOARD);
    m_iColCount = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_ROI_COL_COUNT);
    m_iRowCount = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_ROI_ROW_COUNT);
    m_iColOffset = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_ROI_COL_OFFSET);
    m_iRowOffset = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_ROI_ROW_OFFSET);
    
    //////////////////////////////////////////////////////////
    m_lastColCount = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::BANMIANBUQUAN_COLS);
    m_pihao_position = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::LOT_POSITION);
    m_jiaonang = bool(RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::USE_JIAONANG));
    m_is_pihao = bool(RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::USE_PIHAO));
    m_usemodel = bool(RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::USE_MODEL));
    cout<<m_usemodel<<"============================================"<<endl;
    m_areamin = CustomizedJsonConfig::instance().get<int>(strProdName + "_areamin");
    m_segclass = CustomizedJsonConfig::instance().getVector<int>(strProdName+"_segmentclass");
    m_segParameters.clear();
    cout<<"m_segParameters"<<m_segclass.size()<<": ";
	for (int i = 0; i < m_segclass.size(); i++)
	{
   		int value = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::DEFECT_CUCAO + m_segclass[i] - 12);
		m_segParameters.emplace_back(m_segclass[i]);
		m_segParameters.emplace_back(value);
        cout<<m_segclass[i]<<":"<<value<<"  ";
	}
 
	// cout << m_segParameters << endl;
    
    vector<int> H_W = CustomizedJsonConfig::instance().getVector<int>(strProdName+"_H_W");
    m_height = H_W[1];
    m_width = H_W[0];
    // 抠图、瑕疵检测参数
    m_iPillRadius = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_RADIUS);
    m_iTabletMatchScore = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::TABLET_MATCH_SCORE);

    //图像翻转
    m_pngconvert = bool(RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::PNG_CONVERT));
    m_iCropWidth = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::MODEL_WIDTH);
    m_iCropHeight = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::MODEL_HEIGHT);
    m_class_num = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::MODEL_CATEGORY);
    cout <<" m_class_num _____" << m_class_num << endl;
    m_fillerimg = Mat::zeros(m_iCropHeight, m_iCropWidth, CV_8UC3);

    m_tabletRoiEnable = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_TABLET_ROI_ENABLE_" + to_string(boardId()) + "_" + to_string(workflowId()));

    int rectStartIdx = int(ProductSettingFloatMapper::BOX_START_INDEX);
    m_boardRoiRects.clear();
    m_tabletRoiRects.clear();
    for (int boardIdx = 0; boardIdx < m_iBoardCount; boardIdx++)
    {
        int left = RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + (2*boardId()+workflowId())*48 + 4*boardIdx);
        int top = RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + (2*boardId()+workflowId())*48 + 4*boardIdx + 1);
        int right = left + RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + (2*boardId()+workflowId())*48 + 4*boardIdx + 2);
        int bottom = top + RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + (2*boardId()+workflowId())*48 + 4*boardIdx + 3);
        if (top >= bottom || left >= right || top < 0 || left < 0 || bottom >= FRAME_ROWS || right >= FRAME_COLS)
        {
            LogERROR << "board roi is out of range, top: " << top << " ,bottom: " << bottom << " ,left: " << left << " ,right: " << right;
            if (m_is_koutu)
            {
                continue;
            }
            return false;
        }        
        cout << left << " " << top << " " << right << " " << bottom << endl;
        double scale_x = IMAGE_WIDTH/KOUTU_WIDTH;
        double scale_y = IMAGE_HEIGHT/KOUTU_HEIGHT;
        if(m_is_koutu)
        {
            m_boardRoiRects.emplace_back(Rect(Point((left-20)/scale_x, (top-20)/scale_y), Point((right+20)/scale_x, (bottom+20)/scale_y)));
            cout << m_boardRoiRects[boardIdx] << "____" << scale_x << "  " << scale_y << endl;
        }
        else
        {   
            m_boardRoiRects.emplace_back(Rect(Point(left-20, top-20), Point(right+20, bottom+20)));    
            if (m_lastColCount != 0)
            {
                int m_lastColCount_ = workflowId()==0 ? m_lastColCount : m_iColCount - m_lastColCount;
                m_iColCount = boardIdx==0 ? m_iColCount : m_lastColCount_;          //mqd
            }                    
            int tabletHeight = (bottom-top + m_iRowOffset * (m_iRowCount - 1)) / m_iRowCount;
            int tabletWidth = (right-left + m_iColOffset * (m_iColCount - 1)) / m_iColCount;
            for (int r = 0; r < m_iRowCount; r++)
            {
                for (int c = 0; c < m_iColCount; c++)
                {
                    cout << "boardIdx____________________   " << boardIdx << "   m_iRowCount: " << m_iRowCount << "  m_iColCount:__ "<< m_iColCount << endl;
                    if (m_tabletRoiEnable[r * m_iColCount + c] == 0)      //mqd
                    {
                        continue;
                    }
                    int tabletTop = (tabletHeight - m_iRowOffset) * r;
                    int tabletBot = (tabletHeight - m_iRowOffset) * r + tabletHeight;
                    int tabletLeft = (tabletWidth - m_iColOffset) * c;
                    int tabletRight = (tabletWidth - m_iColOffset) * c + tabletWidth;
                    m_tabletRoiRects.emplace_back(Rect(Point(tabletLeft + left, tabletTop + top), Point(tabletRight + left, tabletBot + top)));
                }
            }
        }
    }
        
    String engineName = "/opt/app/model_trt/"+strProdName+"_board1.trt";
    cout<<engineName<<endl;
    if(m_jiaonang)
        loadTensorrtModel(engineName, TensorrtOutputType::CATEGORY_SEGMENT, 40, m_class_num, m_iCropWidth, m_iCropHeight, 3);
    else
        loadTensorrtModel(engineName, TensorrtOutputType::CATEGORY_SEGMENT, 25, m_class_num, m_iCropWidth, m_iCropHeight, 3);
    
    if (m_is_koutu)
    {
        loadTensorrtModel_koutu("/opt/app/model_trt/koutu.trt", 1, 1, 320, 256, 3);
    }
      
    return true;

}


bool BeforeSealingWorkflow::reconfigParameters()
{
    try
    {
        string strProdName = RunningInfo::instance().GetProductionInfo().GetCurrentProd();
        vector<int> vLowerHsv = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_lowerHsv0");
        vector<int> vUpperHsv = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_upperHsv0");
        m_lowerHsv = Scalar(vLowerHsv[0], vLowerHsv[1], vLowerHsv[2]);
        m_upperHsv = Scalar(vUpperHsv[0], vUpperHsv[1], vUpperHsv[2]);

        if (!AppWorkflow::reconfigParameters())
        {
            return false;
        }
    }
    catch(const std::exception& e)
    {
        LogERROR << e.what() << '\n';
        return false;
    }
    
    return true;
}


bool AfterSealingWorkflow::reconfigParameters()
{
    try
    {
        string strProdName = RunningInfo::instance().GetProductionInfo().GetCurrentProd();
        vector<int> vLowerHsv = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_lowerHsv0");
        vector<int> vUpperHsv = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_upperHsv0");
        vector<int> vLowerHsv_1 = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_lowerHsv1");
        vector<int> vUpperHsv_1 = CustomizedJsonConfig::instance().getVector<int>(strProdName + "_upperHsv1");
        m_lowerHsv = Scalar(vLowerHsv[0], vLowerHsv[1], vLowerHsv[2]);
        m_upperHsv = Scalar(vUpperHsv[0], vUpperHsv[1], vUpperHsv[2]);
        m_lowerHsv_1 = Scalar(vLowerHsv_1[0], vLowerHsv_1[1], vLowerHsv_1[2]);
        m_upperHsv_1 = Scalar(vUpperHsv_1[0], vUpperHsv_1[1], vUpperHsv_1[2]);

        cout<<"m_lowerHsv: "<<m_lowerHsv<<", m_upperHsv: "<<m_upperHsv<<endl;
        cout<<"m_lowerHsv_1: "<<m_lowerHsv_1<<", m_upperHsv_1: "<<m_upperHsv_1<<endl;
        if (!AppWorkflow::reconfigParameters())
        {
            return false;
        }

       
        m_lotNumberRoiRects.clear();
        int lotNumCount = RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::LOTNUM_COUNT_PER_BOARD);
        int rectStartIdx = int(ProductSettingFloatMapper::BOX_START_INDEX) + (2*boardId() + workflowId()) * 48 + 4 * 4;

        if (!m_is_koutu)
        {
            for (int boardIdx = 0; boardIdx < getBoardCount()*lotNumCount; boardIdx++)
            {
                if(workflowId()==0 and boardIdx ==1)
                    continue;
                int left = RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + 4*boardIdx);
                int top = RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + 4*boardIdx + 1);
                int right = left + RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + 4*boardIdx + 2);
                int bottom = top + RunningInfo::instance().GetProductSetting().GetFloatSetting(rectStartIdx + 4*boardIdx + 3);
                cout << "top: " << top << " ,bottom: " << bottom << " ,left: " << left << " ,right: " << right << endl;
                if (top >= bottom || left >= right || top < 0 || left < 0 || bottom >= FRAME_ROWS || right >= FRAME_COLS)
                {
                    LogERROR << "board roi is out of range, top: " << top << " ,bottom: " << bottom << " ,left: " << left << " ,right: " << right;
                    return false;
                }
                m_lotNumberRoiRects.emplace_back(Rect(Point(left, top), Point(right, bottom)));
                // cout << left << " " << top << " " << right << " " << bottom << endl;
                if(m_is_pihao)
                    m_tabletRoiRects.emplace_back(Rect(Point(left, top), Point(right, bottom)));
            }
        }
        
        
        cout<<"m_tabletRoiRects : "<<m_tabletRoiRects.size();
    }
    catch(const std::exception& e)
    {
        LogERROR << e.what() << '\n';
        return false;
    }

    return true;
}


bool AppWorkflow::findMaxContour(const Mat &inputImg, vector<vector<Point>> &contours, int &maxAreaIdx, double &maxArea)
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

void AppWorkflow::fileToString(vector<string> &record, const string &line, char delimiter)
{
    int linepos = 0;
    char c;
    int linemax = line.length();
    string curstring;
    record.clear();
    while (linepos < linemax)
    {
        c = line[linepos];
        if (isdigit(c) || c == '.')
        {
            curstring += c;
        }
        else if (c == delimiter && curstring.size())
        {
            record.push_back(curstring);
            curstring = "";
        }
        ++linepos;
    }
    if (curstring.size())
        record.push_back(curstring);
    return;
}

float AppWorkflow::stringToFloat(string str)
{
    int i = 0, len = str.length();
    float sum = 0;
    while (i < len)
    {
        if (str[i] == '.')
            break;
        sum = sum * 10 + str[i] - '0';
        ++i;
    }
    ++i;
    float t = 1, d = 1;
    while (i < len)
    {
        d *= 0.1;
        t = str[i] - '0';
        sum += t * d;
        ++i;
    }
    return sum;
}

bool AppWorkflow::readCSV(string filepath, vector<vector<float>> &a)
{
    vector<float> b;
    vector<string> row;
    string line;
    string filename;
    ifstream in(filepath);
    if (in.fail())
    {
        return false;
    }
    while (getline(in, line) && in.good())
    {
        fileToString(row, line, ',');
        for (int i = 0, leng = row.size(); i < leng; i++)
        {
            b.push_back(stringToFloat(row[i]));
        }
        a.push_back(b);
        b.clear();
    }
    in.close();
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool AppWorkflow::colorSegmentCheckTable(const Mat &srcImage, Mat &resultImg, const vector<vector<float>> &colorTable, const int colorDis, const int grayThreshL, const int grayThreshH)
{
    if (srcImage.data == NULL)
    {
        return false;
    }
    if (colorTable.size() != 255 || colorTable[0].size() != 6)
    {
        return false;
    }

    Mat grayscale;
    cvtColor(srcImage, grayscale, CV_BGR2GRAY);
    Mat processedImg;
    blur(srcImage, processedImg, Size(7, 7));

    float R_B(0), B_G(0);
    Mat flagImg = Mat::zeros(srcImage.rows, srcImage.cols, CV_8UC1);
    for (int i = 0; i < processedImg.rows; i++)
    {
        for (int j = 0; j < processedImg.cols; j++)
        {
            R_B = (float)(1.0 * processedImg.at<Vec3b>(i, j)[2]) / (1.0 * processedImg.at<Vec3b>(i, j)[0]);
            B_G = (float)(1.0 * processedImg.at<Vec3b>(i, j)[0]) / (1.0 * processedImg.at<Vec3b>(i, j)[1]);
            if ((int)grayscale.at<uchar>(i, j) >= grayThreshL && (int)grayscale.at<uchar>(i, j) <= grayThreshH)
            {
                if (R_B >= (colorTable[(int)grayscale.at<uchar>(i, j)][2] - colorDis) && R_B <= (colorTable[(int)grayscale.at<uchar>(i, j)][1] + colorDis) &&
                    B_G >= (colorTable[(int)grayscale.at<uchar>(i, j)][5] - colorDis) && B_G <= (colorTable[(int)grayscale.at<uchar>(i, j)][4] + colorDis))
                {
                    flagImg.at<uchar>(i, j) = 255;
                }
            }
        }
    }
    Mat element = getStructuringElement(MORPH_RECT, Size(15, 15));
    Mat erodeImg;
    erode(flagImg, erodeImg, element);
    dilate(erodeImg, resultImg, element);
    return true;
}

//////////////////////////////////////////////////////////////////////////////
bool AppWorkflow::templateMatch(const Mat &src_img, const Mat &template_img, const int match_scale, const int count, const float score, const int distance, vector<Point> &points)
{
    if (!src_img.data)
	{
		LogERROR << "invalid input image";
		return false;
	}
    if (!template_img.data)
	{
		LogERROR << "invalid template image";
		return false;
	}
    Mat srcResizeImg, tempResizeImg, blurTempImg, blurImg;
    resize(src_img, srcResizeImg, Size(src_img.cols/match_scale, src_img.rows/match_scale));
    resize(template_img, tempResizeImg, Size(template_img.cols/match_scale, template_img.rows/match_scale));

    medianBlur(tempResizeImg, blurTempImg, 3);
    medianBlur(srcResizeImg, blurImg, 3);

    Mat matchdst_img;
    matchdst_img.create(srcResizeImg.dims, srcResizeImg.size, srcResizeImg.type());
    cv::matchTemplate(blurImg, blurTempImg, matchdst_img, TM_CCOEFF_NORMED);
    cv::Point minPoint;
    cv::Point maxPoint;
    double minVal(0);
    double maxVal(0);

    while (points.size() < count || count == -1)
    {
        cv::minMaxLoc(matchdst_img, &minVal, &maxVal, &minPoint, &maxPoint);
        cv::rectangle(matchdst_img, cv::Point(maxPoint.x - distance, maxPoint.y - distance), cv::Point(maxPoint.x + distance, maxPoint.y + distance), 255, -1);
        if (maxVal > score)
        {
            points.emplace_back(Point((maxPoint.x + tempResizeImg.cols/2)*match_scale, (maxPoint.y + tempResizeImg.rows/2)*match_scale));
        }
        else
        {
            break;
        }
    }

    if (points.size() != count && count != -1)
    {
        return false;
    }
    return true;

}


bool AppWorkflow::imageBinaryByHsv(const Mat &srcImage, const int blurKernelSize, const Scalar &lowerHsv, const Scalar &upperHsv, Mat &dstImage)
{
    try
    {    
        Mat blurImg, hsvImg;
        blur(srcImage, blurImg, Size(blurKernelSize, blurKernelSize));
        cvtColor(blurImg, hsvImg, CV_BGR2HSV);
        inRange(hsvImg, lowerHsv, upperHsv, dstImage);
        // cout<<lowerHsv<<endl;
        // cout<<upperHsv<<endl;
        // dilate(dstImage,dstImage,getStructuringElement(MORPH_RECT,Size(7,7)));
        
        if (upperHsv[0]  > 180)
        {
            Mat appendImg;
            inRange(hsvImg, Scalar(0, lowerHsv[1], lowerHsv[2]), Scalar(upperHsv[0]  - 180, upperHsv[1], upperHsv[2]), appendImg);
            bitwise_or(dstImage, appendImg, dstImage);
        }
        // opencv_utils::saveImageInTemp(to_string(boardId())+"dstImage",dstImage);
        // opencv_utils::saveImageInTemp(to_string(boardId())+"srcImage",srcImage);
		// Mat cannyImg, openImg;
		// Canny(blurImg, cannyImg, 70, 150);
		Mat kernel = getStructuringElement(MORPH_RECT, Size(7, 7));//保证是奇数
		morphologyEx(dstImage, dstImage, MORPH_DILATE, kernel);
		// dstImage = (dstImage - openImg) > 0;
    }
    catch(const std::exception& e)
    {
        LogERROR << e.what();
        return false;
    }
    return true;
}


bool AppWorkflow::matchLocatePill(Mat &srcImage, Mat &resultMask, Point &locatePoint)
{
    vector<Point> matchPoints;
    if (!templateMatch(srcImage, m_templateImage, 2, 1, m_iTabletMatchScore, m_templateImage.cols, matchPoints))
    {
        LogERROR << "locating can't find contours";
        return false;
    }
    locatePoint = matchPoints[0];
    circle(resultMask, locatePoint, m_iPillRadius, Scalar(255), -1);
    circle(srcImage, locatePoint, m_iPillRadius, Scalar(0, 255, 0), 5);
    return true;
}

bool AppWorkflow::locatePillByHsv(const Mat &srcImage, const Scalar &lowerHsv, const Scalar &upperHsv, const int minArea, vector<Point> &maxContour, Point &locatePoint)
{
    //----------------------------hsv-------------------------------
    // opencv_utils::saveImageInTemp("srcImage", srcImage);
    Mat binaryImg;
    if (!imageBinaryByHsv(srcImage, 9, lowerHsv, upperHsv, binaryImg))
    {
        LogERROR << "imageBinaryByHsv failed";
        return false;
    }
    // return false;
    vector<vector<Point>> contoursTablet;
    int maxAreaIdxTablet(-1);
    double maxAreaTablet(0.0);
    if (!findMaxContour(binaryImg, contoursTablet, maxAreaIdxTablet, maxAreaTablet))
    {
        LogERROR << "locating can't find contours";
        return false;
    }
    // cout << "tablet area: " << maxAreaTablet << " minArea: " << m_areamin << endl;
    if (maxAreaTablet < m_areamin)
    {
        LogERROR << "locating find the max contour is ";
        return false;
    }

    // maxContour = contoursTablet[maxAreaIdxTablet];
    // drawContours(srcImage, contoursTablet, maxAreaIdxTablet, Scalar(0, 255, 0), -1);    
    Rect box = boundingRect(contoursTablet[maxAreaIdxTablet]);
    int x = (box.tl().x + box.br().x) / 2;
    int y = (box.tl().y + box.br().y) / 2;
    // cout<<"aaaaa: "<<x<<" ---- "<<y<<endl;

    if(m_jiaonang)
    {
        Mat binaryImg_1;
        if (!imageBinaryByHsv(srcImage, 9, m_lowerHsv_1, m_upperHsv_1, binaryImg_1))
        {
            LogERROR << "111111111111111imageBinaryByHsv failed";
            return false;
        }
        vector<vector<Point>> contoursTablet_1;
        int maxAreaIdxTablet_1(-1);
        double maxAreaTablet_1(0.0);
        if (!findMaxContour(binaryImg_1, contoursTablet_1, maxAreaIdxTablet_1, maxAreaTablet_1))
        {
            LogERROR << "11111111111111locating can't find contours";
            return false;
        }
        // if (maxAreaTablet < m_areamin)
        // {
        //     LogERROR << "11111111111111111locating find the max contour is ";
        //     return false;
        // }
        // maxContour_1 = contoursTablet[maxAreaIdxTablet_1];
        // drawContours(srcImage, contoursTablet_1, maxAreaIdxTablet_1, Scalar(255, 0, 0), -1);    
        Rect box_1 = boundingRect(contoursTablet_1[maxAreaIdxTablet_1]);
        
        int x1 = (box_1.tl().x + box_1.br().x) / 2;
        int y1 = (box_1.tl().y + box_1.br().y) / 2;
        x = (x+x1)/2;
        // cout<<"box_tl:"<<box.tl().y<<" , box_br:========= "<<box.br().y<<endl;
        // cout<<"box1_tl:"<<box_1.tl().y<<" , box1_br:========= "<<box_1.br().y<<endl;
        // int y_min = box.tl().y > box_1.tl().y ? box_1.tl().y : box.tl().y;
        // int y_max = box.br().y > box_1.br().y ? box.br().y : box_1.br().y;
   
        // y = (y_min+y_max)/2;
        // cout<<"y_old: "<<(y+y1)/2<<" , new_y: "<<(y_min+y_max)/2<<endl;
        
        y = (y+y1)/2;
    }
    // cout<<"=================================="<<endl;
    locatePoint.x = x;
    locatePoint.y = y;
  
    
    

    if (1 == int(RunningInfo::instance().GetTestProductSetting().getPhase())) //mqd
    {
        // opencv_utils::saveImageInTemp("/processed/qqqqqqqq", binaryImg);
        Mat colorMask;
        vector<Mat> mergeImgs({binaryImg*0.2, binaryImg, binaryImg*0.2});
        merge(mergeImgs, colorMask);
        addWeighted(srcImage, 1, colorMask, 0.3, 0, srcImage);
    }
    // cout<<"=================================="<<endl;
    return true;
}


// bool BeforeSealingWorkflow::computerVisionProcess()
// {
// 	// app need to write its own code to handle computer vision related process properly
// 	// this method only works with framework 5.0.0.0 and up
//     cout << "before sealing workflow computerVisionProcess" << endl;
// 	return true;
// }

// bool AfterSealingWorkflow::computerVisionProcess()
// {
// 	// app need to write its own code to handle computer vision related process properly
// 	// this method only works with framework 5.0.0.0 and up
//     cout << "after sealing workflow computerVisionProcess" << endl;
// 	return true;
// }


bool BeforeSealingWorkflow::locatePill(cv::Mat &srcImage, vector<Point> &maxContour, cv::Point &locatePoint)
{
    // cout << "before sealing workflow" << endl;
    return locatePillByHsv(srcImage, m_lowerHsv, m_upperHsv, 750, maxContour, locatePoint);
}

bool AfterSealingWorkflow::locatePill(cv::Mat &srcImage, vector<Point> &maxContour, cv::Point &locatePoint)
{
    // cout << "after sealing workflow" << endl;
    return locatePillByHsv(srcImage, m_lowerHsv, m_upperHsv, 300, maxContour, locatePoint);

    // return matchLocatePill(srcImage, resultMask, locatePoint);
}


bool AfterSealingWorkflow::checkBoardDefect(cv::Mat &srcImage, cv::Mat &resultMask)
{
    for (int i = 0; i < m_boardRoiRects.size(); i++)
    {
        Mat boardImage = srcImage(m_boardRoiRects[i]);
        resize(boardImage, boardImage, Size(boardImage.cols/4, boardImage.rows/4));
        Scalar lowerHsv(140, 45, 16);
        Scalar upperHsv(200, 255, 180);
        Mat binaryImg;
        if (imageBinaryByHsv(boardImage, 5, lowerHsv, upperHsv, binaryImg))
        {
            // opencv_utils::saveImageInTemp("/processed/binaryImg", binaryImg);
            if (countNonZero(binaryImg) > 300)
            {
                rectangle(m_workflowImage, m_boardRoiRects[i], Scalar(0, 0, 255), 20);
                m_pView->getTarget(i*10)->setResult(4);
                continue; 
            }
        }
    }

    for (int i = 0; i < m_lotNumberRoiRects.size(); i++)
    {
        Rect textRoiRect = m_lotNumberRoiRects[i];
        Mat textRoiImg = srcImage(textRoiRect);

        Mat grayImg, blurImg, cannyImg, dilateImg, dilateInvImg;
        cvtColor(textRoiImg, grayImg, COLOR_RGB2GRAY);
        blur(grayImg, blurImg, Size(5, 5));
        cv::Canny(blurImg, cannyImg, 20, 40, 3);
        Mat element = getStructuringElement(MORPH_RECT, Size(7, 7));
        dilate(cannyImg, dilateImg, element);
        bitwise_not(dilateImg, dilateInvImg);

        // opencv_utils::saveImageInTemp("dilateInvImg", dilateInvImg);
        vector<vector<Point>> contours;
        int maxAreaIdx = 0;
        double maxArea = 0;
        findMaxContour(dilateInvImg, contours, maxAreaIdx, maxArea);
        // cout << "maxArea: " << maxArea << endl;
        if (maxArea > 5000)
        {
            rectangle(m_workflowImage, textRoiRect, Scalar(0, 0, 255), 4);
            cout << "maxArea: " << maxArea << endl;
            cout << "未找到生产日期" << endl;
            // opencv_utils::saveImageInTemp("/processed/ng", dilateInvImg);
            // targetsResult[0] = 2;
            m_pView->getTarget(i/2*10)->setResult(7);
            continue;
        }
        else
        {
            rectangle(m_workflowImage, textRoiRect, Scalar(255, 0, 0), 4);
        }
     }




        // Mat grayImg, blurImg, cannyImg, dilateImg;
        // cvtColor(textRoiImg, grayImg, COLOR_RGB2GRAY);
        // blur(grayImg, blurImg, Size(5, 5));

        // Mat notImg;
        // threshold(blurImg, notImg, 50, 255, THRESH_BINARY_INV);
        // vector<vector<Point>> contours;
        // int maxAreaIdx = 0;
        // double maxArea = 0;

        // findMaxContour(notImg, contours, maxAreaIdx, maxArea); //寻找最大轮廓
        // if (maxArea < 1000)
        // {
        //     rectangle(m_workflowImage, textRoiRect, Scalar(0, 0, 255), 4);
        //     cout << "未找到生产日期" << endl;
        //     targetsResult[0] = 2;

        //     continue;
        // }
        // else
        // {
        //     rectangle(m_workflowImage, textRoiRect, Scalar(255, 0, 0), 4);
        // }

}