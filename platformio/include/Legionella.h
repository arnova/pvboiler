#pragma once
#ifndef LEGIONELLA_H
#define LEGIONELLA_H

#include "system.h"

class CLegionella
{
  public:
    CLegionella() {}; // Empty ctor
    ~CLegionella() {}; // Empty dtor

    void Reset() { m_iSecondsPassedSinceLastDisinfect = 0; m_iDisinfectRuntimeSeconds = 0; m_iSecondsInDangerZone = 0; m_bDisinfectionRequired = false; };
    void Loop();

    const uint32_t GetHoursPassedSinceLastDisinfect() const { return m_iSecondsPassedSinceLastDisinfect / 3600; };
    const uint32_t GetDisinfectRunTimeSeconds() const { return m_iDisinfectRuntimeSeconds; };
    const uint32_t GetHoursInDangerZone() const { return m_iSecondsInDangerZone / 3600; };
    bool DisinfectionRequired() const { return m_bDisinfectionRequired; };

    void UpdateTemperature(const float fTemperature) { m_fTemperature = fTemperature; };
    void SetHoursPassedSinceLastDisinfect(const uint32_t iHours) { m_iSecondsPassedSinceLastDisinfect = iHours * 3600; };
    void SetHoursInDangerZone(const uint32_t iHours) { m_iSecondsInDangerZone = iHours * 3600; };

    void SetDangerZoneLowTemperature(const uint8_t iTemperature) { m_iDangerZoneLowTemperature = iTemperature; };
    void SetDangerZoneHighTemperature(const uint8_t iTemperature) { m_iDangerZoneHighTemperature = iTemperature; };
    void SetDisinfectIntervalDays(const uint8_t iDays) { m_iDisinfectIntervalDays = iDays; };
    void SetDisinfectRunTimeMinutes(const uint8_t iMinutes) { m_iDisinfectRunTimeMinutes = iMinutes; };
    void SetDisinfectTemperature(const uint8_t iTemperature) { m_iDisinfectTemperature = iTemperature; };

  private:
    uint32_t m_iSecondsPassedSinceLastDisinfect = 0; // s
    uint32_t m_iDisinfectRuntimeSeconds = 0; // s
    uint32_t m_iSecondsInDangerZone = 0; // s

    uint8_t m_iDangerZoneLowTemperature = DANGER_ZONE_LOW_TEMPERATURE_DEFAULT; // C
    uint8_t m_iDangerZoneHighTemperature = DANGER_ZONE_HIGH_TEMPERATURE_DEFAULT; // C
    uint8_t m_iDisinfectIntervalDays = DISINFECT_INTERVAL_DAYS_DEFAULT; // days
    uint8_t m_iDisinfectRunTimeMinutes = DISINFECT_RUN_TIME_MINUTES_DEFAULT; // minutes
    uint8_t m_iDisinfectTemperature = DISINFECT_TEMPERATURE_DEFAULT; // C

    float m_fTemperature = -1.0f;
    bool m_bDisinfectionRequired = false;
};
#endif // LEGIONELLA_H
