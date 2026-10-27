#pragma once
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "CommandParser.h"


class CCommandHandler
{
  public:
    CCommandHandler() {};   // Empty ctor
    ~CCommandHandler() {};  // Empty dtor

    static result_code_t FormatIP(uint8_t *data, char *strResult);

    bool GetDebugEnabled(void) const { return m_bDebug; };

  protected:
    static void PrintItemValue(const char* strItem, const char* strValue);
    static void PrintItemValue(const char* strItem, const bool bValue);
    static void PrintItemSecret(const char* strItem, const char* strValue); // Value only shown on serial terminal (or socket terminal when TERMINAL_PASSWORD login is enabled)
    static void PrintItemIp(const char* strItem, const uint8_t* ipAddr);

    result_code_t CmdEchoOnOff(const char *strArgs);
    result_code_t CmdDebugOnOff(const char *strArgs);

  protected:
    bool m_bDebug = false;
};

#endif // COMMAND_HANDLER_H
