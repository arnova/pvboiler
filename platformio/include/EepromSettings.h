#pragma once
#ifndef EEPROM_SETTINGS_H
#define EEPROM_SETTINGS_H

#include <Arduino.h>

// Size of a stored IPv4 address
#define EEPROM_IP_SIZE 4

class CEepromSettings
{
  public:
    // Load setting, use default when out of range (or NaN)
    static void Load(const int iAddr, uint8_t& iSetting, const uint8_t iMin, const uint8_t iMax, const uint8_t iDefault);
    static void Load(const int iAddr, uint16_t& iSetting, const uint16_t iMin, const uint16_t iMax, const uint16_t iDefault);
    static void Load(const int iAddr, float& fSetting, const float fMin, const float fMax, const float fDefault);
    static void Load(const int iAddr, bool& bSetting, const bool bDefault); // 0x00 = false, 0x01 = true, else default
    static void LoadString(const int iAddr, char* strSetting, const size_t iMaxSize); // Cleared when not printable
    static void LoadIp(const int iAddr, uint8_t* ipSetting); // Erased (255.255.255.255) becomes 0.0.0.0

    // Store setting (only when changed). Returns true when changed
    static bool Store(const int iAddr, uint8_t& iSetting, const uint8_t iVal);
    static bool Store(const int iAddr, uint16_t& iSetting, const uint16_t iVal);
    static bool Store(const int iAddr, float& fSetting, const float fVal);
    static bool Store(const int iAddr, bool& bSetting, const bool bVal);
    static bool StoreString(const int iAddr, char* strSetting, const size_t iMaxSize, const char* strVal);
    static bool StoreIp(const int iAddr, uint8_t* ipSetting, const uint8_t* ipVal);

    // Erase range (fill with 0xFF)
    static void Erase(const int iAddr, const size_t iSize);

    static bool IsPrintable(const char* str);

  private:
    static void Commit();
};
#endif // EEPROM_SETTINGS_H
