/* 
  Uptime Class
  (C) Copyright 2026

  Written by       : Arno van Amersfoort
  Dependencies     : (none)
  Initial date     : July 27, 2026
  Last modified    : July 27, 2026
*/

#include "Uptime.h"

void CUptime::Update()
{
  uint32_t now = millis();
  m_iAccumMs += now - m_iLastMillis;
  m_iLastMillis = now;

  m_iSeconds += m_iAccumMs / 1000;
  m_iAccumMs %= 1000;
}


CUptime::uptime_t CUptime::GetBreakdown() const
{
  uint32_t iSeconds = m_iSeconds;

  uptime_t upTime;
  upTime.iYears    = iSeconds / 31536000;  iSeconds %= 31536000;   // 365-day year
  upTime.iDays     = iSeconds / 86400;     iSeconds %= 86400;
  upTime.iHours    = iSeconds / 3600;      iSeconds %= 3600;
  upTime.iMinutes  = iSeconds / 60;        iSeconds %= 60;
  upTime.iSeconds  = iSeconds;

  return upTime;
}
