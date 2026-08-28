#pragma once
#ifndef PVBOILER_COMMAND_HANDLER_H
#define PVBOILER_COMMAND_HANDLER_H

#include "CommandHandler.h"
#include "CommandParser.h"

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  using NetClient = WiFiClient;
#elif defined(ESP32)
  #include <WiFi.h>
  using NetClient = WiFiClient;
#elif defined(ARDUINO_TEENSY41)
  #include <NativeEthernet.h>
  using NetClient = EthernetClient;
#endif

// Forward declare
class CPvBoiler; 
class CNetwork;

class CPvBoilerCommandHandler : public CCommandHandler
{
  public:
    CPvBoilerCommandHandler(CPvBoiler& pvBoiler, CNetwork& network) : m_pvBoiler(pvBoiler), m_network(network) {};

    result_code_t ProcessCommand(char *strCommand);

  private:
    CPvBoiler& m_pvBoiler;
    CNetwork& m_network;

    result_code_t CmdShowVersion(const char *strArgs);
    result_code_t CmdShowFWVersion(const char *strArgs);
    result_code_t CmdShowHelp(const char *strArgs);
    result_code_t CmdShowExpertHelp(const char *strArgs);

    result_code_t CmdInfo(const char *strArgs);
    result_code_t CmdStatus(const char *strArgs);
    result_code_t CmdUpTime(const char *strArgs);
    result_code_t CmdReset(const char *strArgs);
    result_code_t CmdFactoryReset(const char *strArgs);

    result_code_t CmdSetWifiSsid(const char *strArgs);
    result_code_t CmdSetWifiPassword(const char *strArgs);
    result_code_t CmdSetIpAddress(const char *strArgs);
    result_code_t CmdSetNetMask(const char *strArgs);

    result_code_t CmdSetMqttIpAddress(const char *strArgs);
    result_code_t CmdSetMqttUser(const char *strArgs);
    result_code_t CmdSetMqttPassword(const char *strArgs);
    result_code_t CmdSetMqttUpdateInterval(const char *strArgs);

    result_code_t CmdNetWatchdogTimeout(const char *strArgs);
    result_code_t CmdNetWatchdogRecovery(const char *strArgs);
    result_code_t CmdRestartNet(const char *strArgs);

    result_code_t CmdEnable(const char *strArgs);
    result_code_t CmdDisable(const char *strArgs);

    result_code_t CmdSetPowerBudget(const char *strArgs);
    result_code_t CmdSetPowerPercentage(const char *strArgs);
    result_code_t CmdSetPowerBoost(const char *strArgs);

    result_code_t CmdSetBoilerPowerRating(const char *strArgs);
    result_code_t CmdSetDeadZone(const char *strArgs);
    result_code_t CmdSetBudgetMargin(const char *strArgs);
    result_code_t CmdSetLogicMode(const char *strArgs);
    result_code_t CmdSetDimStyle(const char *strArgs);
    result_code_t CmdSetSsrPeriodCount(const char *strArgs);
    result_code_t CmdSetErrorGain(const char *strArgs);
    result_code_t CmdSetStepClamp(const char *strArgs);
};
#endif // PVBOILER_COMMAND_HANDLER_H
