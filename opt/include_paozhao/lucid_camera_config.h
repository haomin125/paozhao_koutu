#ifndef LUCID_CAMERA_CONFIG
#define LUCID_CAMERA_CONFIG

#include "config.h"

#include "ArenaApi.h"
#include <opencv2/opencv.hpp>
#include <vector>

class LucidCameraConfig : public CameraConfig
{
public:
	LucidCameraConfig();
	LucidCameraConfig(const std::string &model, const int width, const int height, const double fps, const int x, const int y, const double exposure = 10000,
	                  const GenICam::gcstring &triggerMode = "Off", const GenICam::gcstring &pixelFormat = "RGB8", const int packetSize = 8192,
	                  const int interpacketDelay = 4000, const float debounceWidth = 5, const float gamma = -1, const float balanceRatioRed = -1,
	                  const float balanceRatioBlue = -1, const float balanceRatioGreen = -1);
	LucidCameraConfig(const int readTimeout, const std::string &model, const int width, const int height, const double fps, const int x, const int y,
	                  const double exposure = 10000, const GenICam::gcstring &triggerMode = "Off", const GenICam::gcstring &pixelFormat = "RGB8",
	                  const int packetSize = 8192, const int interpacketDelay = 4000, const float debounceWidth = 5, const float gamma = -1,
	                  const float balanceRatioRed = -1, const float balanceRatioBlue = -1, const float balanceRatioGreen = -1);
	virtual ~LucidCameraConfig() {}

	ImageDimension &imageDimension() { return m_dimension; }
	void setImageDimension(const int width, const int height, const int fps) { m_dimension = ImageDimension(width, height, fps); }
	ImageOffset &imageOffset() { return m_offset; }
	void setImageOffset(const int x, const int y) { m_offset = ImageOffset(x, y); }
	double exposure() const { return m_dExposure; }
	void setExposure(const double exposure) { m_dExposure = exposure; }
	GenICam::gcstring &triggerMode() { return m_sTriggerMode; }
	void setTriggerMode(const GenICam::gcstring &mode) { m_sTriggerMode = mode; }
	GenICam::gcstring &pixelFormat() { return m_sPixelFormat; }
	void setpixelFormat(const GenICam::gcstring &pixelFormat) {m_sPixelFormat = pixelFormat;}
	int packetSize() const { return m_iPacketSize; }
	void setPacketSize(const int packetSize) { m_iPacketSize = packetSize; }
	int interpacketDelay() const { return m_iInterpacketDelay; }
	void setInterpacketDelay(const int delay) { m_iInterpacketDelay = delay; }
	float debounceWidth() const { return m_fDebounceWidth; }
	void setDebounceWidth(const float width) { m_fDebounceWidth = width; }
	size_t streamBufferSize() const { return m_iStreamBufferSize; }
	void setStreamBufferSize(const size_t size) { m_iStreamBufferSize = size; }

	float gamma() const { return m_fGamma; }
	void setGamma(const float gamma) { m_fGamma = gamma; };
	float balanceRatioRed() const { return m_fBalanceRatioRed; }
	void setBalanceRatioRed(const float red) { m_fBalanceRatioRed = red; }
	float balanceRatioBlue() const { return m_fBalanceRatioBlue; }
	void setBalanceRatioBlue(const float blue) { m_fBalanceRatioBlue = blue; };
	float balanceRatioGreen() const { return m_fBalanceRatioGreen; }
	void setBalanceRatioGreen(const float green) { m_fBalanceRatioGreen = green; };

	// timeout in milliseconds when restart looking for devices updated, default is 500 milliseconds
	int updateDevicesTimeout() const { return m_iUpdateDevicesTimeout; }
	void setUpdateDevicesTimeout(const int timeout) { m_iUpdateDevicesTimeout = timeout; }

	// heartbeat timeout in microseconds when IsConnected() check if camera is online
	// if 3 times timeout occurs, for example if this timeout is 500000 microseconds
	// IsConnected() will turn false after 1500000 microseconds
	double deviceHeartbeatTimeout() const { return m_dDeviceHeartbeatTimeout; }
	void setDeviceHeartbeatTimeout(const double timeout) { m_dDeviceHeartbeatTimeout = timeout; }

	void setNodemap(GENAPI_NAMESPACE::INodeMap *nodemap) { m_pNodeMap = nodemap; }

	// add extra pixel formats which will use CV_8UC1 to convert read frame, by default is Mono8 and BayerRG8
	void addToSingleChannelConversion(const std::string &pixelFormat);
	bool isSingleChannelConversion();

	virtual bool setConfig();

	virtual std::stringstream getConfigParameters();
	virtual void setConfigParameters(const std::stringstream &parameters);
protected:
	GENAPI_NAMESPACE::INodeMap *m_pNodeMap;

private:
	ImageDimension m_dimension;
	ImageOffset m_offset;
	double m_dExposure;
	GenICam::gcstring m_sTriggerMode;
	int m_iPacketSize;
	int m_iInterpacketDelay;
	float m_fDebounceWidth;
	GenICam::gcstring m_sPixelFormat;
	size_t m_iStreamBufferSize;

	float m_fGamma;
	float m_fBalanceRatioRed;
	float m_fBalanceRatioBlue;
	float m_fBalanceRatioGreen;

	// restart() update device timeout in milliseconds, default is 30000 milliseconds
	int m_iUpdateDevicesTimeout;

	// device link heartbeat timeout in microseconds, default is -1, until you need to use disconnect/restart
	double m_dDeviceHeartbeatTimeout;

	std::vector<std::string> m_singleChannelConversion;
};

#endif // LUCID_CAMERA_CONFIG