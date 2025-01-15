#include "xj_app_server.h"
#include "xj_app_tracker.h"

#include "basler_camera.h"
#include "basler_camera_config.h"

#include "mock_camera.h"
#include "mock_camera_config.h"
#include "running_status.h"
#include "shared_utils.h"
#include "timer_utils.hpp"
#include "customized_json_config.h"

// #ifdef __linux__
// #include "xj_app_classifier.h"
// #endif
#include <numeric>
#include <chrono>
#include <thread>

#define DATABASE_ELAPSE_MINUTE 60     // the minutes to update report in database

using namespace std;
using namespace cv;

atomic_bool XJAppServer::m_bIsStop(false);
mutex XJAppServer::m_productNameMutex;

XJAppServer::XJAppServer(const std::shared_ptr<CameraManager> &pCameraManager, const int plcDistance) : m_pCameraManager(pCameraManager),
                                                                                                        m_pProductLine(nullptr),
                                                                                                        m_iPlcDistance(plcDistance),
                                                                                                        m_sProductName(RunningInfo::instance().GetProductionInfo().GetCurrentProd())
{
	// use_model = bool(RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::USE_MODEL));
	m_pDetectors.clear();
	m_cameras.clear();
}

XJAppServer::DetectorRunStatus::Status XJAppServer::DetectorRunStatus::getStatus()
{
	int runStatus = RunningInfo::instance().GetProductionInfo().GetRunStatus();
	int testStatus = RunningInfo::instance().GetTestProductionInfo().GetTestStatus();
	int triggerCameraImageStatus = RunningInfo::instance().GetTestProductionInfo().GetTriggerCameraImageStatus();

	if (runStatus == 1)
	{
		return XJAppServer::DetectorRunStatus::Status::PRODUCT_RUN;
	}

	if (testStatus == 1)
	{
		return XJAppServer::DetectorRunStatus::Status::TEST_RUN;
	}

	if (triggerCameraImageStatus == 1)
	{
		return XJAppServer::DetectorRunStatus::Status::TEST_RUN_WITH_CAMERA;
	}

	return XJAppServer::DetectorRunStatus::Status::NOT_RUN;
}

