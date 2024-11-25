#ifndef APB_WEB_SERVER_HPP
#define APB_WEB_SERVER_HPP

#include "web_server.hpp"

//////////////////////////////////////////////////
//
// This is sub web server for APB(Aluminum/Plastic)
//
//////////////////////////////////////////////////
class ApbWebServer : public WebServer
{
public:
	ApbWebServer(const std::shared_ptr<CameraManager> pCameraManager);
	ApbWebServer(const std::shared_ptr<CameraManager> pCameraManager, const std::string &address, const unsigned short port);
	virtual ~ApbWebServer() {};

private:
	int addWebCmds();
	bool saveTestProdSetting(const std::shared_ptr<HttpServer::Request> &request, const bool updateMemory, std::string &returnedContent);

	// Get commands
	int GetListUsers();
	int GetRole();
	int GetAllUsers();
	int GetRolePrivilege();
	int GetAlarms();
	int GetAlarmEventTotal();

	// Post commands
	int Login();
	int ChangePwd();
	int AddUser();
	int DeleteUser();
	int DeleteUsers();
	int ActivateUser();
	int UpdateUserInfo();
	int AddRolePrivilege();
	int DeleteRolePrivilege();
	int UpdateRolePrivilege();
	int AddUserEvent();
	int SetCurrentProd();
	int CreateTestProduction();
	int SettingWrite();

	// others
	int ImageProcessed();
	int GetImagePath();
	int SetImagePath();
	int SaveCameraParams();
};

#endif // APB_WEB_SERVER_HPP