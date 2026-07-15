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

    void IRAM_ATTR ZeroCrossHandler();
    void IRAM_ATTR TriacPhaseHandler();

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

    volatile uint32_t m_iLastZeroCrossTime = 0;
    volatile uint32_t m_iPhaseCorrectionTime = 300; // Default = 300 uS
    volatile uint32_t m_iZeroCrossTime = 0;
    volatile bool m_bZeroCrossTimeUpdated = false;
    volatile bool m_bTriacOn = false;
    volatile float m_fTriacAngleFactor = 1.0f; // Off
    volatile uint8_t m_iOutputPercentage = 0;
    volatile uint8_t m_iSSRPeriodCounter = 0;
};
#endif // APP_H