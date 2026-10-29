#pragma once
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "CommandParser.h"

#include <IPAddress.h>


class CCommandHandler
{
  public:
    CCommandHandler() {};   // Empty ctor
    ~CCommandHandler() {};  // Empty dtor

    bool GetDebugEnabled(void) const { return m_bDebug; };

  protected:
    static void PrintItemValue(const char* strItem, const char* strValue, const char* strSuffix = nullptr);
    static void PrintItemValue(const char* strItem, const bool bValue);
    // NOTE: Overloads for all int types so (u)int8/16/32_t args resolve unambiguously (int32_t is int or long, depending on toolchain)
    static void PrintItemValue(const char* strItem, const int iValue, const char* strSuffix = nullptr) { PrintItemValue(strItem, static_cast<long>(iValue), strSuffix); };
    static void PrintItemValue(const char* strItem, const unsigned int iValue, const char* strSuffix = nullptr) { PrintItemValue(strItem, static_cast<unsigned long>(iValue), strSuffix); };
    static void PrintItemValue(const char* strItem, const long iValue, const char* strSuffix = nullptr);
    static void PrintItemValue(const char* strItem, const unsigned long iValue, const char* strSuffix = nullptr);
    static void PrintItemValue(const char* strItem, const float fValue, const uint8_t iDecimals, const char* strSuffix = nullptr);
    static void PrintItemSecret(const char* strItem, const char* strValue); // Value only shown on serial terminal (or socket terminal when TERMINAL_PASSWORD login is enabled)
    static void PrintItemIp(const char* strItem, const IPAddress& ipAddr);

    result_code_t CmdEchoOnOff(const char *strArgs);
    result_code_t CmdDebugOnOff(const char *strArgs);

  protected:
    bool m_bDebug = false;
};

#endif // COMMAND_HANDLER_H
