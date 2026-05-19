#ifndef SSD1306_H
#define SSD1306_H

#include "system.h"

#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
class CSSD1306
{
  public:
    CSSD1306() {}; // Empty constructor
    ~CSSD1306() {}; // Empty destructor

    void Init();
    void Loop();
    void SetPowerValue(const float fVal) { m_iPower = fVal; };
    void SetInfo(const char* strInfo) { strcpy(m_strInfo, strInfo); };

  private:
    void WriteDisplayStr(const char* strLine, const uint8_t iLine = 0, const bool bClear = true);
    void SetScrollText1(const char* strText);
    void SetScrollText2(const char* strText);
    void SetPowerValue();

    uint16_t m_iPower = 0;
    uint8_t m_iPercentage = 0;
    uint8_t m_iScrollTimer = 0;
    char m_strScrollText1[40] = {0};
    char m_strScrollText2[64] = {0};
    char m_strInfo[32] = {0};
    uint8_t m_iScrollCount1 = 0;
    uint8_t m_iScrollCount2 = 0;
};
#endif // SSD1306_H