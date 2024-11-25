#ifndef MOCK_CAMERA_H
#define MOCK_CAMERA_H

#include "camera.h"
#include "mock_camera_config.h"

#include <memory>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

class MockCommonCamera : public BaseCamera {
public:
	MockCommonCamera(const std::string& cameraName, const std::string &deviceName, const std::shared_ptr<CameraConfig> pConfig);
	virtual ~MockCommonCamera();

	virtual bool start();
	virtual bool open() = 0;
	virtual bool read(cv::Mat &frame) = 0;
	virtual void release();
	virtual bool restart();
protected:
	std::shared_ptr<cv::VideoCapture> m_pCapture;

	void blockRead(cv::Mat &frame);
	void nonBlockRead(cv::Mat &frame);
};

class MockCamera : public MockCommonCamera {
public:
	MockCamera(const std::string& cameraName, const std::string &deviceName, const std::shared_ptr<MockCameraConfig> pConfig);
	virtual ~MockCamera() {};

	virtual bool open();
	virtual bool read(cv::Mat &frame);
};

class MockDirectory : public MockCommonCamera {
public:
	MockDirectory(const std::string& cameraName, const std::string &deviceName, const std::shared_ptr<MockDirectoryConfig> pConfig);
	virtual ~MockDirectory() {};

	virtual bool open();
	virtual bool read(cv::Mat &frame);
private:
	std::vector<std::string> m_fileNames;
	int m_fileIndex;
};

#endif // MOCK_CAMERA_H