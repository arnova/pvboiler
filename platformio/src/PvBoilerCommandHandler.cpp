#include "PvBoilerCommandHandler.h"
#include "Uptime.h"
#include "Terminal.h"
#include "PvBoiler.h"
#include "util.h"
#include "Network.h"
#include "system.h"
#include "SettingLimits.h"

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
  const result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  CTerminal::println(FPSTR(HELP_STR_P));

  return pack_result_code(ERR_CODE_OK);
}


// Show expert help screen
result_code_t CPvBoilerCommandHandler::CmdShowExpertHelp(const char *strArgs)
{
  const result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

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
  int32_t iInterval;
  const result_code_t result = parse_decimal_arg(strArgs, MQTT_UPDATE_TIME_MIN, MQTT_UPDATE_TIME_MAX, &iInterval);
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
  const result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  // Output result before restarting, else it's lost on a socket (terminal) connection
  CTerminal::println("OK");

  delay(3000); // Delay to make sure network socket processing is done

  m_network.InitWifi(true);

  return pack_result_code(ERR_CODE_OK_NULL); // Result already printed
}


result_code_t CPvBoilerCommandHandler::CmdInfo(const char *strArgs)
{
  const result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  PrintItemValue("host_name", m_network.GetHostName());
  PrintItemValue(" wifi_ssid", m_network.GetWifiSsid());

  PrintItemSecret(" wifi_pass", m_network.GetWifiPassword());

  PrintItemIp(" ip", m_network.GetIpAddr());
  PrintItemIp(" netmask", m_network.GetNetMask());
  PrintItemIp(" mqtt_server", m_network.GetMqttIpAddr());

  const char* strMqttUser = m_network.GetMqttUser();
  PrintItemValue(" mqtt_user", (strlen(strMqttUser) == 0) ? "(empty)" : strMqttUser);

  PrintItemSecret(" mqtt_password", m_network.GetMqttPassword());

  CTerminal::println("");

  PrintItemValue("boiler_rating", m_pvBoiler.GetBoilerPowerRating(), "W");
  PrintItemValue(" dim_style", m_pvBoiler.GetDimStyle() == CPvBoiler::DIM_STYLE_PHASE_ANGLE ? "phase-angle" : "ssr");
  PrintItemValue(" ssr_period_count", m_pvBoiler.GetSsrPeriodCount());

  CTerminal::println("");

  PrintItemValue("budget_src", m_pvBoiler.GetDsmrPowerBudget() ? "dsmr-reader" : "direct");
  PrintItemValue(" error_gain_pos", m_pvBoiler.GetErrorGainPos(), 3);
  PrintItemValue(" error_gain_neg", m_pvBoiler.GetErrorGainNeg(), 3);
  PrintItemValue(" step_clamp_pos", m_pvBoiler.GetStepClampPos(), 2, "%");
  PrintItemValue(" step_clamp_neg", m_pvBoiler.GetStepClampNeg(), 2, "%");
  PrintItemValue(" dead_zone", m_pvBoiler.GetDeadZone(), "W");
  PrintItemValue(" budget_margin", m_pvBoiler.GetBudgetMargin(), "W");

  CTerminal::println("");

  PrintItemValue("thermostat_hysteresis", m_pvBoiler.GetThermostatHysteresis(), "C");
  PrintItemValue(" danger_zone_low", m_pvBoiler.GetDangerZoneLowTemperature(), "C");
  PrintItemValue(" danger_zone_high", m_pvBoiler.GetDangerZoneHighTemperature(), "C");
  PrintItemValue(" disinfect_interval", m_pvBoiler.GetDisinfectionIntervalDays(), "d");
  PrintItemValue(" disinfect_duration", m_pvBoiler.GetDisinfectionDurationMinutes(), "m");
  PrintItemValue(" disinfect_temp", m_pvBoiler.GetDisinfectionTemperature(), "C");

  CTerminal::println("");

  PrintItemValue("mqtt_update_interval", m_pvBoiler.GetMqttUpdateInterval(), "s");
  PrintItemValue(" net_wd_timeout", m_pvBoiler.GetNetWatchDogTimeout(), "s");
  PrintItemValue(" net_wd_recovery", m_pvBoiler.GetNetWatchDogRecovery(), "s");

  CTerminal::println("");
  CTerminal::println("");

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdStatus(const char *strArgs)
{
  const result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  PrintItemValue("wifi_conn", m_network.IsConnected());
  PrintItemValue(" mqtt_conn", m_network.IsMqttConnected());
  PrintItemIp(" wifi_ip", WiFi.localIP());
  PrintItemIp(" wifi_subnet", WiFi.subnetMask());

  CTerminal::println("");

  PrintItemValue("mode", CPvBoiler::m_modeNames[m_pvBoiler.GetMode()].strCli);
  PrintItemValue(" state", CPvBoiler::m_stateNames[m_pvBoiler.GetState()].strCli);

  PrintItemValue(" perc_set", m_pvBoiler.GetPowerPercentage(), "%");
  PrintItemValue(" budget_set", m_pvBoiler.GetPowerBudget(), "W");

  CTerminal::println("");

  PrintItemValue("power_out", m_pvBoiler.GetCurrentPower(), "W");
  PrintItemValue(" perc_out", m_pvBoiler.GetCurrentPercentage(), 2, "%");

  CTerminal::println("");

  if (m_pvBoiler.GetBoilerTemperature() > 0.0f)
  {
    PrintItemValue("boiler_t", m_pvBoiler.GetBoilerTemperature(), 1, "C");
  }
  else
  {
    PrintItemValue("boiler_t", "n/a");
  }

  PrintItemValue(" over_heated", m_pvBoiler.IsBoilerOverheated());
  PrintItemValue(" thermostat_set", m_pvBoiler.GetThermostatSetpoint(), m_pvBoiler.IsThermostatMode() ? "C" : "C(off)");
  PrintItemValue(" thermostat_allows_heating", m_pvBoiler.GetThermostatAllowsHeating());

  CTerminal::println("");

  PrintItemValue("disinfection_en", m_pvBoiler.GetDisinfectionEnabled() ? "on" : "off");
  PrintItemValue(" disinfection_required", m_pvBoiler.GetLegionellaDisinfectionRequired());
  PrintItemValue(" in_danger_zone", m_pvBoiler.GetLegionellaInDangerZoneHours(), "h");
  PrintItemValue(" disinfection_age", m_pvBoiler.GetLegionellaDisinfectionAgeHours(), "h");
  PrintItemValue(" disinfecting_elapsed", m_pvBoiler.GetLegionellaDisinfectingElapsedMinutes(), "m");

  CTerminal::println("");

  const uint16_t iPeriod = m_pvBoiler.GetMainsPeriod();
  if (!m_pvBoiler.IsMainsPeriodValid())
  {
    PrintItemValue("mains_period", "?ms");
    PrintItemValue(" mains_freq", "?Hz");
  }
  else
  {
    PrintItemValue("mains_period", static_cast<float>(iPeriod) / 500.0f, 3, "ms");
    PrintItemValue(" mains_freq", (500.0f * 1000.0f) / static_cast<float>(iPeriod), 2, "Hz");
  }

  const uint16_t iWindow = m_pvBoiler.GetMainsZeroCrossWindow();
  if (!m_pvBoiler.IsMainsZeroCrossWindowValid())
  {
    PrintItemValue(" mains_zero_cross_window", "?ms");
  }
  else
  {
    PrintItemValue(" mains_zero_cross_window", static_cast<float>(iWindow) / 1000.0f, 3, "ms");
  }

  PrintItemValue(" mains_error", !m_pvBoiler.GetPowerGoodFlag());

  CTerminal::println("");

  if (m_pvBoiler.GetDimStyle() == CPvBoiler::DIM_STYLE_PHASE_ANGLE)
  {
    PrintItemValue("triac_phase_angle", static_cast<float>(m_pvBoiler.GetTriacPhaseAngle()) / 1000.0f, 3, "ms");
    PrintItemValue(" triac_angle_factor", m_pvBoiler.GetTriacAngleFactor(), 4);

    CTerminal::println("");
  }

  CTerminal::println("");

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdUpTime(const char *strArgs)
{
  const result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  // Publish uptime
  const CUptime::uptime_t upTime = m_pvBoiler.GetUpTime();
  char strTemp[24];
  snprintf(strTemp, sizeof(strTemp), "%ud %02u:%02u:%02u", upTime.iDays, upTime.iHours, upTime.iMinutes, upTime.iSeconds);
  CTerminal::println(strTemp);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdReset(const char *strArgs)
{
  const result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.Reset();
  m_pvBoiler.MqttRequestRepublish(); // Publish all values directly instead of waiting for the update interval

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdReboot(const char *strArgs)
{
  const result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  // Clean disconnect so we report offline
  m_network.GetMqttClient().ServerDisconnect();

  CTerminal::println("OK");

  delay(3000); // Delay to make sure network socket processing is done

  ESP.restart();

  // In principle anything from here on is not executed

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdFactoryReset(const char *strArgs)
{
  const result_code_t result = check_arguments(strArgs, ARG_INT32_NONE);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.FactoryReset();
  m_pvBoiler.MqttRequestRepublish(); // Publish all values directly instead of waiting for the update interval

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetPowerBudget(const char *strArgs)
{
  int32_t iPower;
  const result_code_t result = parse_decimal_arg(strArgs, INT32_MIN, INT32_MAX, &iPower);
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
  int32_t iPerc;
  const result_code_t result = parse_decimal_arg(strArgs, 0, 100, &iPerc);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetPowerPercentage(iPerc);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetThermostatSetpoint(const char *strArgs)
{
  int32_t iTemperature;
  const result_code_t result = parse_decimal_arg(strArgs, THERMOSTAT_SETPOINT_MIN, THERMOSTAT_SETPOINT_MAX, &iTemperature);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetThermostatSetpoint(iTemperature);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDisinfectionEnabled(const char *strArgs)
{
  bool bEnable;
  const result_code_t result = parse_bool_arg(strArgs, &bEnable);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetDisinfectionEnabled(bEnable);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetThermostatHysteresis(const char *strArgs)
{
  int32_t iHysteresis;
  const result_code_t result = parse_decimal_arg(strArgs, THERMOSTAT_HYSTERESIS_MIN, THERMOSTAT_HYSTERESIS_MAX, &iHysteresis);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetThermostatHysteresis(iHysteresis);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDangerZoneLowTemperature(const char *strArgs)
{
  int32_t iTemperature;
  const result_code_t result = parse_decimal_arg(strArgs, DANGER_ZONE_TEMPERATURE_MIN, DANGER_ZONE_TEMPERATURE_MAX, &iTemperature);
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
  int32_t iTemperature;
  const result_code_t result = parse_decimal_arg(strArgs, DANGER_ZONE_TEMPERATURE_MIN, DANGER_ZONE_TEMPERATURE_MAX, &iTemperature);
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
  int32_t iDays;
  const result_code_t result = parse_decimal_arg(strArgs, DISINFECTION_INTERVAL_DAYS_MIN, DISINFECTION_INTERVAL_DAYS_MAX, &iDays);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetDisinfectionIntervalDays(iDays);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDisinfectDuration(const char *strArgs)
{
  int32_t iMinutes;
  const result_code_t result = parse_decimal_arg(strArgs, DISINFECTION_DURATION_MINUTES_MIN, DISINFECTION_DURATION_MINUTES_MAX, &iMinutes);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetDisinfectionDurationMinutes(iMinutes);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDisinfectTemperature(const char *strArgs)
{
  int32_t iTemperature;
  const result_code_t result = parse_decimal_arg(strArgs, DISINFECTION_TEMPERATURE_MIN, DISINFECTION_TEMPERATURE_MAX, &iTemperature);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetDisinfectionTemperature(iTemperature);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetBoilerPowerRating(const char *strArgs)
{
  int32_t iPower;
  const result_code_t result = parse_decimal_arg(strArgs, BOILER_POWER_RATING_MIN, BOILER_POWER_RATING_MAX, &iPower);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetBoilerPowerRating(iPower);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetDeadZone(const char *strArgs)
{
  int32_t iDeadZone;
  const result_code_t result = parse_decimal_arg(strArgs, DEAD_ZONE_MIN, DEAD_ZONE_MAX, &iDeadZone);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetDeadZone(iDeadZone);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetBudgetMargin(const char *strArgs)
{
  int32_t iMargin;
  const result_code_t result = parse_decimal_arg(strArgs, BUDGET_MARGIN_MIN, BUDGET_MARGIN_MAX, &iMargin);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetBudgetMargin(iMargin);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetMode(const char *strArgs)
{
  if (strArgs == NULL || !*strArgs)
    return pack_result_code(ERR_CODE_ARG_MISSING, ARG_INT32_NUM1);

  CPvBoiler::mode_t mode;
  if (!CPvBoiler::GetModeFromCliName(strArgs, mode))
    return pack_result_code(ERR_CODE_ARG_VAL, ARG_INT32_NUM1);

  m_pvBoiler.SetMode(mode);

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
  int32_t iCount;
  const result_code_t result = parse_decimal_arg(strArgs, SSR_PERIOD_COUNT_MIN, SSR_PERIOD_COUNT_MAX, &iCount);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetSsrPeriodCount(iCount);

  return pack_result_code(ERR_CODE_OK);
}



result_code_t CPvBoilerCommandHandler::CmdSetErrorGainPos(const char *strArgs)
{
  double fGain;
  const result_code_t result = parse_double_arg(strArgs, ERROR_GAIN_POS_MIN, ERROR_GAIN_POS_MAX, &fGain);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetErrorGainPos(fGain);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetErrorGainNeg(const char *strArgs)
{
  double fGain;
  const result_code_t result = parse_double_arg(strArgs, ERROR_GAIN_NEG_MIN, ERROR_GAIN_NEG_MAX, &fGain);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetErrorGainNeg(fGain);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetStepClampPos(const char *strArgs)
{
  double fClamp;
  const result_code_t result = parse_double_arg(strArgs, STEP_CLAMP_POS_MIN, STEP_CLAMP_POS_MAX, &fClamp);
  if (result.code != ERR_CODE_OK)
    return result;

  m_pvBoiler.SetStepClampPos(fClamp);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CPvBoilerCommandHandler::CmdSetStepClampNeg(const char *strArgs)
{
  double fClamp;
  const result_code_t result = parse_double_arg(strArgs, STEP_CLAMP_NEG_MIN, STEP_CLAMP_NEG_MAX, &fClamp);
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
  int32_t iTime;
  const result_code_t result = parse_decimal_arg(strArgs, 0, NETWORK_WATCHDOG_RECOVERY_MAX, &iTime);
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