bool XJAppServer::initXJApp(std::shared_ptr<BaseIoManager> &pPlcManager)
{
	// initial json configuration
#ifdef __linux__
	ClassifierResult appDefects("lvsuPz", "/opt/config/lvsuPz_config.json");
#else
	ClassifierResult appDefects("lvsuPz", "C:\\opt\\config\\lvsuPz_config.json");
#endif

	// initial product line
	if (!initProductLine() || m_pProductLine == nullptr)
	{
		LogERROR << "Initial product line failed";
		return false;
	}

	// initial tracker, all detector will shared the same tracker here
	shared_ptr<AppTracker> pTracker = make_shared<AppTracker>((const int)m_pProductLine->boardsSize());

	// initial board with multiple views, each view has 1 target in product line
	// if you have multiple targets here. please see me for details -- Luo Bin
	m_pDetectors.clear();
	for (int boardIdx = 0; boardIdx < (int)m_pProductLine->boardsSize(); boardIdx++)
	{
		shared_ptr<Board> pBoard = m_pProductLine->getBoard(boardIdx);
		shared_ptr<ProductLineConfig> pProdlineConfig = m_pProductLine->getConfig();

		// set initial margin in each target of the view
		if (!initTargetMarginInBoard(boardIdx))
		{
			LogERROR << "Initial target margin in board[" << boardIdx << "] failed";
			return false;
		}

		// initial detector config, the workflow number should be pulled out from product line configuration
		shared_ptr<AppDetectorConfig> pDetectorConfig = make_shared<AppDetectorConfig>(pProdlineConfig->viewsPerBoard(boardIdx));

		// initial detector
		shared_ptr<AppDetector> pDetector(nullptr);
		
#ifdef BEFORE_SEALING
		pDetector = make_shared<BeforeSealingDetector>(pDetectorConfig, pTracker, pBoard);
#elif AFTER_SEALING
		pDetector = make_shared<AfterSealingDetector>(pDetectorConfig, pTracker, pBoard);
#else
		if (boardIdx == 0)
			pDetector = make_shared<BeforeSealingDetector>(pDetectorConfig, pTracker, pBoard);
		else if (boardIdx == 1)
			pDetector = make_shared<AfterSealingDetector>(pDetectorConfig, pTracker, pBoard);
		else
			LogERROR << "board id is invalid when initial detector";
#endif

		if (!pDetector->createBoardTrackingHistory(m_iPlcDistance))
		{
			LogERROR << "Create board tracking history failed";
			return false;
		}

// #ifdef __linux__
// 		// initial classifier
// 		shared_ptr<BaseClassifier> pClassifier(nullptr);
// 	#ifdef BEFORE_SEALING
// 		pClassifier = make_shared<BeforeSealingClassifier>((const int)ProductSettingFloatMapper::SENSITIVITY);
// 	#elif AFTER_SEALING
// 		pClassifier = make_shared<AfterSealingClassifier>((const int)ProductSettingFloatMapper::SENSITIVITY);
// 	#else
// 		if (boardIdx == 0)
// 			pClassifier = make_shared<BeforeSealingClassifier>((const int)ProductSettingFloatMapper::SENSITIVITY);
// 		else if (boardIdx == 1)
// 			pClassifier = make_shared<AfterSealingClassifier>((const int)ProductSettingFloatMapper::SENSITIVITY);
// 		else
// 			LogERROR << "board id is invalid when initial classifier";
// 	#endif

// 		LogDEBUG << "Views: " << pProdlineConfig->viewsPerBoard(boardIdx);
// 		LogDEBUG << "Target: " << pProdlineConfig->targetsInViewPerBoard(boardIdx);
// 		int batchSize = pProdlineConfig->viewsPerBoard(boardIdx) * pProdlineConfig->targetsInViewPerBoard(boardIdx);
// 		if (!batchSize)
// 		{
// 			LogERROR << "Invalid batch size required by classifier";
// 			return false;
// 		}
// 		if (!pClassifier->initClassifier(batchSize))
// 		{
// 			LogERROR << "Initial classifier failed";
// 			return false;
// 		}
// 		pDetector->setClassifier(pClassifier);
// #endif

		pDetector->setIoManager(pPlcManager);
		m_pDetectors.emplace_back(ServerDetector(pDetector));
	}

	// set the running data status count to the last board ID
	RunningInfo::instance().GetRunningData().useBoardIdKey(false);

	// log default product name which has be initialed in constructor
	LogDEBUG << "Default Product: " << m_sProductName;

	return true;
}

bool XJAppServer::initProductLine()
{
	#if defined(BEFORE_SEALING) || defined(AFTER_SEALING)
		map<int, vector<int>> boardToView = {{0, {0, 1}}};
		map<int, int> viewToBoard = {{0, 0}, {1, 0}};
		std::vector<int> targetsInView = { 30, 30};
	#else
		map<int, vector<int>> boardToView = {{0, {0, 1}}, {1, {2, 3}}};
		map<int, int> viewToBoard = {{0, 0}, {1, 0}, {2, 1}, {3, 1}};
		std::cout<<"----------------------------------------------"<<std::endl;
		std::vector<int> targetsInView = { 20, 20, 20, 20};
	#endif

	shared_ptr<AppProductLineConfig> pProdlineConfig = make_shared<AppProductLineConfig>(boardToView, viewToBoard, targetsInView);

	if (!pProdlineConfig->isValid())
	{
		LogERROR << "Invalid product line configuration";
		return false;
	}

	// save camera to board mapping in running data for trace back board id from camera index
	RunningInfo::instance().GetRunningData().setCameraToBoard(viewToBoard);

	pProdlineConfig->setConfig();
	m_pProductLine = make_shared<ProductLine>(pProdlineConfig);

	return true;
}

