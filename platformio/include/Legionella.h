#pragma once
#ifndef LEGIONELLA_H
#define LEGIONELLA_H

#include "system.h"

// Maximum hours that fit in our (uint32_t) seconds counters
#define LEGIONELLA_HOURS_MAX (UINT32_MAX / 3600)

class CLegionella
{
  public:
    CLegionella() {}; // Empty ctor
    ~CLegionella() {}; // Empty dtor

    void Reset() { m_iDisinfectionAgeSeconds = 0; m_iDisinfectingElapsedSeconds = 0; m_iInDangerZoneSeconds = 0; m_bDisinfectionRequired = false; };
    void Loop();

    const uint32_t GetDisinfectionAgeHours() const { return m_iDisinfectionAgeSeconds / 3600; };
    const uint32_t GetDisinfectingElapsedMinutes() const { return (m_iDisinfectingElapsedSeconds + 59) / 60; }; // Round up so non-zero means disinfecting
    const uint32_t GetInDangerZoneHours() const { return m_iInDangerZoneSeconds / 3600; };
    bool DisinfectionRequired() const { return m_bDisinfectionRequired; };

    void UpdateTemperature(const float fTemperature) { m_fTemperature = fTemperature; };
    void SetDisinfectionAgeHours(const uint32_t iHours) { m_iDisinfectionAgeSeconds = ((iHours > LEGIONELLA_HOURS_MAX) ? LEGIONELLA_HOURS_MAX : iHours) * 3600; };
    void SetInDangerZoneHours(const uint32_t iHours) { m_iInDangerZoneSeconds = ((iHours > LEGIONELLA_HOURS_MAX) ? LEGIONELLA_HOURS_MAX : iHours) * 3600; };

    void SetDangerZoneLowTemperature(const uint8_t iTemperature) { m_iDangerZoneLowTemperature = iTemperature; };
    void SetDangerZoneHighTemperature(const uint8_t iTemperature) { m_iDangerZoneHighTemperature = iTemperature; };
    void SetDisinfectionIntervalDays(const uint8_t iDays) { m_iDisinfectionIntervalDays = iDays; };
    void SetDisinfectionDurationMinutes(const uint8_t iMinutes) { m_iDisinfectionDurationMinutes = iMinutes; };
    void SetDisinfectionTemperature(const uint8_t iTemperature) { m_iDisinfectionTemperature = iTemperature; };

  private:
    uint32_t m_iDisinfectionAgeSeconds = 0; // s
    uint32_t m_iDisinfectingElapsedSeconds = 0; // s
    uint32_t m_iInDangerZoneSeconds = 0; // s

    uint8_t m_iDangerZoneLowTemperature = DANGER_ZONE_LOW_TEMPERATURE_DEFAULT; // C
    uint8_t m_iDangerZoneHighTemperature = DANGER_ZONE_HIGH_TEMPERATURE_DEFAULT; // C
    uint8_t m_iDisinfectionIntervalDays = DISINFECTION_INTERVAL_DAYS_DEFAULT; // days
    uint8_t m_iDisinfectionDurationMinutes = DISINFECTION_DURATION_MINUTES_DEFAULT; // minutes
    uint8_t m_iDisinfectionTemperature = DISINFECTION_TEMPERATURE_DEFAULT; // C

    float m_fTemperature = -1.0f;
    bool m_bDisinfectionRequired = false;
};
#endif // LEGIONELLA_H
