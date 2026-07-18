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
    void IRAM_ATTR TriacGateHandler();

    void MqttPublish();
    void PollSerial();
    void PollEthernet();
    void HandleNetwork();
    void HandleDisplay();

    CNetwork& GetNetwork() { return m_network; };
    CPVBoiler& GetPvBoiler() { return m_pvBoiler; };
    CPVBoilerCommandHandler& GetPvBoilerCommandHandler() { return m_commandHandler; };

  private:
    CNetwork m_network;
    CPVBoiler m_pvBoiler;
    CPVBoilerCommandHandler m_commandHandler;
    CSSD1306 m_display;
    uint8_t m_displayCount = 0;

    elapsedMillis m_ledTimer = 0;
    elapsedMillis m_displayTimer = 0;

    volatile uint32_t m_iLastZeroCrossTime = 0;
    volatile uint32_t m_iLastEventTime = 0;
    volatile uint32_t m_iPhaseCorrectionTime = ZERO_CROSS_PHASE_CORRECTION_DEFAULT;
    volatile uint32_t m_iZeroCrossTime = 0;
    volatile bool m_bTriacOn = false;
    volatile uint8_t m_iSSRPeriodCounter = 0;

    uint8_t m_termCharCount = 0;
    char m_strTermCommand[CMD_BUF_SIZE] = { 0 };
    char m_strTermOldCommand[CMD_BUF_SIZE] = { 0 };
};
#endif // APP_H