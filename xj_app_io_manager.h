#ifndef XJ_APP_IO_MANAGER_H
#define XJ_APP_IO_MANAGER_H

#include "io_card_manager.h"
#include "iostream"
#include "modbus.h"

class AppIoManager : public AdvtechDirectIOBoardManager
{
public:
    AppIoManager(const std::string &name, const int signalPulseWidth) :
		        AdvtechDirectIOBoardManager(name),
                m_signalPulseWidth(signalPulseWidth)
    {
        init();
    }
    virtual ~AppIoManager() {};

    virtual int init();
    // bool sendGoodSignals(const int port, const std::array<bool, CHANNEL_NUMBER> &bits) { return pulseWrite(port, bits, m_signalPulseWidth); }
    bool sendGoodSignals(const int port, const std::array<bool, 16> &bits);// { return true; }
    bool sendGoodSignals_horse();

private:
    int m_signalPulseWidth;
};

class AppPlcManager : public modbus
{
public:
  AppPlcManager(std::string host, uint16_t port, int slaveId):
  modbus(host, port), m_iSlaveId(slaveId)
  {
    init();
  }
    virtual ~AppPlcManager() {modbus_close();};
    virtual int init();


private:
  int m_iSlaveId;
};


#endif // XJ_APP_IO_MANAGER_H
