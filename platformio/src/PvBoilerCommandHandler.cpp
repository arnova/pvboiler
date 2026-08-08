#include "PvBoilerCommandHandler.h"
#include "Uptime.h"
#include "Terminal.h"
#include "PvBoiler.h"
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
                                  "uptime                 : Show device uptime\r\n"
                                  "enable                 : Enable controller\r\n"
                                  "disable                : Disable controller\r\n"
                                  "budget [p]             : For budget logic mode set available budget to [p] Watt\r\n"
                                  "percent [p]            : For percent logic mode set percentage to [p] percent\r\n"
                                  "boost [on|off]         : Turn boost mode (100% output) on or off\r\n"
                                  "boiler [p]             : Set boiler power rating to [p] Watt\r\n"
                                  "logicmode [l]          : Set logic mode to [l] (\"percent\" or \"budget\")\r\n"
                                  "ssid [s]               : Set WiFi SSID to [s]\r\n"
                                  "pass [w]               : Set WiFi password to [w]\r\n"
                                  "ipaddr [ip]            : Set [ip] (\"dhcp\" for DHCP) for device IP address\r\n"
                                  "netmask [mask]         : Set [mask] for device IP netmask\r\n"
                                  "serverip [ip]          : Set MQTT server IP address to [ip]\r\n"
                                  "restartnet             : Restart network functions\r\n"
                                  "netwdt [i]             : Set network watchdog timeout to [i] seconds (0 or \"off\" to disable)\r\n"
                                  "netwdr [i]             : Set network watchdog recovery time to [i] seconds\r\n"
                                  "exhelp                 : Show expert help\r\n"
                                  "factoryreset           : Reset all (stored) settings to factory defaults (except network)\r\n"
                                  ;

const char EX_HELP_STR_P[] PROGMEM = "\r\n"
                                  "dimstyle [s]           : Set dim style to [s] (\"ssr\" or \"phase-angle\")\r\n"
                                  "ssrperiod [p]          : When using SSR dim style use SSR period count [p]\r\n"
                                  "sclamp [c]             : Set step-clamp to value [c] percent\r\n"
                                  "egain [g]              : For budget logic mode set error-gain to value [g]\r\n"
                                  "deadzone [d]           : For budget logic mode set deadzone to [d] Watt\r\n"
                                  ;

