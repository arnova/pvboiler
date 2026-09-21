#include "Legionella.h"

void CLegionella::Loop()
{
  m_iSecondsPassedSinceLastDisinfect++;

  if (m_fTemperature > DISINFECT_TEMPERATURE)
  {
    if (m_iDisinfectRuntimeSeconds >= DISINFECT_RUN_TIME_MINUTES * 60)
    {
      m_bMustDisinfect = false;
      m_iSecondsInDangerZone = 0;
      m_iSecondsPassedSinceLastDisinfect = 0;
    }
    else
    {
      m_iDisinfectRuntimeSeconds++;
    }
  }
  else
  {
    m_iDisinfectRuntimeSeconds = 0;
  }

  if (m_fTemperature >= DANGER_ZONE_LOW_TEMPERATURE && m_fTemperature <= DANGER_ZONE_HIGH_TEMPERATURE)
  {
    if (m_iSecondsInDangerZone >= DANGER_ZONE_DWELL_LIMIT_DAYS * 3600 * 24)
    {
      m_bMustDisinfect = true;
    }
    else
    {
      m_iSecondsInDangerZone++;
    }
  }

  // FIXME: Perhaps when below DANGER_ZONE_LOW_TEMPERATURE also check absolute time since last
  // run + time spend in danger zone to detect whether we got e.g. flushed. When flush detected reset m_iSecondsInDangerZone
}