/*
  EEPROM Settings Class
  (C) Copyright 2026

  Written by       : Arno van Amersfoort
  Dependencies     : EEPROM
  Initial date     : October 23, 2026
  Last modified    : October 26, 2026
*/

#include <EEPROM.h>
#include <string.h>

#include "EepromSettings.h"


// Write EEPROM buffer to flash. Note: Only needed on ESP platforms where EEPROM is emulated in flash
void CEepromSettings::Commit()
{
#if defined(ESP8266) || defined(ESP32)
  EEPROM.commit();
#endif
}


bool CEepromSettings::IsPrintable(const char* str)
{
  for (; *str; ++str)
  {
    if (*str < 32 || *str > 126)
    {
      return false;
    }
  }

  return true;
}


void CEepromSettings::Load(const int iAddr, uint8_t& iSetting, const uint8_t iMin, const uint8_t iMax, const uint8_t iDefault)
{
  EEPROM.get(iAddr, iSetting);
  if (iSetting < iMin || iSetting > iMax)
  {
    iSetting = iDefault;
  }
}


void CEepromSettings::Load(const int iAddr, uint16_t& iSetting, const uint16_t iMin, const uint16_t iMax, const uint16_t iDefault)
{
  EEPROM.get(iAddr, iSetting);
  if (iSetting < iMin || iSetting > iMax)
  {
    iSetting = iDefault;
  }
}


void CEepromSettings::Load(const int iAddr, float& fSetting, const float fMin, const float fMax, const float fDefault)
{
  EEPROM.get(iAddr, fSetting);
  if (fSetting < fMin || fSetting > fMax || isnan(fSetting))
  {
    fSetting = fDefault;
  }
}


void CEepromSettings::Load(const int iAddr, bool& bSetting, const bool bDefault)
{
  uint8_t iVal = 0;
  EEPROM.get(iAddr, iVal);
  if (iVal == 0x00)
  {
    bSetting = false;
  }
  else if (iVal == 0x01)
  {
    bSetting = true;
  }
  else
  {
    bSetting = bDefault;
  }
}


void CEepromSettings::LoadString(const int iAddr, char* strSetting, const size_t iMaxSize)
{
  for (size_t it = 0; it < iMaxSize; it++)
  {
    strSetting[it] = EEPROM.read(iAddr + it);
  }
  strSetting[iMaxSize] = '\0'; // Always null terminate

  if (!IsPrintable(strSetting))
  {
    strSetting[0] = '\0';
  }
}


void CEepromSettings::LoadIp(const int iAddr, uint8_t* ipSetting)
{
  bool bErased = true;
  for (uint8_t it = 0; it < EEPROM_IP_SIZE; it++)
  {
    ipSetting[it] = EEPROM.read(iAddr + it);
    if (ipSetting[it] != 0xFF)
    {
      bErased = false;
    }
  }

  // Erased EEPROM: no address set
  if (bErased)
  {
    memset(ipSetting, 0x00, EEPROM_IP_SIZE);
  }
}


bool CEepromSettings::Store(const int iAddr, uint8_t& iSetting, const uint8_t iVal)
{
  if (iVal == iSetting)
    return false;

  EEPROM.put(iAddr, iVal);
  Commit();

  iSetting = iVal;

  return true;
}


bool CEepromSettings::Store(const int iAddr, uint16_t& iSetting, const uint16_t iVal)
{
  if (iVal == iSetting)
    return false;

  EEPROM.put(iAddr, iVal);
  Commit();

  iSetting = iVal;

  return true;
}


bool CEepromSettings::Store(const int iAddr, float& fSetting, const float fVal)
{
  if (fVal == fSetting)
    return false;

  EEPROM.put(iAddr, fVal);
  Commit();

  fSetting = fVal;

  return true;
}


bool CEepromSettings::Store(const int iAddr, bool& bSetting, const bool bVal)
{
  if (bVal == bSetting)
    return false;

  EEPROM.put(iAddr, static_cast<uint8_t>(bVal ? 0x01 : 0x00));
  Commit();

  bSetting = bVal;

  return true;
}


bool CEepromSettings::StoreString(const int iAddr, char* strSetting, const size_t iMaxSize, const char* strVal)
{
  if (strcmp(strVal, strSetting) == 0)
    return false;

  memset(strSetting, 0x00, iMaxSize + 1);
  strcpy(strSetting, strVal);

  for (size_t it = 0; it <= iMaxSize; it++)
  {
    EEPROM.write(iAddr + it, strSetting[it]);
  }
  Commit();

  return true;
}


bool CEepromSettings::StoreIp(const int iAddr, uint8_t* ipSetting, const uint8_t* ipVal)
{
  if (memcmp(ipVal, ipSetting, EEPROM_IP_SIZE) == 0)
    return false;

  memcpy(ipSetting, ipVal, EEPROM_IP_SIZE);

  for (uint8_t it = 0; it < EEPROM_IP_SIZE; it++)
  {
    EEPROM.write(iAddr + it, ipSetting[it]);
  }
  Commit();

  return true;
}


void CEepromSettings::Erase(const int iAddr, const size_t iSize)
{
  for (size_t it = 0; it < iSize; it++)
  {
    EEPROM.write(iAddr + it, 0xFF);
  }
  Commit();
}
