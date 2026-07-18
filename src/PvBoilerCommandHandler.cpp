#include "PvBoilerCommandHandler.h"
#include "TermPrint.h"
#include "pvboiler.h"
#include "util.h"
#include "Network.h"
#include "system.h"

#include <string.h>

const char HELP_STR_P[] PROGMEM = "\r\n"
                                  "help                   : This screen\r\n"
                                  "!                      : Repeat previous command\r\n"
                                  "ver                    : Show device version\r\n"
                                  "reset                  : Reset controller\r\n"
                                  "reboot                 : Reboot controller\r\n"
                                  "info                   : Show device info\r\n"
                                  "status                 : Show device status\r\n"
                                  "boiler [p]             : Set boiler power rating to [p] Watt\r\n"
                                  "logicmode [c]          : Set logic mode to [c] (\"percentage\" or \"budget\")\r\n"
                                  "margin [p]             : For budget logic mode margin to [p] Watt\r\n"
                                  "dimstyle [s]           : Set dim style to [s] (\"ssr\" or \"phase-cut\")\n\r"
                                  "ssrperiod [p]          : When using SSR dim style use SSR period count [p]\r\n"
                                  "egain [g]              : Set error-gain value [g]\r\n"
                                  "ssid [s]               : Set WiFi SSID to [s]\r\n"
                                  "pass [w]               : Set WiFi password to [w]\r\n"
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

  CTermPrint::println(FPSTR(VER_STR_P));

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdShowFWVersion(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  CTermPrint::println(MY_VERSION);

  return pack_result_code(ERR_CODE_OK);
}


