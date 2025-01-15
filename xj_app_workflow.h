#ifndef XJ_APP_WORKFLOW_H
#define XJ_APP_WORKFLOW_H

#include "workflow.h"
#include "tensorrt_classifier.h"

#define MAX_BOARD_COUNT 4
#define MAX_TEBLET_COUNT 20
#define IMAGE_WIDTH 2448
#define IMAGE_HEIGHT 2048
#define KOUTU_WIDTH 320.0
#define KOUTU_HEIGHT 256.0
// #define scale_x 2448/320
// #define scale_y 2048/256

enum class ProductSettingFloatMapper : int
{
	SAVE_IMAGE_TYPE = 0, 			// save a image for a given type 0, 1, and 2; 0 means all, 1 means only save defect, and 2 means not save at all
	IMAGE_SAVE_SIZE = 1,
	MODEL_TYPE = 2,
	SENSITIVITY0 = 3, 				// the sensitivity of the model, range (0, 1)
	ALARM_THRESHOLD = 4,     		// the threshold for the alarm to kick defects out, range (0, 1]
	PRESSURE_THRESHOLD = 5,  		// the pressure alarm value, range (0, 1]
    SENSITIVITY1 = 6, 	
    TABLET_RADIUS = 7,
    TABLET_MATCH_SCORE = 8,

    LOT_POSITION = 9,               //增加批号位置，上下左右分别用1/2/3/4表示
    BOARD_COUNT_PER_VIEW = 10,
    TABLET_COUNT_PER_BOARD = 11,
    TABLET_ROI_COL_COUNT = 12,
    TABLET_ROI_ROW_COUNT = 13,
    TABLET_ROI_COL_OFFSET = 14,
    TABLET_ROI_ROW_OFFSET = 15,
    LOTNUM_COUNT_PER_BOARD = 16,
    BANMIANBUQUAN_COLS = 17,


    USE_JIAONANG = 18,
    USE_PIHAO = 19,

    SENSITIVITY = 20,
    MODEL_WIDTH,
    MODEL_HEIGHT,
    MODEL_CATEGORY,

    TEST_TYPE = 24,
    TEST_CHANNEL,

    PNG_CONVERT = 26,
    USE_MODEL,
    

    HSV = 28,    //占用到41, BOARD0 Hl Hu Sl Su Vl Vu ; board1 Hl Hu Sl Su Vl Vu 

    DEFECT_CUCAO = 40,
    DEFECT_HEIDIAN,
    DEFECT_JIAFEN,
    DEFECT_JIETOU,
    DEFECT_KELI,
    DEFECT_LOULVBO,
    DEFECT_LVBOPOSUN,
    DEFECT_POSUN,
    DEFECT_QUEJIAO,
    DEFECT_REFENGBULIANG,
    DEFECT_YIWU,
    DEFECT_JIAONANGDINGMAO = 51,
    PLC_PARAMETERS = 52,
    // X_TEST_OFFSET1 = 17,
    // X_TEST_OFFSET2 = 18,

    // SPACE_SIZE = 26,
    // TEXT_LENGTH1 = 27,
    // TEXT_LENGTH2 = 28,

    // BLACK_DOT_GRAY_LEVEL_DIFF = 20,
    // BLACK_DOT_AREA = 21,
    // MISS_CORNER_GRAY_LEVEL_DIFF = 22,
    // MISS_CORNER_AREA = 23,
    // PVC_GRAY_LEVEL_DIFF = 24,
    // PVC_AREA = 25
    BOX_START_INDEX = 60

};


class AppWorkflow : public BaseWorkflow
{
public:
	AppWorkflow(const int workflowId, const std::shared_ptr<WorkflowConfig> &pConfig);
	virtual ~AppWorkflow() {}

    virtual bool reconfigParameters();
	virtual bool imagePreProcess();
	virtual bool computerVisionProcess();
    int getBoardCount() { return m_iBoardCount; };
    int getTabletCountPerBoard() { return m_iTabletCountPerBoard; };
    int getRowCount() {return m_iRowCount;};
    int getColCount() {return m_iColCount;};
    int getlastColCount() {return m_lastColCount ;};
    bool getpihao() {return m_is_pihao; };
    std::vector<float> getGoodCategoryScores() {return m_score;}