bool XJAppServer::initTargetMarginInBoard(const int boardId)
{
	shared_ptr<Board> pBoard = m_pProductLine->getBoard(boardId);
	shared_ptr<ProductLineConfig> pProdlineConfig = m_pProductLine->getConfig();

	if (pBoard == nullptr || pProdlineConfig == nullptr)
	{
		LogERROR << "Invalid board or product line config";
		return false;
	}

	for (int viewIdx = 0; viewIdx < (int)pBoard->viewsSize(); viewIdx++)
	{
		shared_ptr<View> pView = pBoard->getView(viewIdx);
		for(int targetIdx = 0; targetIdx < pProdlineConfig->targetsInViewPerBoard(boardId); targetIdx++)
		{
			shared_ptr<TargetObject> pTarget = pView->getTarget(targetIdx);
			vector<unsigned int> margins = dynamic_pointer_cast<AppProductLineConfig>(pProdlineConfig)->boundBoxConfig(boardId, viewIdx);
			pTarget->setBoundMargin(margins[0], margins[1], margins[2], margins[3]);
		}
	}

	return true;
}

void XJAppServer::initProductCameras()
{
    vector<string> cameraSNs = CustomizedJsonConfig::instance().getVector<string>("CAMERA_SNs");
#if defined(BEFORE_SEALING) || defined(AFTER_SEALING)
	map<int, pair<string, string>> cameraNames = {
		{0, {"CAM0", cameraSNs[0]}},
		{1, {"CAM1", cameraSNs[1]}}
	};
	// shared_ptr<BaslerCameraConfig> pConfig[2];
	pConfig[0] = make_shared<BaslerCameraConfig>(8000, "lvsuPz.pb", 2448, 2048, 20, 0, 0, 350, "On");
	pConfig[1] = make_shared<BaslerCameraConfig>(8000, "lvsuPz.pb", 2448, 2048, 20, 0, 0, 350, "On");
	for (size_t i = 0; i < 2; i++)
	{
		pConfig[i]->setGammaEnable(true);
		pConfig[i]->setGamma(0.4);
	}
	
#else
	map<int, pair<string, string>> cameraNames = {
		{0, {"CAM0", cameraSNs[0]}},
		{1, {"CAM1", cameraSNs[1]}},
		{2, {"CAM2", cameraSNs[2]}},
		{3, {"CAM3", cameraSNs[3]}}
	};
	shared_ptr<BaslerCameraConfig> pConfig[4];
	pConfig[0] = make_shared<BaslerCameraConfig>(8000, "lvsuPz.pb", 2448, 2048, 20, 0, 0, 500, "On");
	pConfig[1] = make_shared<BaslerCameraConfig>(8000, "lvsuPz.pb", 2448, 2048, 20, 0, 0, 500, "On");
	pConfig[2] = make_shared<BaslerCameraConfig>(8000, "lvsuPz.pb", 2448, 2048, 20, 0, 0, 500, "On");
	pConfig[3] = make_shared<BaslerCameraConfig>(8000, "lvsuPz.pb", 2448, 2048, 20, 0, 0, 500, "On");
#endif
	m_cameraType="basler";
	// init product cameras
	initCameras<BaslerCamera, BaslerCameraConfig>(cameraNames, pConfig);
}

