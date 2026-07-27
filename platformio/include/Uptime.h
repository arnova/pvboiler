#pragma once
#ifndef UPTIME_H
#define UPTIME_H

#include <Arduino.h>

class CUptime
{
  public:
    struct uptime_s
    {
      uint8_t iSeconds;
      uint8_t iMinutes;
      uint8_t iHours;
      uint16_t iDays;
      uint8_t iYears;
    };
    typedef struct uptime_s uptime_t;

    void Update();
    uptime_t GetBreakdown() const;

    uint32_t Get() const { return m_iSeconds; }

  private:
    uint32_t m_iLastMillis = 0;
    uint32_t m_iAccumMs    = 0;
    uint32_t m_iSeconds    = 0;
};
#endif // UPTIME_H