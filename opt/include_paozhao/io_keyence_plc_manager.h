#ifndef IO_KEYENCE_PLC_MANAGER_H
#define IO_KEYENCE_PLC_MANAGER_H

#include "io_manager.h"

#include <string>
#include <vector>

class KeyencePlcManager : public TcpIpManager
{
public:
	KeyencePlcManager(const std::string &serverIP, int serverPort);
	KeyencePlcManager(const std::string &serverIP, int serverPort, const int sendRecvTimeout);
	virtual ~KeyencePlcManager() {}

	virtual int init();
	virtual bool errorHappens(const unsigned char *responseBuffer);

	virtual bool writeRegisters(const std::string &address, const std::vector<int> &data);
    virtual bool writeRegister(const std::string &address, const int data);
	virtual bool readRegisters(const std::string &address, std::vector<int> &readData, const int size);
    virtual bool readRegister(const std::string &address, int &readData);

	// this method can be overriden from subclass to access light source setting if you need
	virtual bool write(const int channel, const unsigned int value) { return false; }
	// this method can be overriden from subclass to access io manager to test if remove bad product successfully
	virtual bool write(const int id, const int parameter, const unsigned int data) { return false; }

private:
	static std::mutex m_plcMutex;
};

#endif // IO_KEYENCE_PLC_MANAGER_H