void XJAppServer::initMockCameras(const string &filePath)
{
#if defined(BEFORE_SEALING) || defined(AFTER_SEALING)
	map<int, pair<string, string>> cameraNames = {
		{0, {"CAM0", "Mock Camera 1"}},
		{1, {"CAM1", "Mock Camera 2"}}
	};

	shared_ptr<MockCameraConfig> pConfig[2];
	// pConfig[0] = make_shared<MockCameraConfig>("lvsuPz.pb", filePath + "/0.avi");
	// pConfig[1] = make_shared<MockCameraConfig>("lvsuPz.pb", filePath + "/1.avi");
	pConfig[0] = make_shared<MockCameraConfig>("lvsuPz.pb", "/opt/videos/0.avi");
	pConfig[1] = make_shared<MockCameraConfig>("lvsuPz.pb", "/opt/videos/1.avi");
	// pConfig[0] = make_shared<MockCameraConfig>("lvsuPz.pb", "/opt/0.avi");
	// pConfig[1] = make_shared<MockCameraConfig>("lvsuPz.pb", "/opt/1.avi");
	// shared_ptr<MockDirectoryConfig> pConfig[2];
	// pConfig[0] = make_shared<MockDirectoryConfig>("lvsuPz.pb", "/opt/pics0", ".png");
	// pConfig[1] = make_shared<MockDirectoryConfig>("lvsuPz.pb", "/opt/pics1", ".png");
	
#else
	map<int, pair<string, string>> cameraNames = {
		{0, {"CAM0", "Mock Camera 1"}},
		{1, {"CAM1", "Mock Camera 2"}},
		{2, {"CAM2", "Mock Camera 3"}},
		{3, {"CAM3", "Mock Camera 4"}}
	};

	shared_ptr<MockCameraConfig> pConfig[4];
	// pConfig[0] = make_shared<MockCameraConfig>("lvsuPz.pb", filePath + "/BeforeLeft.avi");
	// pConfig[1] = make_shared<MockCameraConfig>("lvsuPz.pb", filePath + "/BeforeRight.avi");
	// pConfig[2] = make_shared<MockCameraConfig>("lvsuPz.pb", filePath + "/AfterLeft.avi");
	// pConfig[3] = make_shared<MockCameraConfig>("lvsuPz.pb", filePath + "/AfterRight.avi");
	pConfig[0] = make_shared<MockCameraConfig>("lvsuPz.pb", "/opt/history/0.png");
	pConfig[1] = make_shared<MockCameraConfig>("lvsuPz.pb", "/opt/history/1.png");
	pConfig[2] = make_shared<MockCameraConfig>("lvsuPz.pb", "/opt/history/2.png");
	pConfig[3] = make_shared<MockCameraConfig>("lvsuPz.pb", "/opt/history/3.png");
#endif
	// init mock cameras
	m_cameraType="mock";
	initCameras<MockCamera, MockCameraConfig>(cameraNames, pConfig);
	// initCameras<MockDirectory, MockDirectoryConfig>(cameraNames, pConfig);
}

bool XJAppServer::initCameraManager(const string &cameraType, const string &filePath)
{
	if (cameraType == "basler")
	{
		LogDEBUG << "Adding Basler cameras";
		initProductCameras();
	}
	else if (cameraType == "mock")
	{
		LogDEBUG << "Adding Mock cameras";
		initMockCameras(filePath);
	}

	// set cameras in product line
	for (const auto &itr : m_cameras)
	{
		m_pProductLine->setBoardCamera(itr.first, itr.second);
	}

	return true;
}

bool XJAppServer::camerasAreReady(const int boardId)
{
	vector<string> startedCameras;

	startedCameras.clear();
	for (const auto &itr : m_cameras[boardId])
	{
		if (itr->cameraStarted())
		{
			startedCameras.emplace_back(itr->cameraName());
		}
	}

	return !startedCameras.empty();
}

bool XJAppServer::updateDatabaseReport(const int boardId)
{
	string product = RunningInfo::instance().GetProductionInfo().GetCurrentProd();
	string lot = RunningInfo::instance().GetProductionInfo().GetCurrentLot();

	return RunningInfo::instance().GetRunningData().saveToDatabase(product, boardId, lot);
}

