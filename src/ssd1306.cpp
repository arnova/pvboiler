/* 
  SSD1306 Display Class
  (C) Copyright 2025

  Written by       : Arno van Amersfoort
  Dependencies     : util, U8g2lib
  Initial date     : October 14, 2025
  Last modified    : May 19, 2025
*/

#include "ssd1306.h"
#include "util.h"
#include "system.h"

#define FONT_VERTICAL_SPACING 22

U8G2_SSD1306_128X64_NONAME_F_SW_I2C g_u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

void CSSD1306::WriteDisplayStr(const char* strLine, const uint8_t iLine /* = 0 */, const bool bClear /* = true */)
{
  if (bClear)
    g_u8g2.clearBuffer();                   // clear the internal memory

  g_u8g2.drawStr(0, FONT_VERTICAL_SPACING + (iLine * (FONT_VERTICAL_SPACING + 15)), strLine);
  g_u8g2.sendBuffer();
}


void CSSD1306::SetScrollText1(const char* strText)
{
  strcpy(m_strScrollText1, "        ");
  strcat(m_strScrollText1, strText);
  strcat(m_strScrollText1, "        ");
}


void CSSD1306::SetScrollText2(const char* strText)
{
  strcpy(m_strScrollText2, "        ");
  strcat(m_strScrollText2, strText);
  strcat(m_strScrollText2, "        ");
}


void CSSD1306::SetPowerValue()
{
  if (isnan(m_iPower))
  {
    SetScrollText1("No value");
  }
  else
  {
    String strValue = "P=" + String(m_iPower) + "W " + String(m_iPercentage) + "%";

    SetScrollText1(strValue.c_str());
  }
}


void CSSD1306::Init()
{
  // Init display
  g_u8g2.begin();
  g_u8g2.setFont(u8g2_font_inb19_mf);

  WriteDisplayStr("PVBoiler");
  WriteDisplayStr("v" MY_VERSION, 1, false);
}


void CSSD1306::Loop()
{
  if (m_iScrollTimer++ > 1)
  {
    m_iScrollTimer = 0;
  }
  else
  {
    return;
  }

  // Handle scrolling
  if (m_iScrollCount1 >= strlen(m_strScrollText1))
  {
    m_iScrollCount1 = 0;
    // Set value
    SetPowerValue();
  }
  else
  {
    WriteDisplayStr(m_strScrollText1 + m_iScrollCount1, 0, false);
    m_iScrollCount1++;
  }

  // Handle scrolling
  if (m_iScrollCount2 >= strlen(m_strScrollText2))
  {
    m_iScrollCount2 = 0;
  }
  else
  {
    WriteDisplayStr(m_strScrollText2 + m_iScrollCount2, 1, false);
    m_iScrollCount2++;
  }
}
