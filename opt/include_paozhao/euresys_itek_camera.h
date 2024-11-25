#ifndef EURESYS_ITEK_CAMERA_H
#define EURESYS_ITEK_CAMERA_H

#include "euresys_camera.h"
#include "euresys_itek_camera_config.h"

#include <string>

class EuresysItekCamera : public EuresysCamera
{
public:
	EuresysItekCamera(const std::string &cameraName, const std::string &deviceName, const std::shared_ptr<EuresysItekCameraConfig> pConfig);
	virtual ~EuresysItekCamera() {};

protected:
	virtual bool setCameraConfig();
};

#endif // EURESYS_ITEK_CAMERA_H