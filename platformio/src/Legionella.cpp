#include "Legionella.h"

void CLegionella::Loop()
{
  m_iDisinfectionAgeSeconds++;

  if (m_fTemperature > m_iDisinfectionTemperature)
  {
    if (++m_iDisinfectingElapsedSeconds >= m_iDisinfectionDurationMinutes * 60UL)
    {
      m_bDisinfectionRequired = false;
      m_iInDangerZoneSeconds = 0;
      m_iDisinfectionAgeSeconds = 0;
    }
  }
  else
  {
    m_iDisinfectingElapsedSeconds = 0;
  }

  if (m_fTemperature >= m_iDangerZoneLowTemperature && m_fTemperature <= m_iDangerZoneHighTemperature)
  {

    if (++m_iInDangerZoneSeconds >= m_iDisinfectionIntervalDays * 3600UL * 24)
    {
      m_bDisinfectionRequired = true;
    }
  }

  // FIXME: Perhaps when below danger zone low temperature also check absolute time since last
  // run + time spend in danger zone to detect whether we got e.g. flushed. When flush detected reset m_iInDangerZoneSeconds
}