bool XJAppServer::initBoardParameters(const int boardId)
{
	m_pDetectors[boardId].m_pDetector->reconfigWorkflow();
	// reset save image type, size, and save image handler
	m_pDetectors[boardId].m_pDetector->saveImageType((const int)RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::SAVE_IMAGE_TYPE));
	m_pDetectors[boardId].m_pDetector->saveImageSize((const int)RunningInfo::instance().GetProductSetting().GetFloatSetting((const int)ProductSettingFloatMapper::IMAGE_SAVE_SIZE));
	m_pDetectors[boardId].m_pDetector->resetSaveImageHandler();
	m_pDetectors[boardId].m_pDetector->saveGoodImageDir("/opt/history/good/");
	m_pDetectors[boardId].m_pDetector->saveBadImageDir("/opt/history/bad/");

	/*         相机参数设置                     */
	////////////////////////////////////////////

	if(m_cameraType!="mock")	
	{
		vector<double> gamma = CustomizedJsonConfig::instance().getVector<double>(m_sProductName +"_"+ "gamma");
		vector<int> expose = CustomizedJsonConfig::instance().getVector<int>(m_sProductName +"_"+ "expose");
		vector<int> packetDelay = CustomizedJsonConfig::instance().getVector<int>("packetDelay");
		m_pCameraManager->stopCameras();
		for(int i=0;i<2;i++)
		{
			pConfig[i]->setInterpacketDelay(packetDelay[i]);
			pConfig[i]->setExposure(expose[i]);
			pConfig[i]->setGammaEnable(true);
			pConfig[i]->setGamma(gamma[i]);
			pConfig[i]->setConfig();
		}
		
		m_pCameraManager->startCameras();
	}
	return true;


}

bool XJAppServer::isProductSwitched()
{
	unique_lock<mutex> lock(m_productNameMutex);
	string currentProductName = RunningInfo::instance().GetProductionInfo().GetCurrentProd();
	if (currentProductName != m_sProductName)
	{
		m_sProductName = currentProductName;
		return true;
	}
	return false;
}

bool XJAppServer::runDetector(const int boardId)
{
	if (boardId < 0 || boardId >= m_pDetectors.size())
	{
		LogERROR << "Invalid board Id: " << boardId;
		return false;
	}
	// bool use_model = CustomizedJsonConfig::instance().get<bool>(m_sProductName+"_usemodel");
	
	// cout<<"===================="<<use_model<<"==================="<<endl;
timer_utils::Timer<chrono::microseconds> detectTimer;
	//////////////////////////// STEP1: get next image ////////////////////////////
	// product count will be increased if get next frame sucessfully, to make log//
	// consistant, we add 1 here instead                                         //
	LogDEBUG << "Board[" << boardId << "] get next frame started. \t Product count: " << m_pDetectors[boardId].m_pDetector->productCount() + 1;
	if (!m_pDetectors[boardId].m_pDetector->getNextFrame())
	{
		LogERROR << "Board[" << boardId << "] get next image failed";
		return false;
	}
	
detectTimer.Reset();
	//////////////////////////// STEP2: pre-process next image ////////////////////////////
	LogDEBUG << "Board[" << boardId << "] process image started. \t Product count: " << m_pDetectors[boardId].m_pDetector->productCount();
	if (!m_pDetectors[boardId].m_pDetector->imagePreProcess())
	{
		LogERROR << "Board[" << boardId << "] process next image failed";
		return false;
	}




	m_pDetectors[boardId].m_pDetector->saveImageInMultiThread();
	// cout << "time: " << "Board[" << boardId << "]   " << detectTimer.Elapsed().count() << endl;

	//////////////////////////// STEP4: purge and cleanup next image ////////////////////////////
	LogDEBUG << "Board[" << boardId << "] cleanup started. \t Product count: " << m_pDetectors[boardId].m_pDetector->productCount();
	if (!m_pDetectors[boardId].m_pDetector->cleanupBoard())
	{
		LogERROR << "Board[" << boardId << "] cleanp board and PLC remove failed";
		// return false;
	}

	//////////////////////////// STEP5: draw next image ////////////////////////////
	LogDEBUG << "Board[" << boardId << "] drawing started. \t Product count: " << m_pDetectors[boardId].m_pDetector->productCount();
	double scale = 0.5;
	if (!m_pDetectors[boardId].m_pDetector->drawBoard(scale))
	{
		LogERROR << "Board[" << boardId << "] draw board failed";
		return false;
	}
	cout << "time: " << "Board[" << boardId << "]   " << detectTimer.Elapsed().count() << endl;
	jishi.emplace_back(detectTimer.Elapsed().count());
	if (jishi.size() == 1000)
	{
		int i = 0;
		for(double num : jishi)
		{
			i++;
			cout << "Board[" << boardId << "]   "<< "第" << i << "次时间_" << num << endl;
		}
		double maxValue = *max_element(jishi.begin(), jishi.end());
		double sum = accumulate(jishi.begin(), jishi.end(), 0);
		double average = static_cast<double>(sum) / jishi.size();
		cout << "Maximum value: " << maxValue << endl;
		cout << "Average value: " << average << endl;
		LogDEBUG << "time_Maximum value: " << maxValue;
		LogDEBUG << "time_Average value: " << average;
	}
	
	return true;
}