// Show copyright + firmware version
result_code_t CPvBoilerCommandHandler::CmdShowVersion(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  CTerminal::println(FPSTR(VER_STR_P));

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdShowFWVersion(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  CTerminal::println(MY_VERSION);

  return pack_result_code(ERR_CODE_OK);
}


// Show help screen
result_code_t CPvBoilerCommandHandler::CmdShowHelp(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  CTerminal::println(FPSTR(HELP_STR_P));

  return pack_result_code(ERR_CODE_OK);
}


// Show expert help screen
result_code_t CPvBoilerCommandHandler::CmdShowExpertHelp(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  CTerminal::println(FPSTR(EX_HELP_STR_P));

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetIpAddress(const char *strArgs)
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


result_code_t CPvBoilerCommandHandler::CmdSetNetMask(const char *strArgs)
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


result_code_t CPvBoilerCommandHandler::CmdSetServerIp(const char *strArgs)
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


result_code_t CPvBoilerCommandHandler::CmdSetWifiSsid(const char *strArgs)
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


result_code_t CPvBoilerCommandHandler::CmdSetWifiPassword(const char *strArgs)
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



result_code_t CPvBoilerCommandHandler::CmdRestartNet(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  m_network.InitWifi(true);

  return pack_result_code(ERR_CODE_OK);
}



result_code_t CPvBoilerCommandHandler::CmdEnable(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  m_pvBoiler.SetCtrlOnOff(true);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdDisable(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  m_pvBoiler.SetCtrlOnOff(false);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdInfo(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  char strBuf[20]; // Enough room for float with 3 decimals

  CTerminal::print("ssid=");
  CTerminal::print(m_network.GetWifiSsid());

  CTerminal::print(" pass=");
  CTerminal::print(m_network.GetWifiPassword());

  CTerminal::print(" ip=");
  CTerminal::print(m_network.GetIpAddr()[0]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetIpAddr()[1]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetIpAddr()[2]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetIpAddr()[3]);

  CTerminal::print(" netmask=");
  CTerminal::print(m_network.GetNetMask()[0]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetNetMask()[1]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetNetMask()[2]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetNetMask()[3]);

  CTerminal::print(" server=");
  CTerminal::print(m_network.GetServerIp()[0]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetServerIp()[1]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetServerIp()[2]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetServerIp()[3]);

  CTerminal::println("");

  CTerminal::print("boiler_rating=");
  snprintf(strBuf, sizeof(strBuf), "%uW", m_pvBoiler.GetBoilerPowerRating());
  CTerminal::print(strBuf);

  CTerminal::print(" dim_style=");
  CTerminal::print(m_pvBoiler.GetDimStyle() == CPvBoiler::DIM_STYLE_PHASE_ANGLE ? "phase-angle" : "ssr");

  CTerminal::print(" ssr_period_count=");
  snprintf(strBuf, sizeof(strBuf), "%u", m_pvBoiler.GetSsrPeriodCount());
  CTerminal::print(strBuf);

  CTerminal::println("");

  CTerminal::print("error_gain=");
  snprintf(strBuf, sizeof(strBuf), "%.3f", m_pvBoiler.GetErrorGain());
  CTerminal::print(strBuf);

  CTerminal::print(" step_clamp=");
  snprintf(strBuf, sizeof(strBuf), "%.2f%%", m_pvBoiler.GetStepClamp());
  CTerminal::print(strBuf);

  CTerminal::print(" dead_zone=");
  snprintf(strBuf, sizeof(strBuf), "%uW", m_pvBoiler.GetDeadZone());
  CTerminal::print(strBuf);

  CTerminal::println("");

  CTerminal::print("net_wd_timeout=");
  snprintf(strBuf, sizeof(strBuf), "%us", m_pvBoiler.GetNetWatchDogTimeout());
  CTerminal::print(strBuf);

  CTerminal::print(" net_wd_recovery=");
  snprintf(strBuf, sizeof(strBuf), "%us", m_pvBoiler.GetNetWatchDogRecovery());
  CTerminal::print(strBuf);

  CTerminal::println("");
  CTerminal::println("");

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdStatus(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  CTerminal::print("on_off=");
  CTerminal::print(m_pvBoiler.GetCtrlOnOff() ? "1" : "0");

  CTerminal::print(" error=");
  CTerminal::print(m_pvBoiler.GetError() ? "1" : "0");

  CTerminal::println("");

  CTerminal::print("wifi_conn=");
  CTerminal::print(m_network.IsConnected() ? "1" : "0");

  CTerminal::print(" mqtt_conn=");
  CTerminal::print(m_network.IsMqttConnected() ? "1" : "0");

  char strBuf[20]; // Enough room for float with 3 decimals

  CTerminal::print(" wifi_ip=");
  snprintf(strBuf, sizeof(strBuf), "%u.%u.%u.%u", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  CTerminal::print(strBuf);

  CTerminal::println("");

  CTerminal::print("logic_mode=");
  CTerminal::print(m_pvBoiler.GetLogicMode() == CPvBoiler::LOGIC_MODE_PERCENT ? "percent" : "budget");

  if (m_pvBoiler.GetLogicMode() == CPvBoiler::LOGIC_MODE_PERCENT)
  {
    CTerminal::print(" perc_set=");
    snprintf(strBuf, sizeof(strBuf), "%u%%", m_pvBoiler.GetPowerPercentage());
    CTerminal::print(strBuf);
  }
  else
  {
    CTerminal::print(" budget_set=");
    snprintf(strBuf, sizeof(strBuf), "%dW", m_pvBoiler.GetPowerBudget());
    CTerminal::print(strBuf);
  }

  CTerminal::print(" boost=");
  CTerminal::print(m_pvBoiler.GetPowerBoost() ? "1" : "0");

  CTerminal::println("");

  CTerminal::print("power_out=");
  snprintf(strBuf, sizeof(strBuf), "%uW", m_pvBoiler.GetCurrentPower());
  CTerminal::print(strBuf);

  CTerminal::print(" perc_out=");
  snprintf(strBuf, sizeof(strBuf), "%.2f%%", m_pvBoiler.GetCurrentPercentage());
  CTerminal::print(strBuf);

  CTerminal::println("");

  CTerminal::print("net_period=");
  snprintf(strBuf, sizeof(strBuf), "%.3fms", static_cast<float>(m_pvBoiler.GetNetPeriod()) / 500.0f);
  CTerminal::print(strBuf);

  CTerminal::print(" net_freq=");
  snprintf(strBuf, sizeof(strBuf), "%.2fHz", (500.0f * 1000.0f) / static_cast<float>(m_pvBoiler.GetNetPeriod()));
  CTerminal::print(strBuf);

  CTerminal::print(" zero_cross_window=");
  snprintf(strBuf, sizeof(strBuf), "%.3fms", static_cast<float>(m_pvBoiler.GetZeroCrossWindow()) / 1000.0f);
  CTerminal::print(strBuf);

  if (m_pvBoiler.GetDimStyle() == CPvBoiler::DIM_STYLE_PHASE_ANGLE)
  {
    CTerminal::println("");

    CTerminal::print("phase_angle=");
    snprintf(strBuf, sizeof(strBuf), "%.3fms", static_cast<float>(m_pvBoiler.GetTriacPhaseAngle()) / 1000.0f);
    CTerminal::print(strBuf);

    CTerminal::print(" angle_factor=");
    snprintf(strBuf, sizeof(strBuf), "%.4f", m_pvBoiler.GetTriacAngleFactor());
    CTerminal::print(strBuf);
  }

  CTerminal::println("");
  CTerminal::println("");

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdUpTime(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  // Publish uptime
  const CUptime::uptime_t upTime = m_pvBoiler.GetUpTime();
  char strTemp[24];
  snprintf(strTemp, sizeof(strTemp), "%uy %ud %02u:%02u:%02u", upTime.iYears, upTime.iDays, upTime.iHours, upTime.iMinutes, upTime.iSeconds);
  CTerminal::println(strTemp);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdReset(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  m_pvBoiler.Reset();

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdFactoryReset(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS);

  m_pvBoiler.FactoryReset();

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetPowerBudget(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  if (m_pvBoiler.GetLogicMode() == CPvBoiler::LOGIC_MODE_PERCENT)
    return pack_result_code(ERR_CODE_CMD_INVALID);
   
  int32_t iPower;
  result = get_int32_from_string(strArgs, &iPower, INT32_MIN, INT32_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetPowerBudget(iPower);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetPowerPercentage(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  if (m_pvBoiler.GetLogicMode() == CPvBoiler::LOGIC_MODE_BUDGET)
    return pack_result_code(ERR_CODE_CMD_INVALID);

  int32_t iPerc;
  result = get_int32_from_string(strArgs, &iPerc, 0, 100, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetPowerPercentage(iPerc);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetPowerBoost(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (STRIEQUALS(strArgs, "on") || STRIEQUALS(strArgs, "1"))
    m_pvBoiler.SetPowerBoost(true);
  else if (STRIEQUALS(strArgs, "off") || STRIEQUALS(strArgs, "0"))
    m_pvBoiler.SetPowerBoost(false);
  else
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetBoilerPowerRating(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iPower;
  result = get_int32_from_string(strArgs, &iPower, 1, BOILER_POWER_RATING_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetBoilerPowerRating(iPower);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDeadZone(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iDeadZone;
  result = get_int32_from_string(strArgs, &iDeadZone, DEAD_ZONE_MIN, DEAD_ZONE_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetDeadZone(iDeadZone);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetLogicMode(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (STRIEQUALS(strArgs, "percent") || STRIEQUALS(strArgs, "percentage") || STRIEQUALS(strArgs, "p"))
    m_pvBoiler.SetLogicMode(CPvBoiler::LOGIC_MODE_PERCENT);
  else if (STRIEQUALS(strArgs, "budget") || STRIEQUALS(strArgs, "b"))
    m_pvBoiler.SetLogicMode(CPvBoiler::LOGIC_MODE_BUDGET);
  else
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

  // Logic-mode changed so need to publish config
  m_pvBoiler.MqttPublishConfig();

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDimStyle(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (STRIEQUALS(strArgs, "phase-angle") || STRIEQUALS(strArgs, "phase-cut") || STRIEQUALS(strArgs, "p"))
    m_pvBoiler.SetDimStyle(CPvBoiler::DIM_STYLE_PHASE_ANGLE);
  else if (STRIEQUALS(strArgs, "ssr") || STRIEQUALS(strArgs, "s"))
    m_pvBoiler.SetDimStyle(CPvBoiler::DIM_STYLE_SSR);
  else
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

  // Dim style changed so need to publish config
  m_pvBoiler.MqttPublishConfig();

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetSsrPeriodCount(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iCount;
  result = get_int32_from_string(strArgs, &iCount, 1, SSR_PERIOD_COUNT_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetSsrPeriodCount(iCount);

  return pack_result_code(ERR_CODE_OK);
}



result_code_t CPvBoilerCommandHandler::CmdSetErrorGain(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  double fGain;
  result = get_double_from_string(strArgs, &fGain, ERROR_GAIN_MIN, ERROR_GAIN_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetErrorGain(fGain);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetStepClamp(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  double fClamp;
  result = get_double_from_string(strArgs, &fClamp, STEP_CLAMP_MIN, STEP_CLAMP_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetStepClamp(fClamp);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdNetWatchdogTimeout(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  if (STRIEQUALS(strArgs, "off") || STRIEQUALS(strArgs, "0"))
  {
    m_pvBoiler.SetNetWatchDogTimeout(0);
  }
  else
  {
    int32_t iTimeout;
    result = get_int32_from_string(strArgs, &iTimeout, 0, NETWORK_WATCHDOG_TIMEOUT_MAX, ARG_INT32_NUM1);
    if (result.code != ERR_CODE_OK)
      return result;

    m_pvBoiler.SetNetWatchDogTimeout(iTimeout);
  }

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdNetWatchdogRecovery(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iTime;
  result = get_int32_from_string(strArgs, &iTime, 0, NETWORK_WATCHDOG_RECOVERY_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetNetWatchDogRecovery(iTime);

  return pack_result_code(ERR_CODE_OK);
}


// Process provided string command
result_code_t CPvBoilerCommandHandler::ProcessCommand(char *strCommand)
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
  else if (STRIEQUALS(strCommand, "exhelp"))
  {
    result = CmdShowExpertHelp(strArgs);
  }
  else if (STRIEQUALS(strCommand, "reboot"))
  {
    result = CmdReboot(strArgs);
  }
  else if (STRIEQUALS(strCommand, "reset"))
  {
    result = CmdReset(strArgs);
  }
  else if (STRIEQUALS(strCommand, "factoryreset"))
  {
    result = CmdFactoryReset(strArgs);
  }
  else if (STRIEQUALS(strCommand, "info"))
  {
    result = CmdInfo(strArgs);
  }
  else if (STRIEQUALS(strCommand, "status"))
  {
    result = CmdStatus(strArgs);
  }
  else if (STRIEQUALS(strCommand, "uptime"))
  {
    result = CmdUpTime(strArgs);
  }
  else if (STRIEQUALS(strCommand, "budget"))
  {
    result = CmdSetPowerBudget(strArgs);
  }
  else if (STRIEQUALS(strCommand, "percent"))
  {
    result = CmdSetPowerPercentage(strArgs);
  }
  else if (STRIEQUALS(strCommand, "boost"))
  {
    result = CmdSetPowerBoost(strArgs);
  }
  else if (STRIEQUALS(strCommand, "boiler"))
  {
    result = CmdSetBoilerPowerRating(strArgs);
  }
  else if (STRIEQUALS(strCommand, "deadzone"))
  {
    result = CmdSetDeadZone(strArgs);
  }
  else if (STRIEQUALS(strCommand, "logicmode"))
  {
    result = CmdSetLogicMode(strArgs);
  }
  else if (STRIEQUALS(strCommand, "dimstyle"))
  {
    result = CmdSetDimStyle(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ssrperiod"))
  {
    result = CmdSetSsrPeriodCount(strArgs);
  }
  else if (STRIEQUALS(strCommand, "egain"))
  {
    result = CmdSetErrorGain(strArgs);
  }
  else if (STRIEQUALS(strCommand, "sclamp"))
  {
    result = CmdSetStepClamp(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ssid"))
  {
    result = CmdSetWifiSsid(strArgs);
  }
  else if (STRIEQUALS(strCommand, "pass"))
  {
    result = CmdSetWifiPassword(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ipaddr"))
  {
    result = CmdSetIpAddress(strArgs);
  }
  else if (STRIEQUALS(strCommand, "netmask"))
  {
    result = CmdSetNetMask(strArgs);
  }
  else if (STRIEQUALS(strCommand, "serverip"))
  {
    result = CmdSetServerIp(strArgs);
  }
  else if (STRIEQUALS(strCommand, "restartnet"))
  {
    result = CmdRestartNet(strArgs);
  }
  else if (STRIEQUALS(strCommand, "enable"))
  {
    result = CmdEnable(strArgs);
  }
  else if (STRIEQUALS(strCommand, "disable"))
  {
    result = CmdDisable(strArgs);
  }
  else if (STRIEQUALS(strCommand, "netwdt"))
  {
    result = CmdNetWatchdogTimeout(strArgs);
  }
  else if (STRIEQUALS(strCommand, "netwdr"))
  {
    result = CmdNetWatchdogRecovery(strArgs);
  }

  return result;
}
