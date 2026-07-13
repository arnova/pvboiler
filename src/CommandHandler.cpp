#include "CommandHandler.h"
#include "util.h"
#include "TermPrint.h"
#include "system.h"

#include <string.h>
#include <Arduino.h>


result_code_t CCommandHandler::CmdReboot(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  ESP.restart();

  return pack_result_code(ERR_CODE_OK);
}


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


// Handle local echo on/off
result_code_t CCommandHandler::CmdEchoOnOff(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (STRIEQUALS(strArgs, "on"))
    m_bLocalEcho = true;
  else if (STRIEQUALS(strArgs, "off"))
    m_bLocalEcho = false;
  else
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

  return pack_result_code(ERR_CODE_OK);
}
