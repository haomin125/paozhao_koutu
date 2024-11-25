#ifndef BASLER_CAMERA_CONFIG_H
#define BASLER_CAMERA_CONFIG_H

#include "config.h"

#include <pylon/PylonBase.h>
#include <pylon/PylonIncludes.h>
#include <GenApi/INodeMap.h>
#include <opencv2/opencv.hpp>


class BaslerCameraConfig : public CameraConfig
{
public:
	BaslerCameraConfig();
	BaslerCameraConfig(const std::string &model, const int width, const int height,
		const int fps, const int x, const int y, const int exposure = 10000, const GenICam::gcstring &triggerMode = "Off",
		const int packetSize = 8192, const int interpacketDelay = 20000, const int priority = 50, const float timeAbs = 10.0,
		const GenICam::gcstring &pixelFormat = "BayerRG8", const bool gammaEnable = false, const float gamma = -1);
	BaslerCameraConfig(const int readTimeout, const std::string &model, const int width, const int height,
		const int fps, const int x, const int y, const int exposure = 10000, const GenICam::gcstring &triggerMode = "Off",
		const int packetSize = 8192, const int interpacketDelay = 20000, const int priority = 50, const float timeAbs = 10.0,
		const GenICam::gcstring &pixelFormat = "BayerRG8", const bool gammaEnable = false, const float gamma = -1);
	virtual ~BaslerCameraConfig() {}

	// allow app to set its own camera device class, which should be "BaslerGigE", "BaslerUsb", and etc. default is "BaslerGigE"
	std::string &deviceClass() { return m_sDeviceClass; }
	void setDeviceClass(const std::string &device) { m_sDeviceClass = device; }

	ImageDimension &imageDimension() { return m_dimension; }
	void setImageDimension(const int width, const int height, const int fps) { m_dimension = ImageDimension(width, height, fps); }
	ImageOffset &imageOffset() { return m_offset; }
	void setImageOffset(const int x, const int y) { m_offset = ImageOffset(x, y); }
	int exposure() const { return m_iExposure; }
	void setExposure(const int exposure) { m_iExposure = exposure; }
	GenICam::gcstring &triggerMode() { return m_sTriggerMode; }
	void setTriggerMode(const GenICam::gcstring &mode) { m_sTriggerMode = mode; }

	int packetSize() const { return m_iPacketSize; }
	void setPacketSize(const int packetSize) { m_iPacketSize = packetSize; }
	int interpacketDelay() const { return m_iInterpacketDelay; }
	void setInterpacketDelay(const int delay) { m_iInterpacketDelay = delay; }
	int receiveThreadPriority() const { return m_iReceiveThreadPriority; }
	void setReceiveThreadPriority(const int priority) { m_iReceiveThreadPriority = priority; }
	float lineDebouncerTimeAbs() const { return m_fLineDebouncerTimeAbs; }
	void setLineDebouncerTimeAbs(const float timeAbs) { m_fLineDebouncerTimeAbs = timeAbs; }

	GenICam::gcstring &pixelFormat() { return m_sPixelFormat; }
	void setPixelFormat(const GenICam::gcstring &format) { m_sPixelFormat = format; }

	// Here we explain how to use gammaEnable and gamma value:
	// if gammaEnable is true, but gamma = -1, gamma is enable, gamma selector set to sRGB, camera use default 0.4 as gamma value
	// if gamma > 0, gamma is enable, gamma selector set to User, gamma value set to the value what user defined here
	bool isGammaEnable() const { return m_bGammaEnable; }
	void setGammaEnable(const bool gammaEnable) { m_bGammaEnable = gammaEnable; }
	float gamma() const { return m_fGamma; }
	void setGamma(const float gamma) { m_fGamma = gamma; };

	bool softTriggerEnable() const { return m_bSoftTriggerEnable; }
	void setSoftTriggerEnable(const bool softTrigger) { m_bSoftTriggerEnable = softTrigger; }

	int softTriggerWaitTime() const { return m_iSoftTriggerWaitInMilliSeconds; }
	void setSoftTriggerWaitTime(const int timeInMilliSeconds) { m_iSoftTriggerWaitInMilliSeconds = timeInMilliSeconds; }

	void setNodemap(GENAPI_NAMESPACE::INodeMap &nodemap) { m_pNodeMap = &nodemap; }

	virtual bool setConfig();

	virtual std::stringstream getConfigParameters();
	virtual void setConfigParameters(const std::stringstream &parameters);
protected:
	GENAPI_NAMESPACE::INodeMap *m_pNodeMap;

private:
	std::string m_sDeviceClass;
	ImageDimension m_dimension;
	ImageOffset m_offset;
	int m_iExposure;
	// "Off": TriggerMode_Off, "On": TriggerMode_On, default is "Off"
	GenICam::gcstring m_sTriggerMode;
	int m_iPacketSize;
	int m_iInterpacketDelay;
	int m_iReceiveThreadPriority;
	float m_fLineDebouncerTimeAbs;
	GenICam::gcstring m_sPixelFormat;

	bool m_bGammaEnable;
	float m_fGamma;

	// soft trigger related, default soft trigger enable is false, wait time is 5 ms
	bool m_bSoftTriggerEnable;
	int m_iSoftTriggerWaitInMilliSeconds;
};

#endif // BASLER_CAMERA_CONFIG