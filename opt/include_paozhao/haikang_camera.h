#ifndef HAIKANG_CAMERA_H
#define HAIKANG_CAMERA_H

#include "camera.h"
#include "haikang_camera_config.h"
#include "MvCameraControl.h"
#include <opencv2/opencv.hpp>

class HaikangCamera: public BaseCamera
{
public:
    HaikangCamera(const std::string &cameraName, const std::string &deviceName, const std::shared_ptr<HaikangCameraConfig> pConfig);

    virtual ~HaikangCamera();

    virtual bool start();
    virtual bool open();
    virtual bool read(cv::Mat & frame);
    virtual void release();
    virtual bool restart();

private:
    void *m_pCapture;
    unsigned int m_nTLayerType;

    bool blockRead(cv::Mat &frame);
    bool nonBlockRead(cv::Mat &frame);

    bool sendSoftwareTriggerCommand();

    // convert ip address from string to int, such as 10.64.57.91 -> 0x0a40395b
    int convertIpToHex(const std::string &ip);
};
#endif //HAIKANG_CAMERA
