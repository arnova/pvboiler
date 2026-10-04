#pragma once
#ifndef LEGIONELLA_H
#define LEGIONELLA_H

#include "system.h"

#ifndef DANGER_ZONE_LOW_TEMPERATURE
#define DANGER_ZONE_LOW_TEMPERATURE   20.0f // Celsius. FIXME: Should be configurable
#endif

#ifndef DANGER_ZONE_HIGH_TEMPERATURE
#define DANGER_ZONE_HIGH_TEMPERATURE  45.0f // Celsius. FIXME: Should be configurable
#endif

#ifndef DANGER_ZONE_DWELL_LIMIT_DAYS
#define DANGER_ZONE_DWELL_LIMIT_DAYS  7     // Days. FIXME: Should be configurable
#endif

#ifndef DISINFECT_RUN_TIME_MINUTES
#define DISINFECT_RUN_TIME_MINUTES    15    // Minutes. FIXME: Should be configurable
#endif

#ifndef DISINFECT_TEMPERATURE
#define DISINFECT_TEMPERATURE         60.0f // Celcius. FIXME: Should be configurable
#endif


class CLegionella
{
  public:
    CLegionella() {}; // Empty ctor
    ~CLegionella() {}; // Empty dtor

    void Reset() { m_iSecondsPassedSinceLastDisinfect = 0; m_iDisinfectRuntimeSeconds = 0; m_iSecondsInDangerZone = 0; m_bMustDisinfect = false; };
    void Loop();

    const uint32_t GetHoursPassedSinceLastDisinfect() const { return m_iSecondsPassedSinceLastDisinfect / 3600; };
    const uint32_t GetDisinfectRunTimeSeconds() const { return m_iDisinfectRuntimeSeconds; };
    const uint32_t GetHoursInDangerZone() const { return m_iSecondsInDangerZone / 3600; };
    bool MustDisinfect() const { return m_bMustDisinfect; };

    void UpdateTemperature(const float fTemperature) { m_fTemperature = fTemperature; };
    void SetHoursPassedSinceLastDisinfect(const uint32_t iHours) { m_iSecondsPassedSinceLastDisinfect = iHours * 3600; };
    void SetHoursInDangerZone(const uint32_t iHours) { m_iSecondsInDangerZone = iHours * 3600; };

  private:
    uint32_t m_iSecondsPassedSinceLastDisinfect; // s
    uint32_t m_iDisinfectRuntimeSeconds; // s
    uint32_t m_iSecondsInDangerZone; // s

    float m_fTemperature = -1.0f;
    bool m_bMustDisinfect = false;
};
#endif // LEGIONELLA_H
