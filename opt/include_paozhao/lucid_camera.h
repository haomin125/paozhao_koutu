#ifndef LUCID_CAMERA_H
#define LUCID_CAMERA_H

#include "camera.h"
#include "lucid_camera_config.h"

#include <opencv2/opencv.hpp>
#include <vector>

class LucidCamera : public BaseCamera
{
 public:
	LucidCamera(const std::string &cameraName, const std::string &deviceName, const std::shared_ptr<LucidCameraConfig> pConfig);
	virtual ~LucidCamera();

	// initialization of the camera
	virtual bool start();
	virtual bool open();
	virtual bool read(cv::Mat &frame);
	virtual void release();
	virtual bool restart();

	// The following methods are opened for qpp to wait until the leader for the next image has arrived, timeout is 64 bit interger 
	// in milliseconds
	bool waitForNextImage(const unsigned long timeout);
	bool resetWaitForNextImage();

 private:
	Arena::IDevice *m_pCapture;
	Arena::ISystem *m_pSystem;

	bool blockRead(cv::Mat &frame);
	bool nonBlockRead(cv::Mat &frame);

	int locateDevice(std::vector<Arena::DeviceInfo> &infos);
};

class LucidCameraSystem {
public:
	virtual ~LucidCameraSystem();

	static LucidCameraSystem &instance();
	static void releaseInstance();

	Arena::ISystem *getISystem() { return m_pSystem; };
protected:
	static LucidCameraSystem* m_systemInstance;

private:
	LucidCameraSystem();

	static std::mutex m_systemMutex;
	Arena::ISystem *m_pSystem;
};

#endif // LUCID_CAMERA_H