bool XJAppServer::parametersTest(const int boardId)
{
	if (boardId < 0 || boardId >= m_pDetectors.size())
	{
		LogERROR << "Invalid board Id: " << boardId;
		return false;
	}

	string productName = RunningInfo::instance().GetTestProductionInfo().GetCurrentProd();
	if (productName == "N/A")
	{
		LogERROR << "Test product is unavailable";
		return false;
	}

	string imageFileName = RunningInfo::instance().GetTestProductSetting().getImageName();
	if (imageFileName == "N/A")
	{
		LogERROR << "Invalid processed image name: " << imageFileName;
		return false;
	}

	// int phase = RunningInfo::instance().GetTestProductSetting().getPhase();
	LogINFO << "Parameter test under " << productName << " start...";

	//////////////////////////// STEP1: read image from file ////////////////////////////
	LogDEBUG << "Board[" << boardId << "] read next frame from " << imageFileName << " started.";
	if (!m_pDetectors[boardId].m_pDetector->readFrameFromFile(imageFileName))
	{
		LogERROR << "Board[" << boardId << "] read next image from " << imageFileName << " failed";
		return false;
	}

	// if (phase != (int)TestProcessPhase::NO_PROCESS)
	{
		//////////////////////////// STEP2: pre-process next image /////////////////////////
		LogDEBUG << "Board[" << boardId << "] process image started.";
		if (!m_pDetectors[boardId].m_pDetector->imagePreProcess())
		{
			LogERROR << "Board[" << boardId << "] process next image failed";
			return false;
		}
	}

	// if (phase == (int)TestProcessPhase::PRE_PROCESS_AND_CLASSIFY)
	{
		//////////////////////////// STEP3: computer vision and classify next image ////////////////////////////
		LogDEBUG << "Board[" << boardId << "] classify started.";
		if (!m_pDetectors[boardId].m_pDetector->classifyBoard())
		{
			LogERROR << "Board[" << boardId << "] classify board failed";
			return false;
		}
	}

	//////////////////////////// STEP4: draw next image ///////////////////////////////
	LogDEBUG << "Board[" << boardId << "] drawing started.";
	double scale = 0.5;
	if (!m_pDetectors[boardId].m_pDetector->drawBoard(scale))
	{
		LogERROR << "Board[" << boardId << "] draw board failed";
		return false;
	}

	LogINFO << "Parameter test under " << productName << " end.";
	return true;
}

bool XJAppServer::saveCameraTriggeredTestImages(const int boardId)
{
	if (boardId < 0 || boardId >= m_pDetectors.size())
	{
		LogERROR << "Invalid board Id: " << boardId;
		return false;
	}

	string productName = RunningInfo::instance().GetTestProductionInfo().GetCurrentProd();
	if (productName == "N/A")
	{
		LogERROR << "Test product is unavailable";
		return false;
	}

	LogINFO << "Save camera triggered images under " << productName << " start...";

	// replace your own code to build your file path, and file name here if needed.
	// you can get camera name from RunningInfo::instance().GetTestProductSetting().getCameraName()
	// if it is available
#ifdef __linux__
	string filePath("/opt/products");
#else
	string filePath("C:\\opt\\products");
#endif
	string fileName = "TestImage";
	m_pDetectors[boardId].m_pDetector->saveFrameToFile(filePath, fileName);

	LogINFO << "Save camera triggered images under " << productName << " end.";
	return true;
}

