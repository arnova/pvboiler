/* 
  SSD1306 Display Class
  (C) Copyright 2025-2026

  Written by       : Arno van Amersfoort
  Dependencies     : util, U8g2lib
  Initial date     : October 14, 2025
  Last modified    : October 19, 2026
*/

#include "Ssd1306.h"
#include "util.h"
#include "system.h"

#define FONT_VERTICAL_SPACING 20

#ifndef U8G2_FONT
#define U8G2_FONT u8g2_font_helvB12_tf
#endif

// Fallback fonts (largest first) used when a string doesn't fit the display width
static const uint8_t* const g_fallbackFonts[] = { u8g2_font_helvB10_tf, u8g2_font_helvB08_tf };

U8G2_SSD1306_128X64_NONAME_F_SW_I2C g_u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

void CSsd1306::Init()
{
  // Init display
  g_u8g2.begin();
  g_u8g2.setFont(U8G2_FONT);
}


void CSsd1306::WriteDisplayStr(const char* strLine, const uint8_t iLine /* = 0 */, const bool bClear /* = true */)
{
  if (bClear)
    g_u8g2.clearBuffer();                   // clear the internal memory

  // Switch to a smaller font if the string doesn't fit
  const int iDisplayWidth = g_u8g2.getDisplayWidth();
  int iStrWidth = g_u8g2.getStrWidth(strLine);
  for (uint8_t iFont = 0; iStrWidth > iDisplayWidth && iFont < (sizeof(g_fallbackFonts) / sizeof(g_fallbackFonts[0])); iFont++)
  {
    g_u8g2.setFont(g_fallbackFonts[iFont]);
    iStrWidth = g_u8g2.getStrWidth(strLine);
  }

  // Center the text to display (left align if it still doesn't fit)
  const int iX = (iStrWidth < iDisplayWidth) ? ((iDisplayWidth - iStrWidth) / 2) : 0;
  g_u8g2.drawStr(iX, 12 + (iLine * FONT_VERTICAL_SPACING), strLine);
  g_u8g2.sendBuffer();

  g_u8g2.setFont(U8G2_FONT); // Restore default font
}
