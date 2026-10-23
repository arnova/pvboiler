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
                                  "fw                     : Show firmware version (number only)\r\n"
                                  "reset                  : Reset controller\r\n"
                                  "reboot                 : Reboot controller\r\n"
                                  "info                   : Show device info\r\n"
                                  "status                 : Show device status\r\n"
                                  "uptime                 : Show device uptime\r\n"
                                  "echo [e]               : Set terminal local echo to [e] (\"on\" or \"off\")\r\n"
                                  "budget [p]             : For budget mode set available budget to [p] Watt\r\n"
                                  "percent [p]            : For percent mode set percentage to [p] percent\r\n"
                                  "budgetsrc [s]          : Set power budget source to [s] (\"direct\" or \"dsmr-reader\")\r\n"
                                  "thermsp [t]            : Set thermostat setpoint to [t] Celsius (" STRINGIZE(THERMOSTAT_SETPOINT_MIN) "-" STRINGIZE(THERMOSTAT_SETPOINT_MAX) ")\r\n"
                                  "boiler [p]             : Set boiler power rating to [p] Watt\r\n"
                                  "mode [m]               : Set (operating) mode to [m] (\"percent\", \"budget\", \"percent_sp\", \"budget_sp\", \"sp_budget\", \"percent_sp_budget\", \"standby\", \"off\", \"setpoint\", \"boost\")\r\n"
                                  "hostname [h]           : Set hostname to [h] (requires reboot)\r\n"
                                  "ssid [s]               : Set WiFi SSID to [s]\r\n"
                                  "pass [w]               : Set WiFi password to [w]\r\n"
                                  "ipaddr [ip]            : Set [ip] (\"dhcp\" for DHCP) for device IP address\r\n"
                                  "netmask [mask]         : Set [mask] for device IP netmask\r\n"
                                  "serverip [ip]          : Set MQTT server IP address to [ip]\r\n"
                                  "mqttuser [u]           : Set MQTT server user to [u] (may be empty)\r\n"
                                  "mqttpass [p]           : Set MQTT server password to [p] (may be empty)\r\n"
                                  "mqttinterval [s]       : Set MQTT update interval to [s] seconds\r\n"
                                  "restartnet             : Restart network (after eg. WiFi/IP settings changes)\r\n"
                                  "netwdt [i]             : Set network watchdog timeout to [i] seconds (0 or \"off\" to disable)\r\n"
                                  "netwdr [i]             : Set network watchdog recovery time to [i] seconds\r\n"
                                  "exhelp                 : Show expert help\r\n"
                                  "factoryreset           : Reset all (stored) settings to factory defaults (except network)\r\n"
                                  ;

const char EX_HELP_STR_P[] PROGMEM = "\r\n"
                                  "dimstyle [s]           : Set dim style to [s] (\"ssr\" or \"phase-angle\")\r\n"
                                  "ssrperiod [p]          : When using SSR dim style use SSR period count [p] (full periods)\r\n"
                                  "scp [c]                : Set positive step-clamp to value [c] percent\r\n"
                                  "scn [c]                : Set negative step-clamp to value [c] percent\r\n"
                                  "egp [g]                : Set budget mode positive error-gain to value [g]\r\n"
                                  "egn [g]                : Set budget mode negative error-gain to value [g]\r\n"
                                  "bmargin [m]            : Set budget mode margin to [m] Watt\r\n"
                                  "deadzone [d]           : Set budget mode deadzone to [d] Watt\r\n"
                                  "thermhyst [h]          : Set thermostat hysteresis to [h] Celsius (" STRINGIZE(THERMOSTAT_HYSTERESIS_MIN) "-" STRINGIZE(THERMOSTAT_HYSTERESIS_MAX) ")\r\n"
                                  "disinfen [e]           : Set legionella disinfection to [e] (\"on\" or \"off\")\r\n"
                                  "disinfint [d]          : Set legionella disinfect interval (max. time in danger zone) to [d] days (" STRINGIZE(DISINFECTION_INTERVAL_DAYS_MIN) "-" STRINGIZE(DISINFECTION_INTERVAL_DAYS_MAX) ")\r\n"
                                  "disinfdur [m]          : Set legionella disinfection duration to [m] minutes (" STRINGIZE(DISINFECTION_DURATION_MINUTES_MIN) "-" STRINGIZE(DISINFECTION_DURATION_MINUTES_MAX) ")\r\n"
                                  "disinftemp [t]         : Set legionella disinfect temperature to [t] Celsius (" STRINGIZE(DISINFECTION_TEMPERATURE_MIN) "-" STRINGIZE(DISINFECTION_TEMPERATURE_MAX) ")\r\n"
                                  "dzlow [t]              : Set legionella danger zone low temperature to [t] Celsius (" STRINGIZE(DANGER_ZONE_TEMPERATURE_MIN) "-" STRINGIZE(DANGER_ZONE_TEMPERATURE_MAX) ")\r\n"
                                  "dzhigh [t]             : Set legionella danger zone high temperature to [t] Celsius (" STRINGIZE(DANGER_ZONE_TEMPERATURE_MIN) "-" STRINGIZE(DANGER_ZONE_TEMPERATURE_MAX) ")\r\n"
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