void XJAppServer::startDetector(const int boardId)
{
	bool areCamerasReady = false;
	timer_utils::Timer<chrono::minutes> reportTimer;
	DetectorRunStatus runStatus;

	try
	{
		reportTimer.Reset();
		while (true)
		{
			if (m_bIsStop)
			{
				LogINFO << "Board[" << boardId << "] exit...";
				break;
			}

			//////////////////////////// PRESTEP0: get current running status ////////////////////////////
			//  get current Run/Test/TriggerCameraImage status
			DetectorRunStatus::Status currentRunStatus = runStatus.getStatus();

			//////////////////////////// PRESTEP1: running status check ////////////////////////////
			//  if system is not running which is 0, sleep and continue
			if (currentRunStatus == DetectorRunStatus::Status::NOT_RUN)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
				if (isProductSwitched())
				{
					LogDEBUG << "Board[" << boardId << "] current product: " << m_sProductName;
					// add your product reconfiguration code here
				}
				// set the first time start flag to true, wait for next time to initialize paramters
				m_pDetectors[boardId].m_bIsFirstStart = true;
				continue;
			}

			//////////////////////////// PRESTEP2: camera status check ////////////////////////////
			// if system is running, or trigger camera image status is true,
			// check if camera is ready, then we go to get next images
			if (currentRunStatus == DetectorRunStatus::Status::PRODUCT_RUN || currentRunStatus == DetectorRunStatus::Status::TEST_RUN_WITH_CAMERA)
			{
				if (!areCamerasReady)
				{
					if (!camerasAreReady(boardId))
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(5));
						continue;
					}
					areCamerasReady = true;
				}
			}

			//////////////////////////// PRESTEP3: restarted status check ////////////////////////////
			// if the system is restarted then at the first time we will reinitialize parameters
			if (m_pDetectors[boardId].m_bIsFirstStart)
			{
				if (!initBoardParameters(boardId))
				{
					LogERROR << "Board[" << boardId << "] initial parameters error, try again. ";
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
					continue;
				}
				m_pDetectors[boardId].m_bIsFirstStart = false; // only initalize once when first time to start
			}

			//////////////////////////// PRESTEP4: update database report ////////////////////////////
			//  update database report using report_add, it will update record with time, product, board...
			if (reportTimer.Elapsed().count() >= DATABASE_ELAPSE_MINUTE)
			{
				reportTimer.Reset();
				if (!updateDatabaseReport(boardId))
				{
					LogERROR << "Board[" << boardId << "] update report in database failed";
					continue;
				}
			}

			switch (currentRunStatus)
			{
				case DetectorRunStatus::Status::PRODUCT_RUN:
					runDetector(boardId);
					break;
				case DetectorRunStatus::Status::TEST_RUN:
					// if (RunningInfo::instance().GetTestProductSetting().getBoardId() == boardId)	//mqd
					{
						parametersTest(boardId);
						RunningInfo::instance().GetTestProductionInfo().SetTestStatus(0);
					}
					break;
				case DetectorRunStatus::Status::TEST_RUN_WITH_CAMERA:
					if (RunningInfo::instance().GetTestProductSetting().getBoardId() == boardId)
					{
						saveCameraTriggeredTestImages(boardId);
					}
					break;
				default:
					LogERROR << "Detector has invalid running status";
					break;
			}
		}
	}
	catch (...)
	{
		LogCRITICAL << "Critical error happens, detector stop abnormally...";
		RunningInfo::instance().GetRunningData().setCustomerDataByName("critical", "detector-stop-abnormally");
	}

	// update database before detector exits
	if (!updateDatabaseReport(boardId))
	{
		LogERROR << "Board[" << boardId << "] update report in database failed";
	}

	return;
}

void XJAppServer::stopDetectors()
{
	// TODO to set stop flag
	LogINFO << "Detectors ask to stop...";
	m_bIsStop = true;
}