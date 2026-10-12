#include "Legionella.h"

void CLegionella::Loop()
{
  m_iSecondsPassedSinceLastDisinfect++;

  if (m_fTemperature > m_iDisinfectTemperature)
  {
    if (++m_iDisinfectRuntimeSeconds >= m_iDisinfectRunTimeMinutes * 60UL)
    {
      m_bDisinfectionRequired = false;
      m_iSecondsInDangerZone = 0;
      m_iSecondsPassedSinceLastDisinfect = 0;
    }
  }
  else
  {
    m_iDisinfectRuntimeSeconds = 0;
  }

  if (m_fTemperature >= m_iDangerZoneLowTemperature && m_fTemperature <= m_iDangerZoneHighTemperature)
  {

    if (++m_iSecondsInDangerZone >= m_iDisinfectIntervalDays * 3600UL * 24)
    {
      m_bDisinfectionRequired = true;
    }
  }

  // FIXME: Perhaps when below danger zone low temperature also check absolute time since last
  // run + time spend in danger zone to detect whether we got e.g. flushed. When flush detected reset m_iSecondsInDangerZone
}