result_code_t CPvBoilerCommandHandler::CmdSetHostName(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (strlen(strArgs) > HOST_NAME_MAX_SIZE)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_STR_MAX, ARG_INT32_NUM1);
    return store_arg1_int32(resultCode, HOST_NAME_MAX_SIZE);
  }

  if (!CNetwork::IsValidHostName(strArgs))
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

  m_network.SetHostName(strArgs);

  return pack_result_code(ERR_CODE_OK_AFTER_RESTART);
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

  return pack_result_code(ERR_CODE_OK_AFTER_RESTART);
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

  return pack_result_code(ERR_CODE_OK_AFTER_RESTART);
}


result_code_t CPvBoilerCommandHandler::CmdSetMqttIpAddress(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  uint8_t ipAddress[4];
    const result_code_t result = parse_ipv4_arg(strArgs, ipAddress);
  if (result.code != ERR_CODE_OK)
    return pack_result_code(ERR_CODE_INVALID_IPV4);

  m_network.SetMqttServerIp(ipAddress);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetMqttUpdateInterval(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iInterval;
  result = get_int32_from_string(strArgs, &iInterval, MQTT_UPDATE_TIME_MIN, MQTT_UPDATE_TIME_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetMqttUpdateInterval(iInterval);

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

  return pack_result_code(ERR_CODE_OK_AFTER_RESTART);
}


result_code_t CPvBoilerCommandHandler::CmdSetWifiPassword(const char *strArgs)
{
  if (strArgs && strlen(strArgs) > WIFI_PASSWORD_MAX_SIZE)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_STR_MAX, ARG_INT32_NUM1);
    return store_arg1_int32(resultCode, WIFI_PASSWORD_MAX_SIZE);
  }

  m_network.SetWifiPassword((strArgs == NULL) ? "" : strArgs);

  return pack_result_code(ERR_CODE_OK_AFTER_RESTART);
}


result_code_t CPvBoilerCommandHandler::CmdSetMqttUser(const char *strArgs)
{
  if (strArgs && strlen(strArgs) > MQTT_USER_MAX_SIZE)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_STR_MAX, ARG_INT32_NUM1);
    return store_arg1_int32(resultCode, MQTT_USER_MAX_SIZE);
  }

  m_network.SetMqttUser((strArgs == NULL) ? "" : strArgs);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetMqttPassword(const char *strArgs)
{
  if (strArgs && strlen(strArgs) > MQTT_PASSWORD_MAX_SIZE)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_STR_MAX, ARG_INT32_NUM1);
    return store_arg1_int32(resultCode, MQTT_PASSWORD_MAX_SIZE);
  }

  m_network.SetMqttPassword((strArgs == NULL) ? "" : strArgs);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdRestartNet(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  // Output result before restarting, else it's lost on a socket (terminal) connection
  CTerminal::println("OK");

  delay(3000); // Delay to make sure network socket processing is done

  m_network.InitWifi(true);

  return pack_result_code(ERR_CODE_OK_NULL);
}


