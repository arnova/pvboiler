#pragma once
#ifndef APP_H
#define APP_H

#include "system.h"
#include "Network.h"
#include "pvboiler.h"
#include "ssd1306.h"

class CApp
{
  public:
    CApp();

    void Init();
    void Loop();

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
    CSSD1306 m_display;

    elapsedMillis m_mqttReconnectTimer = 0;
    elapsedMillis m_wifiReconnectTimer = 0;
    elapsedMillis m_ledTimer = 0;
    bool m_bWifiConnected = false;
};
#endif // APP_H