#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include "camera.h"
#include "config.h"

#include <map>
#include <string>

class CameraManager
{
public:
	CameraManager();
	virtual ~CameraManager() {}

	size_t camerasSize() { return m_cameras.size(); }

	bool addCamera(std::shared_ptr<BaseCamera> pCamera);

	bool startCamera(const std::string &cameraName);
	void stopCamera(const std::string &cameraName);
	void stopCameras();
	bool startCameras();

	std::shared_ptr<BaseCamera> getCamera(const std::string &cameraName);
 
	bool deviceName(const std::string &cameraName, std::string &deviceName);
private:
	std::map<std::string, std::shared_ptr<BaseCamera> > m_cameras;

	bool start(const std::shared_ptr<BaseCamera> &camera);
};


#endif // CAMERA_MANAGER_H