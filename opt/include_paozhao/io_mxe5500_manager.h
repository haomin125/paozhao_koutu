#ifndef IO_CARD_MANAGER_H
#define IO_CARD_MANAGER_H

#include <string>
#include <thread>
#include <array>

#include "io_manager.h"

// AdLink MXE-5500 Series
class AdlinkMxe5500IoManager : public BaseIoManager 
{
public:
	AdlinkMxe5500IoManager();
	virtual ~AdlinkMxe5500IoManager() {};

	virtual int init();

	// this method can be overriden from subclass to access light source setting if you need
	virtual bool write(const int channel, const unsigned int value) { return false; }
	// this method can be overriden from subclass to access io manager to test if remove bad product successfully
	virtual bool write(const int id, const int parameter, const unsigned int data) { return false; }

	//NOTE:
	//This function is used to set digital logic status in output line
	//argument1: digital logic status presented by an unsigned short
	bool write(const unsigned short data);

	//NOTE:
	//This function is used to set digital logic status in output line, plus set back to 0 after a certain pulse width
	//argument1: digital logic status presented by an unsigned short
	//argument2: pulse width in milliseconds
	bool pulseWrite(const unsigned short data, const int pulseWidth);

	//NOTE:
	//This function is used to read digital logic status in input line
	//argument1: digital logic status returned by an unsigned short
	bool readInput(unsigned short &data);

	//NOTE:
	//This function is used to read digital logic status in output line
	//argument1: digital logic status returned by an unsigned short
	bool readOutput(unsigned short &data);

private:
	// the function below inherits from virtual base class, but not used in this class.
	virtual int Purge(const int messageId) final { return false; }

	static std::mutex m_mxe5500Mutex;
};

#endif // IO_CARD_MANAGER_H