#ifndef DETECTOR_H
#define DETECTOR_H

#include "config.h"
#include "workflow.h"
#include "thread_pool.h"
#include "io_manager.h"
#include "tracker.h"
#include "logger.h"
#include "board.h"
#include "timer_utils.hpp"



#include <condition_variable>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <opencv2/opencv.hpp>

//
// Detector majorly handle the board in product line
// Detector will take predefined board data, construct workflows, classify, track, draw the board,
// and finally using plc if result is bad
//

class SaveImageHandler
{
public:
	enum class SaveImageType : int
	{
		SAVE_ALL_IMAGE = 0,
		SAVE_DEFECT_IMAGE = 1,
		SAVE_NO_IMAGE = 2
	};

	SaveImageHandler();
	virtual ~SaveImageHandler() {};

	void reset();

	SaveImageType saveImageType() const { return m_iSaveImageType; }
	void saveImageType(const SaveImageType type) { m_iSaveImageType = type; }

	int saveImageSize() const { return m_iSaveImageSize; }
	void saveImageSize(const int size) { m_iSaveImageSize = size; }

	std::string goodDir() const { return m_sGoodDir; }
	void goodDir(const std::string &goodDir);

	std::string badDir() const { return m_sBadDir; }
	void badDir(const std::string &badDir);

	virtual bool decideToSave(const cv::Mat &img, const ClassificationResult result, const std::string &imgName);
private:
	// save image config, which will be loaded in app
	SaveImageType m_iSaveImageType;
	int m_iSaveImageSize;

	// timer in minutes, every 60 minutes reset, this is the one we used to save number of 
	// good images if the save image size from UI is greater than 0
	timer_utils::Timer<std::chrono::minutes> m_saveImageTimer;
	int m_iSaveGoodImagesCount;

	std::string m_sGoodDir;
	std::string m_sBadDir;

	std::string trimDirString(const std::string &dirString);
};

class ErrorHandler
{
public:
	enum class ErrorCode : int
	{
		OK = 0,
		GET_NEXT_FRAME_ERROR = 1001,
		GET_NO_FRAMES = 1002,
		GET_PARTIAL_FRAMES = 1003,
		IMAGE_PREPROCESS_ERROR = 1004,
		EXTRACT_IMAGE_ERROR = 1005,
		CLASSIFY_ERROR = 1006,
		MODEL_RUNNING_ERROR = 1007,
		CLEANUP_BOARD_ERROR = 1008,
		PURGE_RESULT_ERROR = 1009,
		DRAW_BOARD_ERROR = 1010,
		READ_IMAGE_FILE_ERROR = 1011,
		GET_NEXT_FRAME_AND_SAVE_ERROR = 1012
	};

	ErrorHandler();
	virtual ~ErrorHandler() {};

	void setErrorMessage(const ErrorCode code, const std::string &message = "");
	void reset();

	virtual int errorCode() const;
	virtual const char *errorMessage();
private:
	ErrorCode m_iErrorCode;
	std::string m_sErrorMessage;
};

class BaseDetector
{
  public:
	BaseDetector(const std::shared_ptr<DetectorConfig> pConfig, const std::shared_ptr<BaseTracker> pTracker, const std::shared_ptr<Board> pBoard);
	virtual ~BaseDetector();

	int productCount() const { return m_iProductCount; }

	size_t totalWorkflows() const { return m_workflows.size(); }
	bool getWorkflowImage(const int workflowId, cv::Mat &frame, bool processed = false);
	bool restartWorkflowCamera(const int workflowId);

	bool hasIncompletedNextFrame() { return m_bIncompletedNextFrame; }
	bool isExpectedExceptions(const std::vector<std::string> &expected);
	bool isExpectedExceptions(const std::vector<std::string> &expected, std::vector<int> &workflowIds);

	virtual bool createBoardTrackingHistory(const int plcDistance) = 0;


	// set io manager
	void setIoManager(const std::shared_ptr<BaseIoManager> pIoManager) { m_pIoManager = pIoManager; }
	std::shared_ptr<BaseIoManager> ioManager() { return m_pIoManager; }

	// save image related
	int saveImageType() const { return (int)m_pSaveImageHandler->saveImageType(); }
	void saveImageType(const int type) { m_pSaveImageHandler->saveImageType((SaveImageHandler::SaveImageType)type); }

	int saveImageSize() const { return m_pSaveImageHandler->saveImageSize(); }
	void saveImageSize(const int size) { m_pSaveImageHandler->saveImageSize(size); }

	std::string saveGoodImageDir() const { return m_pSaveImageHandler->goodDir(); }
	void saveGoodImageDir(const std::string &goodDir) { m_pSaveImageHandler->goodDir(goodDir); }

	std::string saveBadImageDir() const { return m_pSaveImageHandler->badDir(); }
	void saveBadImageDir(const std::string &badDir) { m_pSaveImageHandler->badDir(badDir); }

	void resetSaveImageHandler() { m_pSaveImageHandler->reset(); }

