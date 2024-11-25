#ifndef IO_SEEKING_MANAGER_H
#define IO_SEEKING_MANAGER_H

#include "io_manager.h"

#include <mutex>

enum class SeekingIoResultType : int
{
	SEEKING_IO_RESULT_DIRECT = 0,
	SEEKING_IO_RESULT_BELT_COMBINED = 1,
	SEEKING_IO_RESULT_TURNTABLE_COMBINED = 2
};

class SeekingIoManager : public SerialManager
{
public:
	SeekingIoManager();
	SeekingIoManager(const std::string &device, const long baudRate, const long dataSize, const char parityType, const float nStopBits);
	virtual ~SeekingIoManager() {};

	// Parameters:
	//   messageId: This parameter will associate with message that we want to purge, for instance, 
	//              messageId = 1 means A1, messageId = 2 means A2, and so on
	//   channel:   This is communication channel, we may have multiple channels in one IoManager
	//   type:      This parameters works for seeking special IoManager to tell the type which result
	//              will come from
	virtual int Purge(const int messageId, const int channel = 0, const SeekingIoResultType type = SeekingIoResultType::SEEKING_IO_RESULT_DIRECT);

	// this method can be overriden from subclass to access light source setting if you need
	virtual bool write(const int channel, const unsigned int value) { return false; }
	// this method can be overriden from subclass to access io manager to test if remove bad product successfully
	virtual bool write(const int id, const int parameter, const unsigned int data) { return false; }
private:
	static std::mutex m_seekingMutex;
	int write(const char *s, const int nSendLength, char *r, const int nExpectReceiveBytes, const int channel = 0);

	friend std::ostream& operator<<(std::ostream& os, const std::vector<char> &data);
};

#endif //IO_SEEKING_MANAGER_H