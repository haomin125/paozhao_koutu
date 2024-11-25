#ifndef APP_WEB_SERVER_H
#define APP_WEB_SERVER_H

#include "apb_web_server.hpp"
#include "running_status.h"

enum StringValue
{
    all = 0,
    cropImage,
    rotationOfTD
};

class AppWebServer : public ApbWebServer
{
public:
    AppWebServer(const std::shared_ptr<CameraManager> pCameraManager);
	AppWebServer(const std::shared_ptr<CameraManager> pCameraManager, const std::string &address, const unsigned short port);
	virtual ~AppWebServer() {};

private:
    std::map<std::string, StringValue> mapStringValues;

	int addWebCmds();

    int CreateTestProduction();

    int SettingWrite();

    int SetRect();

    int SetParams();

    int GetDetectingParams();

    int GetImagePath();

    int SetImagePath();

    int GetPlcParams();

	int SetPlcParams();

    int SetDefaultAutoParams();

    int Preview();
    


    ProductSetting::PRODUCT_SETTING m_testProductSetting;
};

#endif //APP_WEB_SERVER_H