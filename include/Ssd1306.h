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

class CSsd1306
{
  public:
    CSsd1306() {}; // Empty constructor
    ~CSsd1306() {}; // Empty destructor

    void Init();

    void WriteDisplayStr(const char* strLine, const uint8_t iLine = 0, const bool bClear = true);
};
#endif // SSD1306_H