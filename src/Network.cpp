#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>
#include <ESPmDNS.h>
#endif
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

#include "Network.h"
#include "TermPrint.h"
#include "util.h"


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
  }

  if (strlen(m_strWifiSsid) == 0)
    return;

#ifdef WIFI_DEBUG
  // We start by connecting to a WiFi network
  CTermPrint::println("");
  CTermPrint::print("Connecting to ");
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
  WiFi.begin(m_strWifiSsid, m_strWifiPassword);

  // Initialize mDNS
  if (!MDNS.begin(HOST_NAME))
  {
    CTermPrint::println("ERROR: Unable to start MDNS responder!");
  }

  // Need to explicitly set hostname as ArduinoOTA will override our mdns-name set above
  ArduinoOTA.setHostname(HOST_NAME);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Init the socket server
  m_socketServer.begin();
  m_socketServer.setNoDelay(true);

#ifdef WIFI_DEBUG
  CTermPrint::println("Listing for socket connections on " STRINGIZE(SOCKET_SERVER_PORT));
#endif
}


void CNetwork::SetWifiSsid(const char* strSsid)
{
  memset(m_strWifiSsid, 0x00, WIFI_SSID_MAX_SIZE + 1);
  strcpy(m_strWifiSsid, strSsid);

  EEPROM.put(EEPROM_WIFI_SSID, m_strWifiSsid);
  EEPROM.commit();

  InitWifi(true);
}


void CNetwork::SetWifiPassword(const char* strPassword)
{
  memset(m_strWifiPassword, 0x00, WIFI_PASSWORD_MAX_SIZE + 1);
  strcpy(m_strWifiPassword, strPassword);

  EEPROM.put(EEPROM_WIFI_PASSWORD, m_strWifiPassword);
  EEPROM.commit();
}


void CNetwork::SetIpAddr(uint8_t* ipAddress)
{
  memcpy(m_ipAddr, ipAddress, sizeof(m_ipAddr));

  EEPROM.put(EEPROM_IP_ADDR, m_ipAddr);
  EEPROM.commit();

  InitWifi(true);
}


void CNetwork::SetNetMask(uint8_t* ipNetMask)
{
  memcpy(m_ipNetmask, ipNetMask, sizeof(m_ipNetmask));

  EEPROM.put(EEPROM_IP_NETMASK, m_ipNetmask);
  EEPROM.commit();

  InitWifi(true);
}


void CNetwork::SetServerIp(uint8_t* ipAddress)
{
  memcpy(m_serverIpAddr, ipAddress, sizeof(m_serverIpAddr));

  EEPROM.put(EEPROM_SERVER_IP_ADDR, m_serverIpAddr);
  EEPROM.commit();

  m_MQTTClient.setServer(m_serverIpAddr, MQTT_PORT);
}
