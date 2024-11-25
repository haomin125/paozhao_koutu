#ifndef IO_OMRON_PLC_MANAGER_H
#define IO_OMRON_PLC_MANAGER_H

#include "io_manager.h"

#include <string>
#include <vector>

class OmronPlcManager : public TcpIpManager
{
public:
	OmronPlcManager(const std::string &serverIP, int serverPort, const std::string &localIP);
	virtual ~OmronPlcManager() {};

	virtual int init();
	virtual bool errorHappens(const unsigned char *responseBuffer);

	virtual bool writeRegisters(const int address, const std::vector<int> &data);
	virtual bool readRegisters(const int address, std::vector<int> &readData, const int size);

	// this method can be overriden from subclass to access light source setting if you need
	virtual bool write(const int channel, const unsigned int value) { return false; }
	// this method can be overriden from subclass to access io manager to test if remove bad product successfully
	virtual bool write(const int id, const int parameter, const unsigned int data) { return false; }

private:
	std::string m_sLocalIP;
	bool m_bNeedHandShake;

	static std::mutex m_plcMutex;

	bool handshake();
};

#endif // IO_OMRON_PLC_MANAGER_H