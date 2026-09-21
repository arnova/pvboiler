#pragma once
#ifndef LEGIONELLA_H
#define LEGIONELLA_H

#include <elapsedMillis.h>

#include "system.h"

#ifndef DANGER_LOW_TEMPERATURE
#define DANGER_LOW_TEMPERATURE  20.0f
#endif

#ifndef DANGER_HIGH_TEMPERATURE
#define DANGER_HIGH_TEMPERATURE 45.0f
#endif

#ifndef DISINFECT_TIMEOUT
#define DISINFECT_TIMEOUT       7     // Days
#endif

#ifndef DISINFECT_RUN_TIME
#define DISINFECT_RUN_TIME      15    // Minutes
#endif

#ifndef DISINFECT_TEMPERATURE
#define DISINFECT_TEMPERATURE   60.0f // Celcius
#endif


class CLegionella
{
  public:
    enum state_e
    {
      STATE_SAFE = 0,
      STATE_DISINFECT
    };
    typedef enum state_e state_t;

    CLegionella() {}; // Empty ctor
    ~CLegionella() {}; // Empty dtor

    void Reset() { m_timeSinceLastDisinfect = 0; m_timeDisinfectRun = 0; m_bMustDisinfect = false; };
    void Loop();
    void UpdateTemperature(const float fTemperature) { m_fTemperature = fTemperature; };
    bool MustDisinfect() { return m_bMustDisinfect; };

    const uint32_t GetTimeSinceLastDisinfect() { return m_timeSinceLastDisinfect; };
    const uint32_t GetDisinfectRunTime() { return m_timeDisinfectRun; };

  private:
    elapsedSeconds m_timeSinceLastDisinfect; // s
    elapsedSeconds m_timeDisinfectRun; // s
    elapsedSeconds m_timeInDangerZone; // s
    elapsedSeconds m_timeBelowDangerZone; // s
    elapsedSeconds m_timeAboveDangerZone; // s
    float m_fTemperature = -1.0f;
    bool m_bMustDisinfect = false;
    state_t iState;
};
#endif // LEGIONELLA_H
