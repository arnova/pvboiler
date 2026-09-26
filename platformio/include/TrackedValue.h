#pragma once

#include <Arduino.h>

// Running average factor: each new reading counts for 1/TRACKED_VALUE_AVERAGE_DIVISOR.
// Preferably a power of two to keep the division cheap in the ISR
#define TRACKED_VALUE_AVERAGE_DIVISOR 16

// Averaged measurement, safe for use in an ISR:
// - In-range readings update a running average (the first one seeds it)
// - Out-of-range readings are rejected
// - Too many consecutive rejected readings invalidate the average
class CTrackedValue
{
public:
  enum result_t
  {
    RESULT_ACCEPTED,  // Reading accepted, average updated
    RESULT_OUTLIER,   // Reading rejected, average kept
    RESULT_RESET      // Reading rejected, no valid average (anymore)
  };

  CTrackedValue(const uint32_t iMin, const uint32_t iMax, const uint8_t iMaxOutliers);

  // Used from ISR
  result_t IRAM_ATTR Update(const uint32_t iValue);
  void IRAM_ATTR Reset(const uint32_t iBadValue);
  uint32_t IRAM_ATTR Get() const;

  // Used outside ISR
  bool IsValid() const;
  uint32_t GetLastBadValue() const;
  uint32_t GetLowest() const;
  uint32_t GetHighest() const;

private:
  const uint32_t m_iMin;
  const uint32_t m_iMax;
  const uint8_t m_iMaxOutliers;

  volatile bool m_bValid = false;
  volatile uint32_t m_iValue = 0;
  uint8_t m_iOutlierCount = 0; // ISR only

  volatile uint32_t m_iLastBadValue = 0;
  volatile uint32_t m_iLowest = UINT32_MAX;
  volatile uint32_t m_iHighest = 0;
};
