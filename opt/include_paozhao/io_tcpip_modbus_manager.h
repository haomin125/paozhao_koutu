#ifndef IO_TCPIP_MODBUS_MANAGER_H
#define IO_TCPIP_MODBUS_MANAGER_H

#include "io_manager.h"

#include <string>
#include <vector>
#include <map>

namespace TcpipModbusConstants
{
	// function code
	const int FunctionCodeUndefined = 0x00;
	const int ReadHoldingRegisters = 0x03;
	const int WriteSingleRegister = 0x06;
	const int WriteMultipleRegisters = 0x10;

	// we only support these 3 functions, but we can expand more if required
	const std::map<unsigned int, std::string> FunctionCode = { 
		{0x03, "READ_HOLDING_REGISTERS"},
		{0x06, "WRITE_SINGLE_REGISTER"},
		{0x10, "WRITE_MULTIPLE_REGISTERS"}
	};

	// the functions we support should have the following exception code, we can expand more if required
	const std::map<unsigned int, std::string> ExceptionCode = {
		{0x01, "ILLEGAL_FUNCTION"},
		{0x02, "ILLEGAL_DATA_ADDRESS"},
		{0x03, "ILLEGAL_DATA_VALUE"},
		{0x04, "SERVER_DEVICE_FAILURE"},
		{0x05, "ACKNOWLEDGE"},
		{0x06, "SERVER_DEVICE_BUSY"},
	};
};

class TcpipModbusManager : public TcpIpManager
{
public:
	TcpipModbusManager(const std::string &serverIP, int serverPort);
	TcpipModbusManager(const std::string &serverIP, int serverPort, const int sendRecvTimeout);
	virtual ~TcpipModbusManager() {};

	virtual int init();
	virtual bool errorHappens(const unsigned char *responseBuffer);

	virtual bool writeRegisters(const int address, const std::vector<int> &data);
	virtual bool writeRegister(const int address, const int data);
	virtual bool readRegisters(const int address, std::vector<int> &data, const int size);
	virtual bool readRegister(const int address, int &data);

	// this method can be overriden from subclass to access light source setting if you need
	virtual bool write(const int channel, const unsigned int value) { return false; }
	// this method can be overriden from subclass to access io manager to test if remove bad product successfully
	virtual bool write(const int id, const int parameter, const unsigned int data) { return false; }

private:
	static std::mutex m_plcMutex;

	unsigned int m_iMsgId;
	unsigned int m_iFunctionCode;

	std::string functionName(const int functionCode) const;
	std::string exceptionMsg(const int exceptionCode) const;

	bool encodeHeader(unsigned char *buffer, const unsigned short length);
	bool decodeHeader(const unsigned char *buffer, unsigned short &length);
};

#endif // IO_TCPIP_MODBUS_MANAGER_H