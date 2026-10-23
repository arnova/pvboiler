/*
  Command Handler Class

  Written by       : Arno van Amersfoort
  Dependencies     : CommandParser, Terminal, util
  Initial date     : August 22, 2019
  Last modified    : October 22, 2026
*/

#include "CommandHandler.h"
#include "system.h"
#include "util.h"
#include "Terminal.h"

#include <string.h>
#include <Arduino.h>


// Helper function to format IP Address
result_code_t CCommandHandler::FormatIP(uint8_t *data, char *strResult)
{
  strResult[0] = '\0';
  for (uint8_t i = 0; i < 4; i++)
  {
    char strDec[4];
    strcat(strResult, uint32_to_decstr((uint32_t)data[i], strDec, sizeof(strDec)));
    if (i < 3)
      strcat(strResult, ".");
  }
  strcat(strResult, "\n\r");

  return pack_result_code(ERR_CODE_OK);
}


void CCommandHandler::PrintItemValue(const char* strItem, const char* strValue)
{
  CTerminal::print(strItem);
  CTerminal::print("=");
  CTerminal::print(strValue);
}


void CCommandHandler::PrintItemValue(const char* strItem, const bool bValue)
{
  PrintItemValue(strItem, bValue ? "1" : "0");
}


void CCommandHandler::PrintItemIp(const char* strItem, const uint8_t* ipAddr)
{
  char strBuf[16]; // Enough room for "255.255.255.255"
  snprintf(strBuf, sizeof(strBuf), "%u.%u.%u.%u", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
  PrintItemValue(strItem, strBuf);
}


// Handle debug on/off
result_code_t CCommandHandler::CmdDebugOnOff(const char *strArgs)
{
  bool bOnOff;
  const result_code_t result = parse_bool_arg(strArgs, &bOnOff);
  if (result.code != ERR_CODE_OK)
    return result;

  m_bDebug = bOnOff;

  return pack_result_code(ERR_CODE_OK);
}


// Handle local echo on/off
result_code_t CCommandHandler::CmdEchoOnOff(const char *strArgs)
{
  bool bOnOff;
  const result_code_t result = parse_bool_arg(strArgs, &bOnOff);
  if (result.code != ERR_CODE_OK)
    return result;

  CTerminal::SetLocalEcho(bOnOff);

  return pack_result_code(ERR_CODE_OK);
}
