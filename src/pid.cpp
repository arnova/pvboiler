#include "pid.h"

float CPid::UpdateValue(const float& fCurVal, const float& fDt /* = 1.0f */)
{
  const float fErrVal = m_fSetPoint - fCurVal;

  // Calculate p-term
  const float fPterm = m_fKp * fErrVal;

  // Add error value to i accumulator
  m_fIacc += (fErrVal * fDt);

  // Calculate i-term
  const float fIterm = m_fKi * m_fIacc;

  // Calculate d-term
  const float fDerative = (fErrVal - m_fLastErrVal) / fDt;
  const float fDterm = m_fKd * fDerative;

  // Calculate output value
  m_fOutValue = fPterm + fIterm + fDterm;

  // Check min/max clamps
  m_bClamped = false;
#ifdef OUT_CLAMP_MAX
  if (m_fOutValue > OUT_CLAMP_MAX)
  {
    m_fOutValue = OUT_CLAMP_MAX;
    m_bClamped = true;
  }
#endif

#ifdef OUT_CLAMP_MIN
  if (m_fOutValue < OUT_CLAMP_MIN)
  {
    m_fOutValue = OUT_CLAMP_MIN;
    m_bClamped = true;
  }
#endif

  if (m_bClamped && m_fKi != 0.0f)
  {
    // (Back) calculate i-acc value
    m_fIacc = (m_fOutValue - fPterm - fDterm) / m_fKi;
  }

  // Update last error value
  m_fLastErrVal = fErrVal;

  return m_fOutValue;
}
