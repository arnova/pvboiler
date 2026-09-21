#include "Legionella.h"

void CLegionella::Loop()
{
  if (m_fTemperature < DISINFECT_TEMPERATURE)
  {
    m_timeDisinfectRun = 0;
  }
  else if (m_timeDisinfectRun > DISINFECT_RUN_TIME * 60)
  {
    m_timeSinceLastDisinfect = 0;
    m_bMustDisinfect = false;
  }

  if (m_fTemperature > DANGER_LOW_TEMPERATURE && m_fTemperature < DANGER_HIGH_TEMPERATURE)
  {
    
  }
  else
  {
    if (m_timeInDangerZone != 0)
    {
      // FIXME: Check time spend in danger zone & time since last time in danger zone (perhaps use a credit counting system)?
    }

    m_timeInDangerZone = 0;
  }

  switch (iState)
  {
    case STATE_SAFE:
    {
      if (m_timeSinceLastDisinfect > DISINFECT_TIMEOUT * 24 * 3600)
      {
        m_timeDisinfectRun = 0;
        m_bMustDisinfect = true;
        iState = STATE_DISINFECT;
      }
    }
    break;

    case STATE_DISINFECT:
    {
    }
    break;
  }
}