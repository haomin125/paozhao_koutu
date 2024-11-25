#ifndef EURESYS_ITEK_CAMERA_CONFIG_H
#define EURESYS_ITEK_CAMERA_CONFIG_H

#include "euresys_camera_config.h"

class EuresysItekCameraConfig : public EuresysCameraConfig
{
public:
	EuresysItekCameraConfig();
	EuresysItekCameraConfig(const std::string &model, const std::string &cameraFile, const int width, const int height, const int lineRate,
	                        const int overlap, const int x, const int y, const int exposure = 10000, const int triggerMode = 0);
	EuresysItekCameraConfig(const int readTimeout, const std::string &model, const std::string &cameraFile, const int width, const int height, const int lineRate,
	                        const int overlap, const int x, const int y, const int exposure = 10000, const int triggerMode = 0);
	virtual ~EuresysItekCameraConfig() {}

	virtual bool setConfig();
};

#endif // EURESYS_ITEK_CAMERA_CONFIG