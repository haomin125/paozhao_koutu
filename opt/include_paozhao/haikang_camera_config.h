#ifndef HAIKANG_CAMERA_CONFIG_H
#define HAIKANG_CAMERA_CONFIG_H
#include "config.h"
#include "MvCameraControl.h"
#include <opencv2/opencv.hpp>

class HaikangCameraConfig: public CameraConfig
{
public:
	HaikangCameraConfig();
	HaikangCameraConfig(const std::string &model,
	                    const int width,
	                    const int height,
	                    const float fps,
	                    const int x,
	                    const int y,
	                    const int pixelFormat,
	                    const float exposure = 2000,
	                    const float gain = -1,
	                    const int balance_ratio_red = -1,
	                    const int balance_ratio_blue = -1,
	                    const int balance_ratio_green = -1,
	                    const int scpd = 8000,
	                    const int lineDebouncerTime = 50,
	                    const int triggerMode = 0,
	                    const float gamma = -1,
	                    const unsigned int triggerActivation = 0,
	                    const unsigned int packetSize = 8164);
	HaikangCameraConfig(const int readTimeout,
	                    const std::string &model,
	                    const int width,
	                    const int height,
	                    const float fps,
	                    const int x,
	                    const int y,
	                    const int pixelFormat,
	                    const float exposure = 2000,
	                    const float gain = -1,
	                    const int balance_ratio_red = -1,
	                    const int balance_ratio_blue = -1,
	                    const int balance_ratio_green = -1,
	                    const int scpd = 8000,
	                    const int lineDebouncerTime = 50,
	                    const int triggerMode = 0,
	                    const float gamma = -1,
	                    const unsigned int triggerActivation = 0,
	                    const unsigned int packetSize = 8164);
	virtual ~HaikangCameraConfig(){}

	ImageDimension &imageDimension(){return m_dimension;}
	void setImageDimension(const int width, const int height, const float fps)
	{
		m_dimension = ImageDimension(width, height, (const int64_t)fps);
	}
	ImageOffset &imageOffset(){return m_offset;}
	void setImageOffset(const int x, const int y){m_offset = ImageOffset(x, y);}

	float exposure() const {return m_fExposure;}
	void setExposure(const float exposure){m_fExposure = exposure;}

	int exposureMode() const {return m_iExposureMode;}
	void setExposureMode(const int mode){m_iExposureMode = mode;}

	int pixelFormat() const {return m_iPixelFormat;}
	void setPixelFormat(const int pixelFormat){m_iPixelFormat = pixelFormat;}

	int triggerMode() const {return m_iTriggerMode;}
	void setTriggerMode(const int mode){m_iTriggerMode = mode;}

	bool isGainEnable() const {return m_bGainEnable;}
	void setGainEnable(const bool gainEnable){m_bGainEnable = gainEnable;}

	float gain() const {return m_fGain;}
	void setGain(const float gain);

	int gainMode() const {return m_iGainMode;}
	void setGainMode(const int gainMode){m_iGainMode = gainMode;}

	bool isBalanceAutoEnable() const {return m_bBalanceAutoEnable;}
	void setBalanceAutoEnable(const bool balanceAutoEnable){m_bBalanceAutoEnable = balanceAutoEnable;}

	int balanceAuto() const {return m_iBalanceAuto;}
	void setBalanceAuto(const int balance) {m_iBalanceAuto = balance;}

	int balanceRatioRed() const {return m_iBalanceRatioRed;}
	void setBalanceRatioRed(const int red);

	int balanceRatioBlue() const {return m_iBalanceRatioBlue;}
	void setBalanceRatioBlue(const int blue);

	int balanceRatioGreen() const {return m_iBalanceRatioGreen;}
	void setBalanceRatioGreen(const int green);

	int scpd() const {return m_iSCPD;}
	void setScpd(const int scpd){m_iSCPD = scpd;}

	int lineDebouncerTime() const {return m_iLineDebouncerTime;}
	void setLineDebouncerTime(const int lineDebouncerTime){m_iLineDebouncerTime = lineDebouncerTime;}

