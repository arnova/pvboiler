#include "PvBoilerCommandHandler.h"
#include "pvboiler.h"
#include "util.h"
#include "system.h"

#include <string.h>

// Forward declare
void PrintStrN(const char *str);
void PrintStr(const char *str);
void PrintChar(const char c);

result_code_t CommandSetBPRating(const uint16_t& iPower);
result_code_t CommandSetDimStyle(const CPVBoiler::dim_style_t& dimStyle);
result_code_t CommandSetPowerBudgetMargin(const uint16_t& iMargin);
result_code_t CommandSetControlMode(const bool& bPercentage);
result_code_t CommandSetSSRPeriodCount(const uint8_t& iCount);
result_code_t CommandSetErrorGain(const float& fGain);

result_code_t CommandSetWifiSsid(const char* strSsid);
result_code_t CommandSetWifiPassword(const char* strPassword);

result_code_t CommandSetIp(uint8_t* ipAddress);
result_code_t CommandSetNetMask(uint8_t* ipNetMask);
result_code_t CommandSetServerIp(uint8_t* ipAddress);
result_code_t CommandRestartNet();

const char HELP_STR[] = "\r\n"
                        "help                   : This screen\r\n"
                        "!                      : Repeat previous command\r\n"
                        "ver                    : Show device version\r\n"
                        "reset                  : Reset controller\r\n"
                        "reboot                 : Reboot controller\r\n"
                        "info                   : Show device info\r\n"
                        "status                 : Show device status\r\n"
                        "bprating [p]           : Set boiler power rating to [p] Watt\r\n"
                        "pbmargin [p]           : Set power budget margin to [p] Watt\r\n"
                        "ctrlmode [c]           : Set control mode to [c] (\"percentage\" or \"budget\")\r\n"
                        "dstyle [s]             : Set dim style to [s] (\"ssr\" or \"phase-cut\")\n\r"
                        "ssrpc [p]              : When using SSR dim style use SSR period count [p]\r\n"
                        "egain [g]              : Set error-gain value [g]\r\n"
                        "wssid [s]              : Set WiFi SSID to [s]\r\n"
                        "wpass [w]              : Set WiFi password to [w]\r\n"
                        "ipaddr [ip]            : Use [ip] (\"dhcp\" for DHCP) for device IP address\r\n"
                        "netmask [mask]         : Use [mask] for device IP netmask\r\n"
                        "serverip [ip]          : Set MQTT server IP address to [ip]\n\r"
                        "restartnet             : Restart network functions\n\r"
                        ;