    virtual bool locatePill(cv::Mat &srcImage, std::vector<cv::Point> &maxContour, cv::Point &locatePoint) = 0;
    virtual bool checkBroken(cv::Mat &srcImage, cv::Mat &resultMask) = 0;
    virtual bool checkBlackSpot(cv::Mat &srcImage, cv::Mat &resultMask) = 0;
    virtual bool checkBoardDefect(cv::Mat &srcImage, cv::Mat &resultMask) = 0;

    int mergeSegmentResult(const int originalResult, const int segmentResult);
    virtual bool result_handle(std::vector<cv::Mat> &srcImage,std::vector<int> &class_result,std::vector<int> &result);
    int segMaskResult(const cv::Mat & mask,const int Index,const double area,bool new_model);
    
protected:
	virtual void drawDesignedTargets(const double scale);
    bool matchLocatePill(cv::Mat &srcImage, cv::Mat &resultMask, cv::Point &locatePoint);
    bool locatePillByHsv(const cv::Mat &srcImage, const cv::Scalar &lowerHsv, const cv::Scalar &upperHsv, const int minArea, std::vector<cv::Point> &maxContour, cv::Point &locatePoint);
    void CharacterFilling(const cv::Mat & handleImg,std::vector<cv::Mat> & croppedImgs);
    cv::Mat m_templateImage;

    void loadTemplateImage() { m_templateImage = cv::imread("./models/templateImage.png"); }
    bool templateMatch(const cv::Mat &src_img, const cv::Mat &template_img, const int match_scale, const int count, const float score, const int distance, std::vector<cv::Point> &points);

    bool imageBinaryByHsv(const cv::Mat &srcImage, const int blurKernelSize, const cv::Scalar &lowerHsv, const cv::Scalar &upperHsv, cv::Mat &dstImage);
    bool findMaxContour(const cv::Mat &inputImg, std::vector<std::vector<cv::Point>> &contours, int &maxAreaIdx, double &maxArea);
    bool saveTargetObjs(const std::vector<int> &result, const std::vector<float> &good_score,const std::vector<cv::Mat> &targetImages);
 
    std::vector<int> getBoardTop() { return m_boardTop; }
    std::vector<int> getBoardBot() { return m_boardBot; }
    
    std::vector<cv::Rect> m_boardRoiRects;
    std::vector<cv::Rect> m_tabletRoiRects;
    std::vector<cv::Rect> m_tabletRoiRects_1;
    std::vector<cv::Rect> m_lotNumberRoiRects;

    cv::Scalar m_lowerHsv;
    cv::Scalar m_upperHsv;
    cv::Scalar m_lowerHsv_1;
    cv::Scalar m_upperHsv_1;

    bool m_is_pihao;
    bool m_jiaonang;
    bool m_usemodel = true;
    bool m_pngconvert = false;
    bool m_is_koutu = true;
    
    cv::Mat m_fillerimg;
    int m_lastColCount;
    int m_pihao_position;

private:
	bool extractImageFrom(const cv::Mat &frame, const std::vector<cv::Rect> &targetRoiRects, std::vector<cv::Mat> &croppedImgs, std::vector<int> &targetResults);
    bool tabletBoardProcess(const cv::Mat &frame, const int boardCount, const int left, const int top, std::vector<cv::Mat> &croppedImgs, std::vector<cv::Rect> &targetsRect, std::vector<int> &targetsResult);
	
    bool loadTensorrtModel(const std::string engineName, const TensorrtOutputType tensorrtOutputType, const int batchsize, const int category, const int width, const int height, const int channel);
    bool loadTensorrtModel_koutu(const std::string engineName, const int batchsize, const int category, const int width, const int height, const int channel);
	bool dlWarmUp();
    bool position(const cv::Mat &frame, const std::vector<cv::Rect> &targetRoiRects, std::vector<cv::Mat> &croppedImgs, std::vector<int> &targetResults);
    bool findBox(const cv::Mat img, const int num, std::vector< std::vector<cv::Rect>> &box);
    bool findBox1(const std::vector< std::vector<cv::Point>> contours, const int SetArea, std::vector<cv::Rect> &box);
    bool tabletRoi(std::vector<cv::Rect> &boardRoiRects, std::vector<cv::Rect> &yaoliRects, const int &tabletId, std::vector<cv::Rect> &v_yaoliRoi, std::vector<int> &targetResults);
    bool piHaoRoi(std::vector<cv::Rect> &boardRoiRects, std::vector<cv::Rect> &pihaoRects, const int &tabletId, std::vector<cv::Rect> &v_pihaoRoi, std::vector<int> &targetResults);
    bool boardRoi(std::vector<cv::Rect> &boardRoiRects, std::vector<cv::Rect> &boardRects, std::vector<cv::Rect> &v_boardRoi);