	bool isGammaEnable() const {return m_bGammaEnable;}
	void setGammaEnable(const bool gammaEnable){m_bGammaEnable = gammaEnable;}

	float gamma() const {return m_fGamma;}
	void setGamma(const float gamma){m_fGamma = gamma;}

	MV_CC_GAMMA_TYPE bayerGammaType() const {return m_bayerGammaType;}
	void setBayerGammaType(const MV_CC_GAMMA_TYPE gammaType){m_bayerGammaType = gammaType;}

	unsigned int triggerActivation() const {return m_iTriggerActivation;}
	void setTriggerActivation(const unsigned int activation){m_iTriggerActivation = activation;}

	unsigned int packetSize() const {return m_iPacketSize;}
	void setPacketSize(const unsigned int packetSize){m_iPacketSize = packetSize;}

	bool softTriggerEnable() const { return m_bSoftTriggerEnable; }
	void setSoftTriggerEnable(const bool softTrigger) { m_bSoftTriggerEnable = softTrigger; }

	int softTriggerWaitTime() const { return m_iSoftTriggerWaitInMilliSeconds; }
	void setSoftTriggerWaitTime(const int timeInMilliSeconds) { m_iSoftTriggerWaitInMilliSeconds = timeInMilliSeconds; }

	unsigned int softTriggerImageNodeNumber() const { return m_iSoftTriggerImageNodeNumber; }
	void setSoftTriggerImageNodeNumber(const unsigned int imageNodeNumber) { m_iSoftTriggerImageNodeNumber = imageNodeNumber; }

	bool setConfig(void *handle)
	{
		m_pHandle = handle;
		return setConfig();
	}

	virtual bool setConfig();

	virtual std::stringstream getConfigParameters();
	virtual void setConfigParameters(const std::stringstream &stream);
protected:
	void *m_pHandle;

private:
	ImageDimension m_dimension;
	ImageOffset m_offset;
	float m_fExposure;
	int m_iExposureMode;
	// 0: MV_TRIGGER_MODE_OFF, 1: MV_TRIGGER_MODE_ON, default is 0
	int m_iTriggerMode;
	int m_iPixelFormat;
	int m_iGainMode;
	float m_fGain;
	int m_iBalanceAuto;
	int m_iBalanceRatioRed;
	int m_iBalanceRatioGreen;
	int m_iBalanceRatioBlue;
	int m_iSCPD;
	int m_iLineDebouncerTime;
	// if gamma is not set, gamma selector set to sRGB, camera use default 0.4 as gamma value
	// if gamma is set, gamma selector set to User, gamma value set to the value what user defined here
	bool m_bGammaEnable;
	float m_fGamma;
	// bayer gamma type is MV_CC_GAMMA_TYPE_NONE by default, you can choose MV_CC_GAMMA_TYPE_VALUE to set bayer
	// gamma value from 0.1 to 4.0, MV_CC_GAMMA_TYPE_LRGB2SRGB to set linear RGB to sRGB, MV_CC_GAMMA_TYPE_SRGB2LRGB 
	// to set sRGB to linear RGB, at this moment, we do not support type MV_CC_GAMMA_TYPE_USER_CURVE
	MV_CC_GAMMA_TYPE m_bayerGammaType;
	// 0: RisingEdge, 1:FallingEdge, 2: LevelHigh, 3: LevelLow, default is 0
	unsigned int m_iTriggerActivation;
	// add those 2 flags in case some cameras not allow to set up those 2 parameters
	bool m_bGainEnable;
	bool m_bBalanceAutoEnable;

	unsigned int m_iPacketSize;

	// soft trigger related, default soft trigger enable is false, wait time is 5 ms, image node number is 1
	bool m_bSoftTriggerEnable;
	int m_iSoftTriggerWaitInMilliSeconds;
	unsigned int m_iSoftTriggerImageNodeNumber;

	void init();
	void resetGainMode();
	void resetBalanceAuto();
};
#endif //HAIKANG_CAMERA_CONFIG
