#ifndef EURESYS_DALSA_CAMERA_H
#define EURESYS_DALSA_CAMERA_H

#include "euresys_camera.h"
#include "euresys_dalsa_camera_config.h"

#include <string>

class EuresysDalsaCamera : public EuresysCamera
{
public:
	EuresysDalsaCamera(const std::string &cameraName, const std::string &deviceName, const std::shared_ptr<EuresysDalsaCameraConfig> pConfig);
	virtual ~EuresysDalsaCamera() {};

protected:
	virtual bool openVirtualSerialCom();
	virtual bool setCameraConfig();
};

#endif // EURESYS_DALSA_CAMERA_H