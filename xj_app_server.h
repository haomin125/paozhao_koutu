#ifndef XJ_APP_SERVER_H
#define XJ_APP_SERVER_H

#include "camera_manager.h"
#include "product_line.h"
#include "io_manager.h"
#include "xj_app_detector.h"
#include "basler_camera_config.h"

#include <atomic>
#include <string>
#include <mutex>

class XJAppServer
{
public:
	XJAppServer(const std::shared_ptr<CameraManager> &pCameraManager, const int plcDistance);
	virtual ~XJAppServer() {};

	struct DetectorRunStatus
	{
		enum class Status : int
		{
			NOT_RUN = 0,
			PRODUCT_RUN = 1,
			TEST_RUN = 2,
			TEST_RUN_WITH_CAMERA = 3
		};

		DetectorRunStatus() {}

		Status getStatus();
	};

	enum class TestProcessPhase : int
	{
		NO_PROCESS = 0,
		PRE_PROCESS_ONLY = 1,
		PRE_PROCESS_AND_CLASSIFY = 2
	};

	int totalDetectors() { return (int)m_pDetectors.size(); }

	bool initXJApp(std::shared_ptr<BaseIoManager> &pPlcManager);
#ifdef __linux__
	bool initCameraManager(const std::string &cameraType, const std::string &filePath = "/opt/");
#elif _WIN64
	bool initCameraManager(const std::string &cameraType, const std::string &filePath = "C:\\opt\\");
#endif

	void startDetector(const int boardId);
	static void stopDetectors();
private:
	static std::atomic_bool m_bIsStop;

	static std::mutex m_productNameMutex;
	std::string m_sProductName;
	std::string m_cameraType;
	std::vector<double> jishi;
	
	
	std::shared_ptr<CameraManager> m_pCameraManager;
	std::shared_ptr<ProductLine> m_pProductLine;
	std::shared_ptr<BaslerCameraConfig> pConfig[2];

	struct ServerDetector
	{
		bool m_bIsFirstStart;
		std::shared_ptr<AppDetector> m_pDetector;

		ServerDetector(const std::shared_ptr<AppDetector> &pDetector) : m_bIsFirstStart(true), m_pDetector(pDetector) {}
	};
	std::vector<ServerDetector> m_pDetectors;

	// cameras per board
	std::map<int, std::vector<std::shared_ptr<BaseCamera>>> m_cameras;

	int m_iPlcDistance;

	bool initProductLine();
	bool initTargetMarginInBoard(const int boardId);
	bool initBoardParameters(const int boardId);

	void initProductCameras();
	void initMockCameras(const std::string &filePath);

	bool camerasAreReady(const int boardId);
	bool updateDatabaseReport(const int boardId);

	bool runDetector(const int boardId);
	bool parametersTest(const int boardId);
	bool saveCameraTriggeredTestImages(const int boardId);

	bool isProductSwitched();

	// init cameras
	template <typename T, typename G>
	void initCameras(const std::map<int, std::pair<std::string, std::string>> &cameras, const std::shared_ptr<G> pConfig[])
	{
		std::shared_ptr<ProductLineConfig> pProductLineConfig = m_pProductLine->getConfig();

		for (const auto &itr : cameras)
		{
			std::shared_ptr<T> pCamera = std::make_shared<T>(itr.second.first, itr.second.second, pConfig[itr.first]);
			m_pCameraManager->addCamera(pCamera);
			m_cameras[pProductLineConfig->getBoard(itr.first)].emplace_back(pCamera);
		}
	}
};

#endif // XJ_APP_SERVER_H