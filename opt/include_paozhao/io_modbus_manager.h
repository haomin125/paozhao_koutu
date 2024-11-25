#ifndef IO_MODBUS_MANAGER_H
#define IO_MODBUS_MANAGER_H

#include "io_manager.h"

#include <mutex>

/////////////////////////////////////////////////////////////////////
// is that possible we have a chance to connect mutiple modbus PLCs in 
// one projects? we limit this possibility for now, and expand it later
// if need
/////////////////////////////////////////////////////////////////////
class SeekingModbusManager : public SerialManager
{
public:
	SeekingModbusManager(const std::string &device, const long baudRate, const long dataSize, const char parityType, const float nStopBits);
	virtual ~SeekingModbusManager() {};

	// Parameters:
	//   unitId:    This is 1 byte additional address in modbus protocol, we called unit id, range 
	//              is from 1 to 247. When we have one plc in slave, unit Id is 0x01.
	//   address:   This is register address in modbus protocal, which you want to read from or write
	//              write to, address is 2 bytes long, the maximum address could be up to 8000.
	//   quantity:  This is how many registers you want to read from or write to, quantity is 2 bytes
	//              long too. 
	//   recvData:  This is a vector of short, the data we read from register, the length of recvData
	//              should be the same as quantity.
	//   sendData:  This is a vector of short too, the data we write to the registers, the length
	//              of sendData should be the same as quantity as well.
	virtual bool readRegisters(const unsigned char unitId, const unsigned short address, const unsigned short quantity, std::vector<short> &recvData);
	virtual bool writeRegisters(const unsigned char unitId, const unsigned short address, const std::vector<short> &sendData);

	// this method can be overriden from subclass to access light source setting if you need
	virtual bool write(const int channel, const unsigned int value) { return false; }
	// this method can be overriden from subclass to access io manager to test if remove bad product successfully
	virtual bool write(const int id, const int parameter, const unsigned int data) { return false; }
private:
	virtual int Purge(const int messageId) final { return -1; }
	virtual int Purge(const int messageId, const int channel = 0) final { return -1; }

	static std::mutex m_modbusMutex;
	bool write(const unsigned char unitId, const unsigned short address, const std::vector<short> &sendData);
};

#endif //IO_MODBUS_MANAGER_H