	// read next frame
	virtual bool getNextFrame();
	// pre-process read frame
	virtual bool imagePreProcess();
	// classify the frame, include both traditional computer vision process and deep learning process, result will be distributed
	virtual bool classifyBoard();
	// clean board will check classified result, and make decision if the object need to be purged
	virtual bool cleanupBoard(const bool needCalculateResult = true);
	// draw frame of the board
	virtual bool drawBoard(const double scale = 1.0, const int thickness = 3);
	// error processing, default is log error message
	virtual void errorProcess();

	// the following methods are used in test environment, read or save image from current workflow, which should be set previously
	virtual bool readFrameFromFile(const std::string &fileName);
	virtual bool saveFrameToFile(const std::string &filePath, const std::string &fileName);
	bool saveImageInMultiThread();
  protected:
	std::shared_ptr<DetectorConfig> m_pConfig;
	std::shared_ptr<BaseTracker> m_pTracker;

	std::shared_ptr<BaseIoManager> m_pIoManager;

	std::shared_ptr<Board> m_pBoard;
	std::shared_ptr<BaseTrackingHistory> m_pTrackingHistory;

	std::vector<std::shared_ptr<BaseWorkflow>> m_workflows;
	std::shared_ptr<ThreadPool> m_pWorkerThread;

	// interface to error handler
	std::shared_ptr<ErrorHandler> m_pErrorHandler;

	// This one need to define its own tracking objects, and add to tracker, leave it to app for details
	virtual bool addToTrackingHistory() = 0;

	// we add the following methods to allow multiple classifier(model) in one detector
	// override this method if you use multiple classifier, and want to input different data to classifiers
	virtual std::map<int, std::pair<std::vector<std::vector<cv::Mat>>, std::vector<std::vector<ClassificationResult>>>> makeClassifierData();
	// override this method if you use multiple classifier, and want to distribute your multiple results to the board/view/target
	virtual bool updateBoardResults(const std::map<int, std::vector<std::vector<ClassificationResult>>> &results);

	// purge board result allow to purge one result or multiple results
	virtual bool purgeBoardResult(const ClassificationResult result);
	virtual bool purgeBoardResult(const std::vector<std::vector<ClassificationResult>> &result, const bool needCalculateResult = true);

	// interface of the same method in SaveImageHandlerr
	virtual bool decideToSave(const cv::Mat &img, const ClassificationResult result, const std::string &imgName)
	{
		return m_pSaveImageHandler->decideToSave(img, result, imgName);
	}

	// add extra method here, in case app need to attach more in save image name
	virtual bool customizedSavedImageName(const std::string &name, const int viewIdx, const int targetIdx, std::string &returned) { return false; }

	// workflow is based on views in board
	template <typename T>
	void initWorkflows()
	{
		m_workflows.clear();

		if (m_pConfig == nullptr || m_pBoard == nullptr)
		{
			LogERROR << "Detector configuration or Board does not defined";
			return;
		}

		if (m_pConfig->totalWorkflows() != (int)m_pBoard->viewsSize())
		{
			LogERROR << "Number of workflows does not match with views in Board";
			return;
		}

		for (int idx = 0; idx < m_pConfig->totalWorkflows(); idx++)
		{
			m_workflows.emplace_back(std::make_shared<T>(idx, m_pConfig->getWorkflowConfig(idx)));
			m_workflows[idx]->setView(m_pBoard->getView(idx));
			m_workflows[idx]->setBoardId(m_pBoard->boardId());
		}
		LogINFO << "Initial workflows successfully";

		m_pWorkerThread = std::make_shared<ThreadPool>(m_pConfig->totalWorkflows());
		m_pWorkerThread->init();
	}

  private:
	int m_iProductCount;
	bool m_bIncompletedNextFrame;

	// interface to SaveImageHandler
	std::shared_ptr<SaveImageHandler> m_pSaveImageHandler;

	void errorProcess(const ErrorHandler::ErrorCode code, const std::string &message = "");
	// bool saveImageInMultiThread();

	bool toGetNextFrame(const std::shared_ptr<BaseWorkflow> pWorkflow, const bool retry = false);
	bool toPreProcessImage(const std::shared_ptr<BaseWorkflow> pWorkflow);
	bool toComputerVisionProcessImage(const std::shared_ptr<BaseWorkflow> pWorkflow);
#if !defined(NO_CLASSIFY) && defined(__linux__)
	bool toClassifyImage(std::map<int, std::pair<std::vector<std::vector<cv::Mat>>, std::vector<std::vector<ClassificationResult>>>> &data);
#endif

	bool getNextFrameInSingleThread();
	bool getNextFrameInMultiThread();
	bool imagePreProcessInSingleThread();
	bool imagePreProcessInMultiThread();

	bool toSaveImage(const int prodCount, const std::vector<std::vector<cv::Mat>> &imageBatch,
	                 std::vector<std::vector<ClassificationResult>> &results);
};

#endif // DETECTOR_H
