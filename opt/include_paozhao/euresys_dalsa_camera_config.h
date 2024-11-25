#ifndef EURESYS_DALSA_CAMERA_CONFIG_H
#define EURESYS_DALSA_CAMERA_CONFIG_H

#include "euresys_camera_config.h"

class EuresysDalsaCameraConfig : public EuresysCameraConfig
{
public:
	EuresysDalsaCameraConfig();
	EuresysDalsaCameraConfig(const std::string &model, const std::string &cameraFile, const int width, const int height, const int lineRate,
		                     const int overlap, const int x, const int y, const int exposure = 10000, const int triggerMode = 0);
	EuresysDalsaCameraConfig(const int readTimeout, const std::string &model, const std::string &cameraFile, const int width, const int height, const int lineRate,
	                         const int overlap, const int x, const int y, const int exposure = 10000, const int triggerMode = 0);
	virtual ~EuresysDalsaCameraConfig() {}

	virtual bool setConfig();
	virtual bool setAsciiMode();
};

#endif // EURESYS_DALSA_CAMERA_CONFIG