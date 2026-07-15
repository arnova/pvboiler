#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>
#include <ESPmDNS.h>
#endif
#include <ArduinoOTA.h>
#include <EEPROM.h>

#include "Network.h"
#include "MqttClient.h"
#include "TermPrint.h"
#include "util.h"


CNetwork::CNetwork()
#ifdef SOCKET_SERVER_PORT
  : m_socketServer(SOCKET_SERVER_PORT)
#endif
{
  m_mqttClient.setClient(m_wifiClient);
}


void CNetwork::LoadSettings()
{
    // Obtain our IP
  EEPROM.get(EEPROM_IP_ADDR, m_ipAddr);
  if (IPAddress(m_ipAddr) == IPAddress(255, 255, 255, 255))
  {
    memset(m_ipAddr, 0x00, 4);
  }

  EEPROM.get(EEPROM_IP_NETMASK, m_ipNetmask);

  EEPROM.get(EEPROM_SERVER_IP_ADDR, m_serverIpAddr);
  if (IPAddress(m_serverIpAddr) == IPAddress(255, 255, 255, 255))
  {
    memset(m_serverIpAddr, 0x00, 4);
  }

  EEPROM.get(EEPROM_WIFI_SSID, m_strWifiSsid);
  EEPROM.get(EEPROM_WIFI_PASSWORD, m_strWifiPassword);
}


void CNetwork::InitWifi(const bool bReconnect)
{
  if (bReconnect)
  {
    WiFi.disconnect();
    //WiFi.reconnect();
  }

  if (strlen(m_strWifiSsid) == 0)
    return;

#ifdef WIFI_DEBUG
  // We start by connecting to a WiFi network
  CTermPrint::println("");
  if (bReconnect)
  {
    CTermPrint::print(PSTR("Reconnecting to WiFi network: "));
  }
  else
  {
    CTermPrint::print(PSTR("Connecting to WiFi network: "));
  }
  CTermPrint::println(m_strWifiSsid);
#endif

  // Check for dhcp ip
  if (IPAddress(m_ipAddr) != IPAddress(0, 0, 0, 0))
  {
    // Static IP. NOTE: No gateway / dns
    WiFi.config(m_ipAddr, 0, m_ipNetmask);
  }
  else
  {
    // DHCP IP
    WiFi.config(0, 0, 0);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOST_NAME);
  WiFi.begin(m_strWifiSsid, m_strWifiPassword);

  // Initialize mDNS
  if (!MDNS.begin(HOST_NAME))
  {
    CTermPrint::println("ERROR: Unable to start MDNS responder!");
  }

  // Need to explicitly set hostname as ArduinoOTA will override our mdns-name set above
  ArduinoOTA.setHostname(HOST_NAME);

  ArduinoOTA.onStart([]() {
    TERM_SERIAL.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    TERM_SERIAL.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    TERM_SERIAL.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    TERM_SERIAL.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) TERM_SERIAL.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) TERM_SERIAL.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) TERM_SERIAL.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) TERM_SERIAL.println("Receive Failed");
    else if (error == OTA_END_ERROR) TERM_SERIAL.println("End Failed");
  });
  ArduinoOTA.begin();

#ifdef SOCKET_SERVER_PORT
  // Init the socket server
  m_socketServer.begin();
  m_socketServer.setNoDelay(true);
  
#ifdef WIFI_DEBUG
  CTermPrint::println("Listening for terminal connections on TCP port " STRINGIZE(SOCKET_SERVER_PORT));
#endif
#endif
}


void CNetwork::EepromCommit()
{
  noInterrupts(); // Enter critical section
  EEPROM.commit();
  interrupts(); // Leave critical section
}


void CNetwork::SetWifiSsid(const char* strSsid)
{
  memset(m_strWifiSsid, 0x00, WIFI_SSID_MAX_SIZE + 1);
  strcpy(m_strWifiSsid, strSsid);

  EEPROM.put(EEPROM_WIFI_SSID, m_strWifiSsid);
  EepromCommit();

  InitWifi(true);
}


void CNetwork::SetWifiPassword(const char* strPassword)
{
  memset(m_strWifiPassword, 0x00, WIFI_PASSWORD_MAX_SIZE + 1);
  strcpy(m_strWifiPassword, strPassword);

  EEPROM.put(EEPROM_WIFI_PASSWORD, m_strWifiPassword);
  EepromCommit();
}


void CNetwork::SetIpAddr(uint8_t* ipAddress)
{
  memcpy(m_ipAddr, ipAddress, sizeof(m_ipAddr));

  EEPROM.put(EEPROM_IP_ADDR, m_ipAddr);
  EepromCommit();

  InitWifi(true);
}


void CNetwork::SetNetMask(uint8_t* ipNetMask)
{
  memcpy(m_ipNetmask, ipNetMask, sizeof(m_ipNetmask));

  EEPROM.put(EEPROM_IP_NETMASK, m_ipNetmask);
  EepromCommit();

  InitWifi(true);
}


void CNetwork::SetMqttServerIp(uint8_t* ipAddress)
{
  memcpy(m_serverIpAddr, ipAddress, sizeof(m_serverIpAddr));

  EEPROM.put(EEPROM_SERVER_IP_ADDR, m_serverIpAddr);
  EepromCommit();

  m_mqttClient.setServer(m_serverIpAddr, MQTT_PORT);
}

#ifdef SOCKET_SERVER_PORT
WiFiClient& CNetwork::GetSocketServerClient()
{
  if (!m_socketServerClient || !m_socketServerClient.connected())
  {
    m_socketServerClient = m_socketServer.accept(); // Check for new client connections
#ifdef WIFI_DEBUG
    if (m_socketServerClient)
    {
      CTermPrint::print("Accepting connection from client: ");
      CTermPrint::println(m_socketServerClient.remoteIP().toString().c_str());
    }
#endif
  }

  return m_socketServerClient;
}
#endif