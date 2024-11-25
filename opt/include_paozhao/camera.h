#ifndef CAMERA_H
#define CAMERA_H

#include "config.h"
#include "classifier_result.h"

#include <string>
#include <mutex>
#include <opencv2/opencv.hpp>

#define CAMERA_RESTART_SLEEP_INTERVAL 30

class BaseCamera {
public:
	BaseCamera(const std::string& cameraName, const std::string &deviceName, const std::shared_ptr<CameraConfig> pConfig);
	virtual ~BaseCamera() {};

	bool cameraStarted();
	bool cameraOpened();

	std::shared_ptr<CameraConfig> config() { return m_pConfig; }

	std::string &deviceName() { return m_sDeviceName; }
	std::string &cameraName() { return m_sCameraName; }
	std::string exceptionDescription() { return m_pException->getDescription(); }

	unsigned long totalFrames() const { return m_iTotalFrames; }
	void setTotalFrames(const unsigned long fcount) { m_iTotalFrames = fcount; }

	// current image result type indicate which type of image to save
	void setCurrentProcessedFrameAndResult(const cv::Mat &frame, const ClassificationResult result);
	void getCurrentProcessedFrameAndResult(cv::Mat &frame, ClassificationResult &result, const bool checkCameraStataus = true);

	virtual bool start() = 0;
	virtual bool open() = 0;
	virtual bool read(cv::Mat &frame) = 0;
	virtual void release() = 0;
	virtual bool restart() = 0;
protected:
	unsigned long m_iTotalFrames;
	std::shared_ptr<CameraConfig> m_pConfig;
	static std::mutex m_mutex;

	class CameraException
	{
	public:
		CameraException() : m_sDescription("") {};
		virtual ~CameraException() {}

		void reset() { m_sDescription = ""; }
		std::string &getDescription() { return m_sDescription; }
		void setDescription(const std::string &error) { m_sDescription = error; }
		void setDescription(const int errorCode);
	private:
		std::string m_sDescription;
	};
	std::shared_ptr<CameraException> m_pException;

	void setCameraStarted(const bool flag);
	void setCameraOpened(const bool flag);
private:
	bool m_bStarted;
	bool m_bOpened;
	std::string m_sCameraName; // Camera Name in Xinjian Project
	std::string m_sDeviceName; // Camera Product Id
	std::pair<cv::Mat, ClassificationResult> m_currentProcessedFrameResult;
};


#endif  // CAMERA_H