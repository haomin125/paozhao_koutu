#include "xj_app_io_manager.h"
#include <unistd.h>

// app need to define its own io manager if it does not use SeekingIoManager from framework

int AppIoManager::init()
{
	// please add your own code if necessary
	return 0;
}

bool AppIoManager::sendGoodSignals(const int port, const std::array<bool, 16> &bits)
{
	for (int i = 0; i < bits.size(); i++)
	{
		if (bits[i] == true)
			writeBit(0, i, 1);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(m_signalPulseWidth));
	for (int i = 0; i < bits.size(); i++)
	{
		if (bits[i] == true)
			writeBit(0, i, 0);
	}
}
bool AppIoManager::sendGoodSignals_horse()
{
	 while (true)
        {
            for(int i = 0; i < 8; ++i)
            {
                // write IO
                writeBit(0, i, 1);
                writeBit(1, i, 1);
                sleep(1);
                writeBit(0, i, 0);
                writeBit(1, i, 0);

            }
            std::cout<<"------------------"<<std::endl;
            for(int i = 0; i < 8; ++i)
            {
                writeBit(0, i, 0);
                writeBit(1, i, 0);
            }
        }
}

int AppPlcManager::init()
{
    // set slave id
    modbus_set_slave_id(m_iSlaveId);
    // connect with the server
    modbus_connect();
	return 0;
}