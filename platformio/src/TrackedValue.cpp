#include "TrackedValue.h"

CTrackedValue::CTrackedValue(const uint32_t iMin, const uint32_t iMax, const uint8_t iMaxOutliers)
  : m_iMin(iMin), m_iMax(iMax), m_iMaxOutliers(iMaxOutliers)
{
}


CTrackedValue::result_t IRAM_ATTR CTrackedValue::Update(const uint32_t iValue)
{
  // Debug statistics over all readings, including rejected ones
  if (iValue < m_iLowest)
  {
    m_iLowest = iValue;
  }

  if (iValue > m_iHighest)
  {
    m_iHighest = iValue;
  }

  if (iValue < m_iMin || iValue > m_iMax)
  {
    if (!m_bValid)
    {
      return RESULT_RESET; // Still no valid average
    }

    if (++m_iOutlierCount >= m_iMaxOutliers)
    {
      Reset(iValue);
      return RESULT_RESET;
    }

    return RESULT_OUTLIER;
  }

  if (!m_bValid)
  {
    m_iValue = iValue; // Seed average
    m_bValid = true;
  }
  else
  {
    m_iValue += (static_cast<int32_t>(iValue) - static_cast<int32_t>(m_iValue)) / TRACKED_VALUE_AVERAGE_DIVISOR; // Running average
  }

  m_iOutlierCount = 0;
  return RESULT_ACCEPTED;
}


// Invalidate average, remember the value that caused it for logging
void IRAM_ATTR CTrackedValue::Reset(const uint32_t iBadValue)
{
  m_iLastBadValue = iBadValue;
  m_bValid = false;
  m_iOutlierCount = 0;
}


uint32_t IRAM_ATTR CTrackedValue::Get() const
{
  return m_iValue;
}


bool CTrackedValue::IsValid() const
{
  return m_bValid;
}


uint32_t CTrackedValue::GetLastBadValue() const
{
  return m_iLastBadValue;
}


uint32_t CTrackedValue::GetLowest() const
{
  return m_iLowest;
}


uint32_t CTrackedValue::GetHighest() const
{
  return m_iHighest;
}
