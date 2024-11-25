#ifndef MOCK_CAMERA_CONFIG_H
#define MOCK_CAMERA_CONFIG_H

#include "config.h"

#include <string>

class MockCameraConfig : public CameraConfig
{
public:
	MockCameraConfig();
	MockCameraConfig(const std::string &model, const std::string &fileName);
	virtual ~MockCameraConfig() {}

	std::string &fileName() { return m_sFileName; }
	void fileName(const std::string &fname) { m_sFileName = fname; }

	virtual bool setConfig() { return !m_sFileName.empty(); }

	virtual std::stringstream getConfigParameters();
	virtual void setConfigParameters(const std::stringstream &parameters);
private:
	std::string m_sFileName;
};

class MockDirectoryConfig : public CameraConfig
{
public:
	MockDirectoryConfig();
	MockDirectoryConfig(const std::string &model, const std::string &directoryName, const std::string &filterFileType);
	virtual ~MockDirectoryConfig() {}

	std::string &directoryName() { return m_sDirectoryName; }
	void directoryName(const std::string &dname) { m_sDirectoryName = dname; }
	std::string &filterFileType() { return m_sFilterFileType; }
	void filterFileType(const std::string &ftype) { m_sFilterFileType = ftype; }

	virtual bool setConfig() { return !m_sDirectoryName.empty(); }

	virtual std::stringstream getConfigParameters();
	virtual void setConfigParameters(const std::stringstream &parameters);
private:
	std::string m_sDirectoryName;
	std::string m_sFilterFileType;
};

#endif // MOCK_CAMERA_CONFIG_H