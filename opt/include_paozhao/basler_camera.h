#ifndef BASLER_CAMERA_H
#define BASLER_CAMERA_H

#include "camera.h"
#include "basler_camera_config.h"

#include <pylon/PylonBase.h>
#include <pylon/PylonIncludes.h>
#include <pylon/InstantCamera.h>
#include <GenApi/INodeMap.h>
#include <opencv2/opencv.hpp>

class BaslerCamera : public BaseCamera
{
 public:
	BaslerCamera(const std::string &cameraName, const std::string &deviceName, const std::shared_ptr<BaslerCameraConfig> pConfig);
	virtual ~BaslerCamera();

	// initialization of the camera
	virtual bool start();
	virtual bool open();
	virtual bool read(cv::Mat &frame);
	virtual void release();
	virtual bool restart();

 private:
	// pay attention on the initialization order,
	// init should be earilier than shared_ptr other wise segmentation fault will occur
	Pylon::PylonAutoInitTerm m_autoInitTerm;
	std::shared_ptr<Pylon::CInstantCamera> m_pCapture;

	bool blockRead(cv::Mat &frame);
	bool nonBlockRead(cv::Mat &frame);

	bool sendSoftwareTriggerCommand();
};

#endif // BASLER_CAMERA_H