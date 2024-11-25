#ifndef IO_CARD_MANAGER_H
#define IO_CARD_MANAGER_H

#include <string>
#include <thread>
#include <array>

#include "io_manager.h"

#include "bdaqctrl.h"

#define CHANNEL_NUMBER 8

// AdvTech Simple Direct IO Board: including PCIe-1751, PCI-1730
class AdvtechDirectIOBoardManager : public BaseIoManager 
{
public:
	AdvtechDirectIOBoardManager(const std::string &name);
	virtual ~AdvtechDirectIOBoardManager();

	virtual int init();

	// this method can be overriden from subclass to access light source setting if you need
	virtual bool write(const int channel, const unsigned int value) { return false; }
	// this method can be overriden from subclass to access io manager to test if remove bad product successfully
	virtual bool write(const int id, const int parameter, const unsigned int data) { return false; }

	//NOTE:
	//This function is used to write byte data to the specified DO channel immediately
	//argument1:which port you want to control? For example, startPort is 0.
	//argument2:data write to port. 
	bool write(const int port, const unsigned char data);

	//NOTE:
	//This function is used to read byte data from the specified DI channel immediately
	//argument1:which port you want to control? For example, startPort is 0.
	//argument2:data read from port. 
	bool read(const int port, unsigned char &data);

	//NOTE:
	//This function is used to write single bit data to the specified DO channel immediately
	//argument1:which port you want to control? For example, startPort is 0.
	//argument2:which bit you want to control? You can write 0--7, any number you want.
	//argument3:what status you want, open or close? 1 menas open, 0 means close.*/
	bool writeBit(const int port, const unsigned char bit, const unsigned char data);

	//NOTE:
	//This function is used to read single bit data from the specified DI channel immediately
	//argument1:which port you want to control? For example, startPort is 0.
	//argument2:which bit you want to control? You can read 0--7, any number you want.
	//argument3:data read from port
	bool readBit(const int port, const unsigned char bit, unsigned char &data);

	//NOTE:
	//This function is used to create a pulse output on the specified DO channel immediately
	//argument1:which port you want to control? For example, startPort is 0.
	//argument2:which bit you want to send? It could be 0--7, any number you want.
	//argument3:pulse width in milliseconds
	bool pulseWrite(const int port,  const unsigned char bit, const int pulseWidth);

	//NOTE:
	//This function is used to create a pulse output on the multiple channels immediately
	//argument1:which port you want to control? For example, startPort is 0.
	//argument2:array of bits you want to send if bits[i] is 1
	//argument3:pulse width in milliseconds
	bool pulseWrite(const int port, const std::array<bool, CHANNEL_NUMBER> &bits, const int pulseWidth);

private:
	// the function below inherits from virtual base class, but not used in this class.
	virtual int Purge(const int messageId) final { return false; }

	std::string m_sName;
	Automation::BDaq::InstantDoCtrl *m_pInstantDoCtrl;
	Automation::BDaq::InstantDiCtrl *m_pInstantDiCtrl;

	static std::mutex m_advTechBoardMutex;
};

#endif // IO_CARD_MANAGER_H