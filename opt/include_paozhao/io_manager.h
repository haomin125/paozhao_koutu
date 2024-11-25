#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <string>

// for Serial Port
#include "ceSerial.h"
#include "io_manager_utils.h"

class BaseIoManager
{
public:
	BaseIoManager();
	virtual ~BaseIoManager() {}

	virtual int init() = 0;

	// This method is the base one which can be accessed from base detector to purge board result
	// messageId: This parameter will associate with message that we want to purge, for instance, 
	//            messageId = 1 means A1, messageId = 2 means A2, and so on
	virtual int Purge(const int messageId) = 0;

	// this method is designed for web server when UI need to access light source setting
	// channel: the channel number of light source
	// value: a unsigned integer associated with brightness of the light source
	virtual bool write(const int channel, const unsigned int value) = 0;

	// this method is designed for web server when UI need to access regular io manager
	// id: port, unit id, or message id, which depends on different type of io manager
	// parameter: pulse width, register address, or the second parameter which used in different io manager
	// data: data write to io manager
	virtual bool write(const int id, const int parameter, const unsigned int data) = 0;
};


class PlcManager : public BaseIoManager
{
public:
	PlcManager(const std::string &gateway, const std::string &protocal = "ab_eip", const std::string &cpu = "SLC", const std::string &path="");
	virtual ~PlcManager() {};

	// set element size after plc manager decleared or each time before read or write
	int getElementSize() const { return m_iElementSize; }
	void setElementSize(const int size) { m_iElementSize = size; }

	// get/set plc internal debug level, return from set will be the previous debug level
	int getPlcDebugLevel();
	int setPlcDebugLevel(const int level);

	virtual int init();

	virtual bool write(const std::string &name, const int value);
	virtual bool read(const std::string &name, int& value);

	// this method can be overriden from subclass to access light source setting if you need
	virtual bool write(const int channel, const unsigned int value) { return false; }
	// this method can be overriden from subclass to access io manager to test if remove bad product successfully
	virtual bool write(const int id, const int parameter, const unsigned int data) { return false; }
private:
	virtual int Purge(const int messageId) final { return -1; }

	std::string createTag(int eSize, const std::string& name) const;

	std::string m_sTagHeader;
	std::string m_sGateway;
	std::string m_sProtocal;
	std::string m_sCpu;
	std::string m_sPath;
	int m_iElementSize;

	static std::mutex m_plcMutex;
};

class SerialManager : public BaseIoManager
{
public:
	SerialManager();
	SerialManager(const std::string &device, const long baudRate, const long dataSize, const char parityType, const float nStopBits);
	virtual ~SerialManager() { close(); }

	virtual int init();

	void setChecksumType(const IoChecksumType type);
	IoChecksumType checksumType() const { return m_checksumType; }

	// Parameters:
	//     messageId: This parameter will associate with message that we want to purge, for instance, 
	//                messageId = 1 means A1, messageId = 2 means A2, and so on
	//     channel:   This is communication channel, we may have multiple channels in one IoManager
	// There are 2 way to use Purge in SerialManager
	//     1. override Purge(messageId) and use purgeBoardResult in BaseDetector
	//     2. override PurgeBoardResult(..) in AppDetector, and use Purge(messageId, channel)
	virtual int Purge(const int messageId);
	virtual int Purge(const int messageId, const int channel = 0);

	// create a new ceSerial port object, and append to the m_serialPorts, if there is no other parameters
	// use those parameters from m_serialPorts[0]
	bool addSerialPort(const int channel, const std::string &device);
	bool addSerialPort(const int channel, const std::string &device, const long baudRate, const long dataSize, const char parityType, const float nStopBits);

	bool isValid() { return m_bIsValid; }
	std::shared_ptr<ce::ceSerial> getSerialPort(const int channel);

	int readTimeout() const { return m_iReadTimeout; }
	void setReadTimeout(const int timeout) { m_iReadTimeout = timeout; }

	// this method can be overriden from subclass to access light source setting if you need
	virtual bool write(const int channel, const unsigned int value) { return false; }
	// this method can be overriden from subclass to access io manager to test if remove bad product successfully
	virtual bool write(const int id, const int parameter, const unsigned int data) { return false; }
protected:
	IoChecksumType m_checksumType;
	std::shared_ptr<io_manager_utils::MessageFactory> m_pMessageFactory;

	std::map<int, std::shared_ptr<ce::ceSerial>> m_serialPorts;

	int read(char &ch, const int channel = 0);
	int write(const char *s, const int channel = 0);
	int writeBuffer(const char *s, const long n, const int channel = 0);
	int writeAndReadBuffer(const char *sendBuf, const int sendLength, char *recvBuf, const int expectedRecvLength, const int channel = 0);

	static std::map<int, std::mutex> m_serialMutex;
	std::mutex& getSerialMutex(const int channel)
	{
		int channelIdx = (channel < 0 || channel >= m_serialPorts.size()) ? 0 : channel;
		return m_serialMutex[channelIdx];    // constructs it inside the map if doesn't exist
	}

private:
	bool m_bIsValid;
	int m_iReadTimeout;

	int close();
};

class TcpIpManager : public BaseIoManager
{
public:
	TcpIpManager();
	TcpIpManager(const std::string &address, int port);
	virtual ~TcpIpManager() { tcpipDisconnect(); }

	virtual int init() = 0;

	// if buffer is nullptr, or buffer contains error code, return true, otherwise, return false
	virtual bool errorHappens(const unsigned char *buffer) = 0;

	// this method can be overriden from subclass to access light source setting if you need
	virtual bool write(const int channel, const unsigned int value) = 0;
	// this method can be overriden from subclass to access io manager to test if remove bad product successfully
	virtual bool write(const int id, const int parameter, const unsigned int data) = 0;

	bool isConnected() const { return m_bIsConnected; }

	bool needReconnect() const { return m_bNeedReConnect; }
	void needReconnect(const bool flag) { m_bNeedReConnect = flag; }
	bool disconnectWhenError() const { return m_bDisconnectWhenError; }
	void disconnectWhenError(const bool flag) { m_bDisconnectWhenError = flag; }

	std::string ipAddress() { return m_sIpAddress; }
	void setIpAddress(const std::string &address) { m_sIpAddress = address; }
	int port() const { return m_iPort; }
	void setPort(const int port) { m_iPort = port; }
	int readTimeout() const { return m_iReadTimeout; }
	void setReadTimeout(const int timeout) { m_iReadTimeout = timeout; }
	int sendTimeout() const { return m_iSendTimeout; }
	void setSendTimeout(const int timeout) { m_iSendTimeout = timeout; }

protected:
	bool tcpipConnect();
	bool sendAndCheckResponse(const unsigned char *sendCmd, const int cmdSize, unsigned char *responseBuffer, const int responseSize, const std::string &header);

private:
	virtual int Purge(const int messageId) final { return false; }

	void tcpipClose();
	void tcpipDisconnect();
	std::string errnoMsg(const int errorCode) const;

	std::string m_sIpAddress;
	int m_iPort;
	bool m_bIsConnected;
	bool m_bNeedReConnect;
	bool m_bDisconnectWhenError;

	int m_iSocketfd;
	int m_iReadTimeout;
	int m_iSendTimeout;

	static std::mutex m_tcpipMutex;
};

#endif // IO_MANAGER_H