// Show copyright + firmware version
result_code_t CPVBoilerCommandHandler::CmdShowVersion(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  PrintStrN(VER_STR);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdShowFWVersion(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  PrintStrN(MY_VERSION);

  return pack_result_code(ERR_CODE_OK);
}


// Show help screen
result_code_t CPVBoilerCommandHandler::CmdShowHelp(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  PrintStrN(HELP_STR);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdIpAddress(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  uint8_t ipAddress[4];
    const result_code_t result = parse_ipv4_arg(strArgs, ipAddress);
  if (result.code != ERR_CODE_OK)
    return pack_result_code(ERR_CODE_INVALID_IPV4);

  CommandSetIp(ipAddress);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdNetMask(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  uint8_t ipAddress[4];
    const result_code_t result = parse_ipv4_arg(strArgs, ipAddress);
  if (result.code != ERR_CODE_OK)
    return pack_result_code(ERR_CODE_INVALID_IPV4);

  CommandSetNetMask(ipAddress);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdServerIp(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  uint8_t ipAddress[4];
    const result_code_t result = parse_ipv4_arg(strArgs, ipAddress);
  if (result.code != ERR_CODE_OK)
    return pack_result_code(ERR_CODE_INVALID_IPV4);

  CommandSetServerIp(ipAddress);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdWifiSsid(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (strlen(strArgs) > WIFI_SSID_MAX_SIZE)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_STR_MAX, ARG_INT32_NUM1);
    return store_arg1_int32(resultCode, WIFI_SSID_MAX_SIZE);
  }

  CommandSetWifiSsid(strArgs);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdWifiPassword(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (strlen(strArgs) > WIFI_PASSWORD_MAX_SIZE)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_STR_MAX, ARG_INT32_NUM1);
    return store_arg1_int32(resultCode, WIFI_PASSWORD_MAX_SIZE);
  }

  CommandSetWifiPassword(strArgs);

  return pack_result_code(ERR_CODE_OK);
}



result_code_t CPVBoilerCommandHandler::CmdRestartNet(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  CommandRestartNet();

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdBoilerPowerRating(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM2);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iPower;
  result = get_int32_from_string(strArgs, &iPower, 1, 10000, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  CommandSetBPRating(iPower);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdPowerBudgetMargin(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iPower;
  result = get_int32_from_string(strArgs, &iPower, 1, 10000, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  CommandSetPowerBudgetMargin(iPower);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdControlMode(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (STRIEQUALS(strArgs, "percentage"))
    CommandSetControlMode(true);
  else if (STRIEQUALS(strArgs, "budget"))
    CommandSetControlMode(false);
  else
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdDimStyle(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (STRIEQUALS(strArgs, "phase-cut"))
    CommandSetDimStyle(CPVBoiler::DIM_STYLE_PHASE_CUT);
  else if (STRIEQUALS(strArgs, "ssr"))
    CommandSetDimStyle(CPVBoiler::DIM_STYLE_SSR);
  else
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdSsrPeriodCount(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iCount;
  result = get_int32_from_string(strArgs, &iCount, 1, 255, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  CommandSetSSRPeriodCount(iCount);

  return pack_result_code(ERR_CODE_OK);
}



result_code_t CPVBoilerCommandHandler::CmdErrorGain(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  double fGain;
  result = get_double_from_string(strArgs, &fGain, 0.01f, 100.0f, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  CommandSetErrorGain(fGain);

  return pack_result_code(ERR_CODE_OK);
}


// Process provided string command
result_code_t CPVBoilerCommandHandler::ProcessCommand(char *strCommand, WiFiClient* wifiClient /* = NULL */)
{
  // Trim strCommand to remove leading/trailing space \n \r
  strtrim(strCommand, " \r\n");

  // Preinit return value
  result_code_t result = pack_result_code(ERR_CODE_CMD_UNKNOWN);

  // Split into strCommand & arguments
  char *strArgs = strsplit(strCommand, " "); // strArgs may be NULL, but is handled by the functions called below
  if (strArgs)
  {
    strtrimleft(strArgs, " "); // Trim any leading spaces
  }

  // Our command interpreter:
  if (STRIEQUALS(strCommand, "ver"))
  {
    result = CmdShowVersion(strArgs);
  }
  else if (STRIEQUALS(strCommand, "fw") || STRIEQUALS(strCommand, "fwv"))
  {
    result = CmdShowFWVersion(strArgs);
  }
  else if (STRIEQUALS(strCommand, "help") || STRIEQUALS(strCommand, "?"))
  {
    result = CmdShowHelp(strArgs);
  }
  else if (STRIEQUALS(strCommand, "reboot"))
  {
    result = CmdReboot(strArgs);
  }
  else if (STRIEQUALS(strCommand, "reset"))
  {
    result = CmdReset(strArgs);
  }
  else if (STRIEQUALS(strCommand, "info"))
  {
    result = CmdInfo(strArgs);
  }
  else if (STRIEQUALS(strCommand, "status"))
  {
    result = CmdStatus(strArgs);
  }
  else if (STRIEQUALS(strCommand, "bprating"))
  {
    result = CmdBoilerPowerRating(strArgs);
  }
  else if (STRIEQUALS(strCommand, "pbmargin"))
  {
    result = CmdPowerBudgetMargin(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ctrlmode"))
  {
    result = CmdControlMode(strArgs);
  }
  else if (STRIEQUALS(strCommand, "dstyle"))
  {
    result = CmdDimStyle(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ssrpc"))
  {
    result = CmdSsrPeriodCount(strArgs);
  }
  else if (STRIEQUALS(strCommand, "egain"))
  {
    result = CmdErrorGain(strArgs);
  }
  else if (STRIEQUALS(strCommand, "wssid"))
  {
    result = CmdWifiSsid(strArgs);
  }
  else if (STRIEQUALS(strCommand, "wpass"))
  {
    result = CmdWifiPassword(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ipaddr"))
  {
    result = CmdIpAddress(strArgs);
  }
  else if (STRIEQUALS(strCommand, "netmask"))
  {
    result = CmdNetMask(strArgs);
  }
  else if (STRIEQUALS(strCommand, "serverip"))
  {
    result = CmdServerIp(strArgs);
  }
  else if (STRIEQUALS(strCommand, "restartnet"))
  {
    result = CmdRestartNet(strArgs);
  }

  // Finally output result-code string (OK or ERROR:)
  if (result.code != ERR_CODE_OK_NULL)
  {
    char strResult[RESULT_BUF_SIZE];
    get_error_string(result, strResult, false);
    PrintStr(strResult);
  }

  return result;
}