result_code_t CPvBoilerCommandHandler::CmdInfo(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  char strBuf[20]; // Enough room for float with 3 decimals

  CTerminal::print("host_name=");
  CTerminal::print(m_network.GetHostName());

  CTerminal::print(" wifi_ssid=");
  CTerminal::print(m_network.GetWifiSsid());

  CTerminal::print(" wifi_pass=");
  const char* strWifiPass = m_network.GetWifiPassword();
  CTerminal::print((strlen(strWifiPass) == 0) ? "(empty)" : strWifiPass);

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

  CTerminal::print(" mqtt_server=");
  CTerminal::print(m_network.GetMqttIpAddr()[0]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetMqttIpAddr()[1]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetMqttIpAddr()[2]);
  CTerminal::print(".");
  CTerminal::print(m_network.GetMqttIpAddr()[3]);

  CTerminal::print(" mqtt_user=");
  const char* strMqttUser = m_network.GetMqttUser();
  CTerminal::print((strlen(strMqttUser) == 0) ? "(empty)" : strMqttUser);

  CTerminal::print(" mqtt_password=");
  const char* strMqttPass = m_network.GetMqttPassword();
  CTerminal::print((strlen(strMqttPass) == 0) ? "(empty)" : strMqttPass);

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

  CTerminal::print("budget_src=");
  CTerminal::print(m_pvBoiler.GetDsmrPowerBudget() ? "dsmr-reader" : "direct");

  CTerminal::print(" error_gain_pos=");
  snprintf(strBuf, sizeof(strBuf), "%.3f", m_pvBoiler.GetErrorGainPos());
  CTerminal::print(strBuf);

  CTerminal::print(" error_gain_neg=");
  snprintf(strBuf, sizeof(strBuf), "%.3f", m_pvBoiler.GetErrorGainNeg());
  CTerminal::print(strBuf);

  CTerminal::print(" step_clamp_pos=");
  snprintf(strBuf, sizeof(strBuf), "%.2f%%", m_pvBoiler.GetStepClampPos());
  CTerminal::print(strBuf);

  CTerminal::print(" step_clamp_neg=");
  snprintf(strBuf, sizeof(strBuf), "%.2f%%", m_pvBoiler.GetStepClampNeg());
  CTerminal::print(strBuf);

  CTerminal::print(" dead_zone=");
  snprintf(strBuf, sizeof(strBuf), "%uW", m_pvBoiler.GetDeadZone());
  CTerminal::print(strBuf);

  CTerminal::print(" budget_margin=");
  snprintf(strBuf, sizeof(strBuf), "%uW", m_pvBoiler.GetBudgetMargin());
  CTerminal::print(strBuf);

  CTerminal::println("");

  CTerminal::print("thermostat_hysteresis=");
  snprintf(strBuf, sizeof(strBuf), "%uC", m_pvBoiler.GetThermostatHysteresis());
  CTerminal::print(strBuf);

  CTerminal::print(" danger_zone_low=");
  snprintf(strBuf, sizeof(strBuf), "%uC", m_pvBoiler.GetDangerZoneLowTemperature());
  CTerminal::print(strBuf);

  CTerminal::print(" danger_zone_high=");
  snprintf(strBuf, sizeof(strBuf), "%uC", m_pvBoiler.GetDangerZoneHighTemperature());
  CTerminal::print(strBuf);

  CTerminal::print(" disinfect_interval=");
  snprintf(strBuf, sizeof(strBuf), "%ud", m_pvBoiler.GetDisinfectionIntervalDays());
  CTerminal::print(strBuf);

  CTerminal::print(" disinfect_duration=");
  snprintf(strBuf, sizeof(strBuf), "%um", m_pvBoiler.GetDisinfectionDurationMinutes());
  CTerminal::print(strBuf);

  CTerminal::print(" disinfect_temp=");
  snprintf(strBuf, sizeof(strBuf), "%uC", m_pvBoiler.GetDisinfectionTemperature());
  CTerminal::print(strBuf);

  CTerminal::println("");

  CTerminal::print("mqtt_update_interval=");
  snprintf(strBuf, sizeof(strBuf), "%us", m_pvBoiler.GetMqttUpdateInterval());
  CTerminal::print(strBuf);

  CTerminal::print(" net_wd_timeout=");
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
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  CTerminal::print("wifi_conn=");
  CTerminal::print(m_network.IsConnected() ? "1" : "0");

  CTerminal::print(" mqtt_conn=");
  CTerminal::print(m_network.IsMqttConnected() ? "1" : "0");

  char strBuf[20]; // Enough room for float with 3 decimals

  CTerminal::print(" wifi_ip=");
  snprintf(strBuf, sizeof(strBuf), "%u.%u.%u.%u", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  CTerminal::print(strBuf);

  CTerminal::println("");

  CTerminal::print("mode=");
  switch(m_pvBoiler.GetMode())
  {
    case CPvBoiler::MODE_PERCENT:
    {
      CTerminal::print("percent");
    }
    break;

    case CPvBoiler::MODE_BUDGET:
    {
      CTerminal::print("budget");
    }
    break;

    case CPvBoiler::MODE_BOOST:
    {
      CTerminal::print("boost");
    }
    break;

    case CPvBoiler::MODE_SETPOINT:
    {
      CTerminal::print("setpoint");
    }
    break;

    case CPvBoiler::MODE_STANDBY:
    {
      CTerminal::print("standby");
    }
    break;

    case CPvBoiler::MODE_OFF:
    {
      CTerminal::print("off");
    }
    break;

    case CPvBoiler::MODE_BUDGET_SETPOINT:
    {
      CTerminal::print("budget_sp");
    }
    break;

    case CPvBoiler::MODE_PERCENT_SETPOINT:
    {
      CTerminal::print("percent_sp");
    }
    break;

    case CPvBoiler::MODE_SETPOINT_BUDGET:
    {
      CTerminal::print("sp_budget");
    }
    break;

    case CPvBoiler::MODE_PERCENT_SETPOINT_BUDGET:
    {
      CTerminal::print("percent_sp_budget");
    }
    break;
  }

  CTerminal::print(" perc_set=");
  snprintf(strBuf, sizeof(strBuf), "%u%%", m_pvBoiler.GetPowerPercentage());
  CTerminal::print(strBuf);

  CTerminal::print(" budget_set=");
  snprintf(strBuf, sizeof(strBuf), "%dW", m_pvBoiler.GetPowerBudget());
  CTerminal::print(strBuf);

  CTerminal::println("");

  CTerminal::print("power_out=");
  snprintf(strBuf, sizeof(strBuf), "%uW", m_pvBoiler.GetCurrentPower());
  CTerminal::print(strBuf);

  CTerminal::print(" perc_out=");
  snprintf(strBuf, sizeof(strBuf), "%.2f%%", m_pvBoiler.GetCurrentPercentage());
  CTerminal::print(strBuf);

  CTerminal::println("");

  CTerminal::print("boiler_t=");
  if (m_pvBoiler.GetBoilerTemperature() > 0.0f)
  {
    snprintf(strBuf, sizeof(strBuf), "%.1fC", m_pvBoiler.GetBoilerTemperature());
    CTerminal::print(strBuf);
  }
  else
  {
    CTerminal::print("n/a");
  }

  CTerminal::print(" over_heated=");
  CTerminal::print(m_pvBoiler.IsBoilerOverheated() ? "1" : "0");

  CTerminal::print(" thermostat_set=");
  snprintf(strBuf, sizeof(strBuf), "%iC", m_pvBoiler.GetThermostatSetpoint());
  CTerminal::print(strBuf);
  if (!m_pvBoiler.IsThermostatMode())
  {
    CTerminal::print("(off)");
  }

  CTerminal::print(" thermostat_allows_heating=");
  CTerminal::print(m_pvBoiler.GetThermostatAllowsHeating() ? "1" : "0");

  CTerminal::println("");

  CTerminal::print("disinfection_en=");
  CTerminal::print(m_pvBoiler.GetDisinfectionEnabled() ? "on" : "off");

  CTerminal::print(" disinfection_required=");
  CTerminal::print(m_pvBoiler.GetLegionellaDisinfectionRequired() ? "1" : "0");

  CTerminal::print(" in_danger_zone=");
  snprintf(strBuf, sizeof(strBuf), "%uh", m_pvBoiler.GetLegionellaInDangerZoneHours());
  CTerminal::print(strBuf);

  CTerminal::print(" disinfection_age=");
  snprintf(strBuf, sizeof(strBuf), "%uh", m_pvBoiler.GetLegionellaDisinfectionAgeHours());
  CTerminal::print(strBuf);

  CTerminal::print(" disinfecting_elapsed=");
  snprintf(strBuf, sizeof(strBuf), "%um", m_pvBoiler.GetLegionellaDisinfectingElapsedMinutes());
  CTerminal::print(strBuf);

  CTerminal::println("");

  CTerminal::print("mains_period=");
  const uint16_t iPeriod = m_pvBoiler.GetMainsPeriod();
  if (iPeriod == 0 || iPeriod == NET_PERIOD_INVALID)
  {
    CTerminal::print("?ms");
  }
  else
  {
    snprintf(strBuf, sizeof(strBuf), "%.3fms", static_cast<float>(iPeriod) / 500.0f);
    CTerminal::print(strBuf);
  }

  CTerminal::print(" mains_freq=");
  if (iPeriod == 0 || iPeriod == NET_PERIOD_INVALID)
  {
    CTerminal::print("?Hz");
  }
  else
  {
    snprintf(strBuf, sizeof(strBuf), "%.2fHz", (500.0f * 1000.0f) / static_cast<float>(iPeriod));
    CTerminal::print(strBuf);
  }

  CTerminal::print(" mains_zero_cross_window=");
  const uint16_t iWindow = m_pvBoiler.GetMainsZeroCrossWindow();
  if (iWindow == 0 || iWindow == ZERO_CROSS_WINDOW_INVALID)
  {
    CTerminal::print("?ms");
  }
  else
  {
    snprintf(strBuf, sizeof(strBuf), "%.3fms", static_cast<float>(iWindow) / 1000.0f);
    CTerminal::print(strBuf);
  }

  CTerminal::print(" mains_error=");
  CTerminal::print(m_pvBoiler.GetPowerGoodFlag() ? "0" : "1");

  CTerminal::println("");

  if (m_pvBoiler.GetDimStyle() == CPvBoiler::DIM_STYLE_PHASE_ANGLE)
  {
    CTerminal::print("triac_phase_angle=");
    snprintf(strBuf, sizeof(strBuf), "%.3fms", static_cast<float>(m_pvBoiler.GetTriacPhaseAngle()) / 1000.0f);
    CTerminal::print(strBuf);

    CTerminal::print(" triac_angle_factor=");
    snprintf(strBuf, sizeof(strBuf), "%.4f", m_pvBoiler.GetTriacAngleFactor());
    CTerminal::print(strBuf);

    CTerminal::println("");
  }

  CTerminal::println("");

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdUpTime(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  // Publish uptime
  const CUptime::uptime_t upTime = m_pvBoiler.GetUpTime();
  char strTemp[24];
  snprintf(strTemp, sizeof(strTemp), "%ud %02u:%02u:%02u", upTime.iDays, upTime.iHours, upTime.iMinutes, upTime.iSeconds);
  CTerminal::println(strTemp);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdReset(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  m_pvBoiler.Reset();

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdReboot(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  CTerminal::println("OK");

  delay(3000); // Delay to make sure network socket processing is done

  ESP.restart();

  // In principle anything from here on is not executed

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdFactoryReset(const char *strArgs)
{
  if (strArgs != NULL && *strArgs)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, ARG_INT32_NONE);

  m_pvBoiler.FactoryReset();

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetPowerBudget(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iPower;
  result = get_int32_from_string(strArgs, &iPower, INT32_MIN, INT32_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetPowerBudget(iPower);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetBudgetSource(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (STRIEQUALS(strArgs, "direct"))
    m_pvBoiler.SetDsmrPowerBudget(false);
  else if (STRIEQUALS(strArgs, "dsmr-reader") || STRIEQUALS(strArgs, "dsmr"))
    m_pvBoiler.SetDsmrPowerBudget(true);
  else
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetPowerPercentage(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iPerc;
  result = get_int32_from_string(strArgs, &iPerc, 0, 100, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetPowerPercentage(iPerc);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetThermostatSetpoint(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iTemperature;
  result = get_int32_from_string(strArgs, &iTemperature, THERMOSTAT_SETPOINT_MIN, THERMOSTAT_SETPOINT_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetThermostatSetpoint(iTemperature);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDisinfectionEnabled(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  bool bEnable;
  result = get_bool_from_string(strArgs, &bEnable, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetDisinfectionEnabled(bEnable);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetThermostatHysteresis(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iHysteresis;
  result = get_int32_from_string(strArgs, &iHysteresis, THERMOSTAT_HYSTERESIS_MIN, THERMOSTAT_HYSTERESIS_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetThermostatHysteresis(iHysteresis);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDangerZoneLowTemperature(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iTemperature;
  result = get_int32_from_string(strArgs, &iTemperature, DANGER_ZONE_TEMPERATURE_MIN, DANGER_ZONE_TEMPERATURE_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  // Low temperature must be below high temperature
  if (iTemperature >= m_pvBoiler.GetDangerZoneHighTemperature())
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_VAL_MAX, ARG_INT32_NUM1);
    return store_arg1_int32(resultCode, m_pvBoiler.GetDangerZoneHighTemperature() - 1);
  }

  m_pvBoiler.SetDangerZoneLowTemperature(iTemperature);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDangerZoneHighTemperature(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iTemperature;
  result = get_int32_from_string(strArgs, &iTemperature, DANGER_ZONE_TEMPERATURE_MIN, DANGER_ZONE_TEMPERATURE_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  // High temperature must be above low temperature
  if (iTemperature <= m_pvBoiler.GetDangerZoneLowTemperature())
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_VAL_MIN, ARG_INT32_NUM1);
    return store_arg1_int32(resultCode, m_pvBoiler.GetDangerZoneLowTemperature() + 1);
  }

  m_pvBoiler.SetDangerZoneHighTemperature(iTemperature);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDisinfectInterval(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iDays;
  result = get_int32_from_string(strArgs, &iDays, DISINFECTION_INTERVAL_DAYS_MIN, DISINFECTION_INTERVAL_DAYS_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetDisinfectionIntervalDays(iDays);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDisinfectDuration(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iMinutes;
  result = get_int32_from_string(strArgs, &iMinutes, DISINFECTION_DURATION_MINUTES_MIN, DISINFECTION_DURATION_MINUTES_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetDisinfectionDurationMinutes(iMinutes);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDisinfectTemperature(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iTemperature;
  result = get_int32_from_string(strArgs, &iTemperature, DISINFECTION_TEMPERATURE_MIN, DISINFECTION_TEMPERATURE_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetDisinfectionTemperature(iTemperature);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetBoilerPowerRating(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iPower;
  result = get_int32_from_string(strArgs, &iPower, BOILER_POWER_RATING_MIN, BOILER_POWER_RATING_MAX, ARG_INT32_NUM1);
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


result_code_t CPvBoilerCommandHandler::CmdSetBudgetMargin(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iMargin;
  result = get_int32_from_string(strArgs, &iMargin, BUDGET_MARGIN_MIN, BUDGET_MARGIN_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetBudgetMargin(iMargin);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetMode(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  if (STRIEQUALS(strArgs, "percent") || STRIEQUALS(strArgs, "percentage") || STRIEQUALS(strArgs, "p"))
    m_pvBoiler.SetMode(CPvBoiler::MODE_PERCENT);
  else if (STRIEQUALS(strArgs, "budget") || STRIEQUALS(strArgs, "b"))
    m_pvBoiler.SetMode(CPvBoiler::MODE_BUDGET);
  else if (STRIEQUALS(strArgs, "percent_sp") || STRIEQUALS(strArgs, "psp"))
    m_pvBoiler.SetMode(CPvBoiler::MODE_PERCENT_SETPOINT);
  else if (STRIEQUALS(strArgs, "budget_sp") || STRIEQUALS(strArgs, "bsp"))
    m_pvBoiler.SetMode(CPvBoiler::MODE_BUDGET_SETPOINT);
  else if (STRIEQUALS(strArgs, "sp_budget") || STRIEQUALS(strArgs, "spb"))
    m_pvBoiler.SetMode(CPvBoiler::MODE_SETPOINT_BUDGET);
  else if (STRIEQUALS(strArgs, "percent_sp_budget") || STRIEQUALS(strArgs, "pspb"))
    m_pvBoiler.SetMode(CPvBoiler::MODE_PERCENT_SETPOINT_BUDGET);
  else if (STRIEQUALS(strArgs, "standby") || STRIEQUALS(strArgs, "s"))
    m_pvBoiler.SetMode(CPvBoiler::MODE_STANDBY);
  else if (STRIEQUALS(strArgs, "off") || STRIEQUALS(strArgs, "0"))
    m_pvBoiler.SetMode(CPvBoiler::MODE_OFF);
  else if (STRIEQUALS(strArgs, "setpoint") || STRIEQUALS(strArgs, "sp"))
    m_pvBoiler.SetMode(CPvBoiler::MODE_SETPOINT);
  else if (STRIEQUALS(strArgs, "boost"))
    m_pvBoiler.SetMode(CPvBoiler::MODE_BOOST);
  else
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

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

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetSsrPeriodCount(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  int32_t iCount;
  result = get_int32_from_string(strArgs, &iCount, SSR_PERIOD_COUNT_MIN, SSR_PERIOD_COUNT_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetSsrPeriodCount(iCount);

  return pack_result_code(ERR_CODE_OK);
}



result_code_t CPvBoilerCommandHandler::CmdSetErrorGainPos(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  double fGain;
  result = get_double_from_string(strArgs, &fGain, ERROR_GAIN_POS_MIN, ERROR_GAIN_POS_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetErrorGainPos(fGain);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetErrorGainNeg(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  double fGain;
  result = get_double_from_string(strArgs, &fGain, ERROR_GAIN_NEG_MIN, ERROR_GAIN_NEG_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetErrorGainNeg(fGain);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetStepClampPos(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  double fClamp;
  result = get_double_from_string(strArgs, &fClamp, STEP_CLAMP_POS_MIN, STEP_CLAMP_POS_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetStepClampPos(fClamp);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetStepClampNeg(const char *strArgs)
{
  result_code_t result = check_arguments(strArgs, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  double fClamp;
  result = get_double_from_string(strArgs, &fClamp, STEP_CLAMP_NEG_MIN, STEP_CLAMP_NEG_MAX, ARG_INT32_NUM1);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetStepClampNeg(fClamp);

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
    if (strArgs == NULL || !*strArgs)
      m_network.GetMqttClient().ServerDisconnect();

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
  else if (STRIEQUALS(strCommand, "echo"))
  {
    result = CmdEchoOnOff(strArgs);
  }
  else if (STRIEQUALS(strCommand, "budget"))
  {
    result = CmdSetPowerBudget(strArgs);
  }
  else if (STRIEQUALS(strCommand, "budgetsrc"))
  {
    result = CmdSetBudgetSource(strArgs);
  }
  else if (STRIEQUALS(strCommand, "percent"))
  {
    result = CmdSetPowerPercentage(strArgs);
  }
  else if (STRIEQUALS(strCommand, "thermsp") || STRIEQUALS(strCommand, "tsp"))
  {
    result = CmdSetThermostatSetpoint(strArgs);
  }
  else if (STRIEQUALS(strCommand, "thermhyst") || STRIEQUALS(strCommand, "thyst"))
  {
    result = CmdSetThermostatHysteresis(strArgs);
  }
  else if (STRIEQUALS(strCommand, "disinfen") || STRIEQUALS(strCommand, "disinfect"))
  {
    result = CmdSetDisinfectionEnabled(strArgs);
  }
  else if (STRIEQUALS(strCommand, "disinfint") || STRIEQUALS(strCommand, "disinfinterval"))
  {
    result = CmdSetDisinfectInterval(strArgs);
  }
  else if (STRIEQUALS(strCommand, "disinfdur") || STRIEQUALS(strCommand, "disinfduration"))
  {
    result = CmdSetDisinfectDuration(strArgs);
  }
  else if (STRIEQUALS(strCommand, "disinftemp"))
  {
    result = CmdSetDisinfectTemperature(strArgs);
  }
  else if (STRIEQUALS(strCommand, "dzlow"))
  {
    result = CmdSetDangerZoneLowTemperature(strArgs);
  }
  else if (STRIEQUALS(strCommand, "dzhigh"))
  {
    result = CmdSetDangerZoneHighTemperature(strArgs);
  }
  else if (STRIEQUALS(strCommand, "boiler"))
  {
    result = CmdSetBoilerPowerRating(strArgs);
  }
  else if (STRIEQUALS(strCommand, "deadzone"))
  {
    result = CmdSetDeadZone(strArgs);
  }
  else if (STRIEQUALS(strCommand, "bmargin"))
  {
    result = CmdSetBudgetMargin(strArgs);
  }
  else if (STRIEQUALS(strCommand, "mode"))
  {
    result = CmdSetMode(strArgs);
  }
  else if (STRIEQUALS(strCommand, "dimstyle"))
  {
    result = CmdSetDimStyle(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ssrperiod"))
  {
    result = CmdSetSsrPeriodCount(strArgs);
  }
  else if (STRIEQUALS(strCommand, "egp") || STRIEQUALS(strCommand, "egainp"))
  {
    result = CmdSetErrorGainPos(strArgs);
  }
  else if (STRIEQUALS(strCommand, "egn") || STRIEQUALS(strCommand, "egainn"))
  {
    result = CmdSetErrorGainNeg(strArgs);
  }
  else if (STRIEQUALS(strCommand, "scp") || STRIEQUALS(strCommand, "sclampp"))
  {
    result = CmdSetStepClampPos(strArgs);
  }
  else if (STRIEQUALS(strCommand, "scn") || STRIEQUALS(strCommand, "sclampn"))
  {
    result = CmdSetStepClampNeg(strArgs);
  }
  else if (STRIEQUALS(strCommand, "hostname") || STRIEQUALS(strCommand, "name"))
  {
    result = CmdSetHostName(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ssid") || STRIEQUALS(strCommand, "wssid") || STRIEQUALS(strCommand, "wifissid"))
  {
    result = CmdSetWifiSsid(strArgs);
  }
  else if (STRIEQUALS(strCommand, "pass") || STRIEQUALS(strCommand, "wpass") || STRIEQUALS(strCommand, "wifipass"))
  {
    result = CmdSetWifiPassword(strArgs);
  }
  else if (STRIEQUALS(strCommand, "ipaddr") || STRIEQUALS(strCommand, "ip"))
  {
    result = CmdSetIpAddress(strArgs);
  }
  else if (STRIEQUALS(strCommand, "netmask"))
  {
    result = CmdSetNetMask(strArgs);
  }
  else if (STRIEQUALS(strCommand, "serverip") || STRIEQUALS(strCommand, "mqttip"))
  {
    result = CmdSetMqttIpAddress(strArgs);
  }
  else if (STRIEQUALS(strCommand, "mqttuser") || STRIEQUALS(strCommand, "muser"))
  {
    result = CmdSetMqttUser(strArgs);
  }
  else if (STRIEQUALS(strCommand, "mqttpass") || STRIEQUALS(strCommand, "mpass"))
  {
    result = CmdSetMqttPassword(strArgs);
  }
  else if (STRIEQUALS(strCommand, "mqttint") || STRIEQUALS(strCommand, "mqttinterval") || STRIEQUALS(strCommand, "minterval"))
  {
    result = CmdSetMqttUpdateInterval(strArgs);
  }
  else if (STRIEQUALS(strCommand, "restartnet"))
  {
    result = CmdRestartNet(strArgs);
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
