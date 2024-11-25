#ifndef EURESYS_CAMERA_H
#define EURESYS_CAMERA_H

#include "camera.h"
#include "euresys_camera_config.h"

#include <string>
#include <opencv2/opencv.hpp>
#include <multicam.h>

class EuresysCamera : public BaseCamera
{
public:
	EuresysCamera(const std::string &cameraName, const std::string &deviceName, const std::shared_ptr<EuresysCameraConfig> pConfig);
	virtual ~EuresysCamera();

	// initialization of the camera
	virtual bool start();
	virtual bool open();
	virtual bool read(cv::Mat &frame);
	virtual void release();
	virtual bool restart();

protected:
	std::shared_ptr<MCHANDLE> m_pChannel;
	std::shared_ptr<void *> m_pSerial;

	virtual bool openVirtualSerialCom();
	virtual bool setCameraConfig() = 0;
private:
	std::shared_ptr<cv::Mat> m_pOverlapFrame;
	std::string m_sPixelFormat;

	bool blockRead(cv::Mat &frame);
	bool nonBlockRead(cv::Mat &frame);
};

#endif // EURESYS_CAMERA_H