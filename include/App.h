#pragma once
#ifndef APP_H
#define APP_H

#include "system.h"
#include "Network.h"
#include "pvboiler.h"

class CApp
{
  public:
    CApp()
      : m_network()
      , m_pvBoiler(m_network)
      , m_commandHandler(m_pvBoiler, m_network)
    {}

    bool MQTTReconnect();
    void pollSerial();
    void pollEthernet();
    bool CheckNetwork();

    CNetwork& GetNetwork() { return m_network; };
    CPVBoiler& GetPvBoiler() { return m_pvBoiler; };
    CPVBoilerCommandHandler& GetPvBoilerCommandHandler() { return m_commandHandler; };

  private:
    CNetwork m_network;
    CPVBoiler m_pvBoiler;
    CPVBoilerCommandHandler m_commandHandler;

    elapsedMillis m_mqttReconnectTimer = 0;
    elapsedMillis m_wifiReconnectTimer = 0;
    elapsedMillis m_ledTimer = 0;
    bool m_bWifiConnected = false;
};
#endif // APP_H