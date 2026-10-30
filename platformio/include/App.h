#pragma once
#ifndef APP_H
#define APP_H

#include "system.h"
#include "SettingLimits.h"
#include "Network.h"
#include "PvBoiler.h"
#include "Ssd1306.h"
#include "Terminal.h"
#include "TrackedValue.h"

class CApp
{
  public:
    CApp();

    void Init();
    void UpdateValues();
    void Loop();

    void IRAM_ATTR ZeroCrossHandler(const uint32_t iNow);
    void IRAM_ATTR TriacGateHandler();

#ifdef ESP32
    void InitMcpwm(void (*isr)(void*));
    void IRAM_ATTR McpwmHandler();
#endif

    bool CommandHandler();
    void HandleNetwork();
    void HandleDisplay();

    CNetwork& GetNetwork() { return m_network; };
    CPvBoiler& GetPvBoiler() { return m_pvBoiler; };

    void MqttRequestRepublish() { m_bMqttRepublish = true; m_mqttRepublishTimer = 0; };

  private:
    bool IRAM_ATTR ScheduleTriac(const uint32_t iFireTime);
    void IRAM_ATTR ArmTriacTimer(const uint32_t iDelayUs);
    void IRAM_ATTR DisarmTriacTimer();
    bool IRAM_ATTR IsGateBlanking(const uint32_t iNow) const;

    static const char* FormatBar(char* strLine, const float fValue);

    CNetwork m_network;
    CPvBoiler m_pvBoiler;
    bool m_bMqttRepublish = false;
    elapsedMillis m_mqttRepublishTimer = 0;
    CPvBoilerCommandHandler m_commandHandler;
    CTerminal m_terminal;
    CSsd1306 m_display;
    uint8_t m_displayCount = 0;

    elapsedMillis m_ledTimer = 0;
    elapsedMillis m_displayTimer = 0;

    CTrackedValue m_period;
    CTrackedValue m_zeroCrossWindow;

    uint32_t m_iLastPeriodStartTime = 0; // ISR only
    uint32_t m_iLastEventTime = 0; // ISR only

    volatile bool m_bTriacOn = false;
    volatile bool m_bGateBlanking = false; // ESP8266 only
    volatile uint32_t m_iGateTime = 0; // ESP32 only: start of the (hardware generated) gate pulse
    volatile bool m_bTriacBackupPending = false;
    volatile uint32_t m_iTriacBackupTime = 0;
    uint16_t m_iSSRPeriodCounter = 0; // ISR only, half periods
    bool m_bSSRCycleOn = false; // ISR only
    volatile uint16_t m_iTriacDelayUs = 0;
    volatile uint8_t m_iSSRPeriodCount = SSR_PERIOD_COUNT_DEFAULT; // Full cycles, always non-zero
    volatile uint8_t m_iSSROnCycles = 0; // Full cycles
    volatile CPvBoiler::dim_style_t m_dimStyle = CPvBoiler::DIM_STYLE_NONE;
};
#endif // APP_H
