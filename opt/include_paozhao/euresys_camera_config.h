#ifndef EURESYS_CAMERA_CONFIG_H
#define EURESYS_CAMERA_CONFIG_H

#include "config.h"

class EuresysCameraConfig : public CameraConfig
{
public:
	EuresysCameraConfig();
	EuresysCameraConfig(const std::string &model, const std::string &cameraFile, const int width, const int height, const int lineRate,
		const int overlap, const int x, const int y, const int exposure = 10000, const int triggerMode = 0);
	EuresysCameraConfig(const int readTimeout, const std::string &model, const std::string &cameraFile, const int width, const int height, const int lineRate,
		const int overlap, const int x, const int y, const int exposure = 10000, const int triggerMode = 0);
	virtual ~EuresysCameraConfig() {}

	ImageDimension &imageDimension() { return m_dimension; }
	void setImageDimension(const int width, const int height, const int lineRate) { m_dimension = ImageDimension(width, height, lineRate); }
	ImageOffset &imageOffset() { return m_offset; }
	void setImageOffset(const int x, const int y) { m_offset = ImageOffset(x, y); }
	int exposure() const { return m_iExposure; }
	void setExposure(const int exposure) { m_iExposure = exposure; }
	int triggerMode() const { return m_iTriggerMode; }
	void setTriggerMode(const int mode) { m_iTriggerMode = mode; }
	int overlap() const { return m_iOverlap; }
	void setOverlap(const int overlap) { m_iOverlap = overlap; }
	std::string cameraFile() const { return m_sCameraFile; }
	void setCameraFile(const std::string &file) { m_sCameraFile = file; }

	void setSerial(const std::shared_ptr<void *> pSerial) { m_pSerial = pSerial; }

	virtual bool setConfig() { return CameraConfig::setConfig(); }

	virtual std::stringstream getConfigParameters();
	virtual void setConfigParameters(const std::stringstream &parameters);
protected:
	std::shared_ptr<void *> m_pSerial;

	virtual bool sendCommand(void* pSerial, const std::vector<unsigned char> &command, const std::string &okResponse);
private:
	int m_iTriggerMode;
	ImageDimension m_dimension;
	ImageOffset m_offset;
	int m_iExposure;
	int m_iOverlap;
	std::string m_sCameraFile;
};

#endif // EURESYS_CAMERA_CONFIG