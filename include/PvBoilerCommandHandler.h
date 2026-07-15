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
class CPVBoiler; 
class CNetwork;

class CPVBoilerCommandHandler : public CCommandHandler
{
  public:
    CPVBoilerCommandHandler(CPVBoiler& pvBoiler, CNetwork& network) : m_pvBoiler(pvBoiler), m_network(network) {};

    result_code_t ProcessCommand(char *strCommand);

  private:
    CPVBoiler& m_pvBoiler;
    CNetwork& m_network;

    result_code_t CmdShowVersion(const char *strArgs);
    result_code_t CmdShowFWVersion(const char *strArgs);
    result_code_t CmdShowHelp(const char *strArgs);

    result_code_t CmdInfo(const char *strArgs);
    result_code_t CmdStatus(const char *strArgs);
    result_code_t CmdReset(const char *strArgs);

    result_code_t CmdIpAddress(const char *strArgs);
    result_code_t CmdNetMask(const char *strArgs);
    result_code_t CmdServerIp(const char *strArgs);
    result_code_t CmdWifiSsid(const char *strArgs);
    result_code_t CmdWifiPassword(const char *strArgs);
    result_code_t CmdRestartNet(const char *strArgs);

    result_code_t CmdBoilerPowerRating(const char *strArgs);
    result_code_t CmdPowerBudgetMargin(const char *strArgs);
    result_code_t CmdLogicMode(const char *strArgs);
    result_code_t CmdDimStyle(const char *strArgs);
    result_code_t CmdSsrPeriodCount(const char *strArgs);
    result_code_t CmdErrorGain(const char *strArgs);
};
#endif // PVBOILER_COMMAND_HANDLER_H
