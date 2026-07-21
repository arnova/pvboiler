#pragma once
#ifndef APP_H
#define APP_H

#include "system.h"
#include "Network.h"
#include "PvBoiler.h"
#include "Ssd1306.h"

class CApp
{
  public:
    CApp();

    void Init();
    void UpdateValues();
    void Loop();

    void IRAM_ATTR ZeroCrossHandler();
    void IRAM_ATTR TriacGateHandler();

    void PollSerial();
    void PollEthernet();
    void HandleNetwork();
    void HandleDisplay();

    CNetwork& GetNetwork() { return m_network; };
    CPvBoiler& GetPvBoiler() { return m_pvBoiler; };

  private:
    CNetwork m_network;
    CPvBoiler m_pvBoiler;
    CPvBoilerCommandHandler m_commandHandler;
    CSsd1306 m_display;
    uint8_t m_displayCount = 0;

    elapsedMillis m_ledTimer = 0;
    elapsedMillis m_displayTimer = 0;

    volatile uint32_t m_iLastZeroCrossTime = 0;
    volatile uint32_t m_iLastEventTime = 0;
    volatile uint32_t m_iPhaseCorrectionTime = ZERO_CROSS_PHASE_CORRECTION_DEFAULT;
    volatile uint32_t m_iZeroCrossTime = 0;
    volatile bool m_bTriacOn = false;
    volatile uint8_t m_iSSRPeriodCounter = 0;
    volatile uint32_t m_iTriacDelayTicks = 0;
    volatile uint8_t m_iSSRPeriodCount = 0;
    volatile uint8_t m_iCurrentPercentage = 0;
    volatile CPvBoiler::dim_style_t m_dimStyle = CPvBoiler::DIM_STYLE_NONE;

    uint8_t m_termCharCount = 0;
    char m_strTermCommand[CMD_BUF_SIZE] = { 0 };
    char m_strTermOldCommand[CMD_BUF_SIZE] = { 0 };
};
#endif // APP_H