    std::vector<std::vector<float>> m_colorTable;
    void fileToString(std::vector<std::string> &record, const std::string &line, char delimiter);
    float stringToFloat(std::string str);
    bool readCSV(const std::string filepath, std::vector<std::vector<float>> &a);
    bool loadColorTable() { return readCSV("./models/RGBScan.csv", m_colorTable); }
    bool colorSegmentCheckTable(const cv::Mat &srcImage, cv::Mat &resultImg, const std::vector<std::vector<float>> &colorTable, const int colorDis, const int grayThreshL, const int grayThreshH);
   
	void loadGapImage() { m_stopgapImage = cv::imread("./models/stopgapImage.png"); }

	cv::Mat m_stopgapImage;

    std::vector<int> m_boardTop{std::vector<int>(MAX_BOARD_COUNT, 0)};
    std::vector<int> m_boardBot{std::vector<int>(MAX_BOARD_COUNT, 0)};
    std::vector<int> m_boardLeft{std::vector<int>(MAX_BOARD_COUNT, 0)};
    std::vector<int> m_boardRight{std::vector<int>(MAX_BOARD_COUNT, 0)};

    std::vector<int> m_tabletRoiEnable{std::vector<int>(MAX_TEBLET_COUNT, 0)};
 
	std::shared_ptr<TensorrtClassifier> m_tensorrtDL;
	std::shared_ptr<TensorrtClassifier> m_tensorrtDL_seg;
	int m_iModelWidth;
	int m_iRoiWidth; // width of one view
	float m_fSensitivity;
    std::vector<float> m_score;
    

    int m_iBoardCount;
    int m_iTabletCountPerBoard;
    int m_iRowCount;
    int m_iColCount;
    int m_iRowOffset;
    int m_iColOffset;
    std::vector<int> m_segParameters;
    int m_height;
    int m_width;
    int m_areamin;
    int m_class_num;

    int m_iCropWidth;
    int m_iCropHeight;

    int m_iPillRadius;
    float m_iTabletMatchScore;
    std::string m_strProdName;

    std::vector<int> m_segclass;

};

class BeforeSealingWorkflow : public AppWorkflow
{
public:
    BeforeSealingWorkflow(const int workflowId, const std::shared_ptr<WorkflowConfig> &pConfig) : AppWorkflow(workflowId, pConfig) {}
    virtual bool locatePill(cv::Mat &srcImage, std::vector<cv::Point> &maxContour, cv::Point &locatePoint);
    virtual bool checkBroken(cv::Mat &srcImage, cv::Mat &resultMask) { return true; };
    virtual bool checkBlackSpot(cv::Mat &srcImage, cv::Mat &resultMask) { return true; };
    virtual bool checkBoardDefect(cv::Mat &srcImage, cv::Mat &resultMask) { return true; };

    virtual bool reconfigParameters();
private:
    cv::Scalar m_lowerHsv;
    cv::Scalar m_upperHsv;
};
 
class AfterSealingWorkflow : public AppWorkflow
{
public:
    AfterSealingWorkflow(const int workflowId, const std::shared_ptr<WorkflowConfig> &pConfig) : AppWorkflow(workflowId, pConfig) 
    {
        loadTemplateImage();
    }
    virtual bool locatePill(cv::Mat &srcImage, std::vector<cv::Point> &maxContour, cv::Point &locatePoint);
    virtual bool checkBroken(cv::Mat &srcImage, cv::Mat &resultMask) { return true; };
    virtual bool checkBlackSpot(cv::Mat &srcImage, cv::Mat &resultMask) { return true; };
    virtual bool checkBoardDefect(cv::Mat &srcImage, cv::Mat &resultMask);// { return true; };

    virtual bool reconfigParameters();
// private:  
    // cv::Scalar m_lowerHsv;
    // cv::Scalar m_upperHsv;
    // cv::Scalar m_lowerHsv_1;
    // cv::Scalar m_upperHsv_1;


};

#endif // XJ_APP_WORKFLOW_H