/*
  Rolling average library
  (C) Copyright 2018-2026, Arno van Amersfoort

  Written by       : Arno van Amersfoort
  Language         : C++11
  Target compiler  : Generic
  Dependencies     : (none)
  Initial date     : March 23, 2018
  Last modified    : September 21, 2026
*/

#include "RollingAverage.h"

void CRollingAverage::SetAvgCount(uint32_t iCount)
{
  if (iCount < 1)
  {
    iCount = 1;
  }

  // Preserve current average in case new count is lower than previous one
  if (iCount < m_iAvgCountSet)
  {
    float fAvgVal = m_fAccVal / m_iAccCount;
    m_iAccCount = iCount;
    m_fAccVal = fAvgVal * m_iAccCount;
  }

  m_iAvgCountSet = iCount;
}


float CRollingAverage::GetValue()
{
  if (m_iAccCount == 0)
  {
    return 0.0f; // No value
  }

  return m_fAccVal / m_iAccCount;
}


float CRollingAverage::UpdateValue(const float fNewVal)
{
  float fAccVal = (m_iAccCount == 0 ? 0.0f : m_fAccVal);

  if (m_iAccCount == m_iAvgCountSet)
  {
    fAccVal -= (fAccVal / m_iAvgCountSet);
  }
  else
  {
    m_iAccCount++;
  }

  m_fAccVal = fAccVal + fNewVal; // Update value

  return GetValue();
}


float CRollingAverage::RemoveValue()
{
  if (m_iAccCount <= 1)
  {
    m_fAccVal = 0.0f;
    m_iAccCount = 0;
  }
  else
  {
    m_fAccVal -= (m_fAccVal / m_iAccCount);
    m_iAccCount--;
  }

  return GetValue();
}