// Show help screen
result_code_t CPVBoilerCommandHandler::CmdShowHelp(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  CTermPrint::println(FPSTR(HELP_STR_P));

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdIpAddress(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  uint8_t ipAddress[4] = { 0 };

  if (!STRIEQUALS(strArgs, "dhcp"))
  {
    const result_code_t result = parse_ipv4_arg(strArgs, ipAddress);
    if (result.code != ERR_CODE_OK)
      return pack_result_code(ERR_CODE_INVALID_IPV4);
  }

  m_network.SetIpAddr(ipAddress);

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

  m_network.SetNetMask(ipAddress);

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

  m_network.MqttUpdateServerIp(ipAddress);

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

  m_network.SetWifiSsid(strArgs);

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

  m_network.SetWifiPassword(strArgs);

  return pack_result_code(ERR_CODE_OK);
}



result_code_t CPVBoilerCommandHandler::CmdRestartNet(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  m_network.InitWifi(true);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdInfo(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  CTermPrint::print("ssid=");
  CTermPrint::print(m_network.GetWifiSsid());

  CTermPrint::print(" pass=");
  CTermPrint::print(m_network.GetWifiPassword());

  CTermPrint::print(" ip=");
  CTermPrint::print(m_network.GetIpAddr()[0]);
  CTermPrint::print('.');
  CTermPrint::print(m_network.GetIpAddr()[1]);
  CTermPrint::print('.');
  CTermPrint::print(m_network.GetIpAddr()[2]);
  CTermPrint::print('.');
  CTermPrint::print(m_network.GetIpAddr()[3]);

  CTermPrint::print(" netmask=");
  CTermPrint::print(m_network.GetNetMask()[0]);
  CTermPrint::print('.');
  CTermPrint::print(m_network.GetNetMask()[1]);
  CTermPrint::print('.');
  CTermPrint::print(m_network.GetNetMask()[2]);
  CTermPrint::print('.');
  CTermPrint::print(m_network.GetNetMask()[3]);

  CTermPrint::print(" server=");
  CTermPrint::print(m_network.GetServerIp()[0]);
  CTermPrint::print('.');
  CTermPrint::print(m_network.GetServerIp()[1]);
  CTermPrint::print('.');
  CTermPrint::print(m_network.GetServerIp()[2]);
  CTermPrint::print('.');
  CTermPrint::print(m_network.GetServerIp()[3]);

  CTermPrint::print(" logic_mode=");
  CTermPrint::print(m_pvBoiler.GetLogicMode() == CPVBoiler::LOGIC_MODE_PERCENTAGE ? "percentage" : "budget");

  CTermPrint::print(" boiler=");
  CTermPrint::print(m_pvBoiler.GetBoilerPowerRating());
  CTermPrint::print("W");

  CTermPrint::print(" margin=");
  CTermPrint::print(m_pvBoiler.GetPowerBudgetMargin());
  CTermPrint::print("W");

  CTermPrint::print(" dim_style=");
  CTermPrint::print(m_pvBoiler.GetDimStyle() == CPVBoiler::DIM_STYLE_PHASE_CUT ? "phase-cut" : "ssr");

  CTermPrint::print(" ssr_period=");
  CTermPrint::print(m_pvBoiler.GetSsrPeriodCount());

  CTermPrint::print(" egain=");
  CTermPrint::print(m_pvBoiler.GetErrorGain());

  CTermPrint::println("");

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdStatus(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  CTermPrint::print("on_off=");
  CTermPrint::print(m_pvBoiler.GetCtrlOnOff() ? "on" : "off");

  CTermPrint::print(" wifi_conn=");
  CTermPrint::print(m_network.IsConnected() ? "1" : "0");

  CTermPrint::print(" wifi_ip=");
  CTermPrint::print(WiFi.localIP().toString().c_str());

  CTermPrint::print(" mqtt_conn=");
  CTermPrint::print(m_network.IsMqttConnected() ? "1" : "0");

  if (m_pvBoiler.GetLogicMode() == CPVBoiler::LOGIC_MODE_PERCENTAGE)
  {
    CTermPrint::print(" perc_set=");
    CTermPrint::print(m_pvBoiler.GetPowerPercentage());
    CTermPrint::print("%");
  }
  else
  {
    CTermPrint::print(" budget_set=");
    CTermPrint::print(m_pvBoiler.GetPowerBudget());
    CTermPrint::print("W");
  }

  CTermPrint::print(" power_out=");
  CTermPrint::print(m_pvBoiler.GetCurrentPower());
  CTermPrint::print("W");

  CTermPrint::print(" perc_out=");
  CTermPrint::print(m_pvBoiler.GetCurrentPercentage());
  CTermPrint::print("%");

  if (m_pvBoiler.GetDimStyle() == CPVBoiler::DIM_STYLE_PHASE_CUT)
  {
    CTermPrint::print(" angle_factor=");
    CTermPrint::print(m_pvBoiler.GetTriacAngleFactor());
  }

  CTermPrint::println("");

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdReset(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  // FIXME: Implementation

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

  m_pvBoiler.SetBoilerPowerRating(iPower);

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

  m_pvBoiler.SetPowerBudgetMargin(iPower);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdLogicMode(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (STRIEQUALS(strArgs, "percentage") || STRIEQUALS(strArgs, "p"))
    m_pvBoiler.SetLogicMode(CPVBoiler::LOGIC_MODE_PERCENTAGE);
  else if (STRIEQUALS(strArgs, "budget") || STRIEQUALS(strArgs, "b"))
    m_pvBoiler.SetLogicMode(CPVBoiler::LOGIC_MODE_BUDGET);
  else
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

  // Logic-mode changed so need to publish config
  m_pvBoiler.MqttPublishConfig();

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPVBoilerCommandHandler::CmdDimStyle(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (STRIEQUALS(strArgs, "phase-cut") || STRIEQUALS(strArgs, "p"))
    m_pvBoiler.SetDimStyle(CPVBoiler::DIM_STYLE_PHASE_CUT);
  else if (STRIEQUALS(strArgs, "ssr") || STRIEQUALS(strArgs, "s"))
    m_pvBoiler.SetDimStyle(CPVBoiler::DIM_STYLE_SSR);
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

  m_pvBoiler.SetSsrPeriodCount(iCount);

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

  m_pvBoiler.SetErrorGain(fGain);

  return pack_result_code(ERR_CODE_OK);
}


// Process provided string command
result_code_t CPVBoilerCommandHandler::ProcessCommand(char *strCommand)
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
  else if (STRIEQUALS(strCommand, "boiler"))
  {
    result = CmdBoilerPowerRating(strArgs);
  }
  else if (STRIEQUALS(strCommand, "margin"))
  {
    result = CmdPowerBudgetMargin(strArgs);
  }
  else if (STRIEQUALS(strCommand, "logicmode"))
  {
    result = CmdLogicMode(strArgs);
  }
  else if (STRIEQUALS(strCommand, "dimstyle"))
  {
    result = CmdDimStyle(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ssrperiod"))
  {
    result = CmdSsrPeriodCount(strArgs);
  }
  else if (STRIEQUALS(strCommand, "egain"))
  {
    result = CmdErrorGain(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ssid"))
  {
    result = CmdWifiSsid(strArgs);
  }
  else if (STRIEQUALS(strCommand, "pass"))
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
    CTermPrint::print(strResult);
  }

  return result;
}
