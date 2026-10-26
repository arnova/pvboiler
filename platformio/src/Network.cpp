/* 
  Network Class
  (C) Copyright 2026

  Written by       : Arno van Amersfoort
  Dependencies     : elapsedMillis MqttClient Terminal util
  Initial date     : July 11, 2026
  Last modified    : October 19, 2026
*/

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ctype.h>

#include "Network.h"
#include "Terminal.h"
#include "util.h"
#include "EepromSettings.h"
#include "EepromLayout.h"
#include "MqttTopics.h"


CNetwork::CNetwork()
#ifdef SOCKET_SERVER_PORT
  : m_socketServer(SOCKET_SERVER_PORT)
#endif
{
}


void CNetwork::Init()
{
  LoadSettings();

  InitWifi(false);

  m_mqttClient.setClient(m_wifiClient);

  if (IPAddress(m_mqttIpAddr) != IPAddress(0, 0, 0, 0))
  {
    m_mqttClient.Init(m_mqttIpAddr, m_strHostName, m_strMqttUser, m_strMqttPassword);
  }
}


bool CNetwork::IsValidHostName(const char* str)
{
  // Hostname is also used for mDNS & MQTT topics: only allow letters, digits & '-' (not at start/end)
  const size_t iLen = strlen(str);
  if (iLen == 0 || str[0] == '-' || str[iLen - 1] == '-')
  {
    return false;
  }

  for (; *str; ++str)
  {
    if (!isalnum((unsigned char) *str) && *str != '-')
    {
      return false;
    }
  }

  return true;
}


const char* CNetwork::FormatIp(const IPAddress& ip, char* strBuf, const size_t iBufSize)
{
  snprintf(strBuf, iBufSize, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return strBuf;
}


void CNetwork::LoadSettings()
{
  CEepromSettings::LoadString(EEPROM_HOST_NAME, m_strHostName, HOST_NAME_MAX_SIZE);
  if (!IsValidHostName(m_strHostName))
  {
    strcpy(m_strHostName, DEFAULT_HOST_NAME);
  }

  CEepromSettings::LoadIp(EEPROM_IP_ADDR, m_ipAddr);

  // Note: Netmask is used as is (also when erased)
  EEPROM.get(EEPROM_IP_NETMASK, m_ipNetmask);

  CEepromSettings::LoadIp(EEPROM_MQTT_IP_ADDR, m_mqttIpAddr);

  CEepromSettings::LoadString(EEPROM_WIFI_SSID, m_strWifiSsid, WIFI_SSID_MAX_SIZE);
  CEepromSettings::LoadString(EEPROM_WIFI_PASSWORD, m_strWifiPassword, WIFI_PASSWORD_MAX_SIZE);
  CEepromSettings::LoadString(EEPROM_MQTT_USER, m_strMqttUser, MQTT_USER_MAX_SIZE);
  CEepromSettings::LoadString(EEPROM_MQTT_PASSWORD, m_strMqttPassword, MQTT_PASSWORD_MAX_SIZE);
}


void CNetwork::InitWifi(const bool bReconnect)
{
  m_wifiTimeoutTimer = 0;
  m_bWifiConnected = false;

  if (bReconnect)
  {
#ifdef WIFI_DEBUG
    //CTerminal::println("");
    //CTerminal::println(PSTR("Disconnecting WiFi"));
#endif
#ifdef SOCKET_SERVER_PORT
    if (m_socketServerClient)
    {
      m_socketServerClient.stop();
    }

    m_socketServer.stop();
#endif
    WiFi.disconnect();

    // Wait for the (asynchronous) disconnect to complete: WiFi.begin() won't (re)connect when it
    // still sees us as connected with the same SSID/password
    for (uint8_t it = 0; it < WIFI_DISCONNECT_TIMEOUT / 10 && WiFi.status() == WL_CONNECTED; it++)
    {
      delay(10);
    }
  }

  if (strlen(m_strWifiSsid) == 0)
    return;

#ifdef WIFI_DEBUG
  CTerminal::println("");
  CTerminal::print(PSTR("(Re)connecting to WiFi network: "));
  CTerminal::println(m_strWifiSsid);
#endif

  // NOTE: Must be set before WiFi.config()/WiFi.mode() as the hostname is applied when the STA interface starts
  WiFi.setHostname(m_strHostName);

  // Check for dhcp ip
  if (IPAddress(m_ipAddr) != IPAddress(0, 0, 0, 0))
  {
    // Static IP. NOTE: No gateway / dns
    WiFi.config(m_ipAddr, IPAddress(0, 0, 0, 0), m_ipNetmask);
  }
  else
  {
    // DHCP IP
    WiFi.config(IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0));
  }

  WiFi.mode(WIFI_STA);
  if (strlen(m_strWifiPassword) != 0)
  {
    WiFi.begin(m_strWifiSsid, m_strWifiPassword);
  }
  else
  {
    WiFi.begin(m_strWifiSsid);
  }
}


void CNetwork::SetHostName(const char* strHostName)
{
  CEepromSettings::StoreString(EEPROM_HOST_NAME, m_strHostName, HOST_NAME_MAX_SIZE, strHostName);
}


void CNetwork::SetWifiSsid(const char* strSsid)
{
  CEepromSettings::StoreString(EEPROM_WIFI_SSID, m_strWifiSsid, WIFI_SSID_MAX_SIZE, strSsid);
}


void CNetwork::SetWifiPassword(const char* strPassword)
{
  CEepromSettings::StoreString(EEPROM_WIFI_PASSWORD, m_strWifiPassword, WIFI_PASSWORD_MAX_SIZE, strPassword);
}


void CNetwork::SetIpAddr(const uint8_t* ipAddress)
{
  CEepromSettings::StoreIp(EEPROM_IP_ADDR, m_ipAddr, ipAddress);
}


void CNetwork::SetNetMask(const uint8_t* ipNetMask)
{
  CEepromSettings::StoreIp(EEPROM_IP_NETMASK, m_ipNetmask, ipNetMask);
}


void CNetwork::ReinitMqtt()
{
  // Disconnect from current server (reports offline)
  m_mqttClient.ServerDisconnect();

  m_mqttClient.Init(m_mqttIpAddr, m_strHostName, m_strMqttUser, m_strMqttPassword);
}


void CNetwork::SetMqttServerIp(const uint8_t* ipAddress)
{
  CEepromSettings::StoreIp(EEPROM_MQTT_IP_ADDR, m_mqttIpAddr, ipAddress);

  ReinitMqtt();
}


void CNetwork::SetMqttUser(const char* strUser)
{
  CEepromSettings::StoreString(EEPROM_MQTT_USER, m_strMqttUser, MQTT_USER_MAX_SIZE, strUser);

  ReinitMqtt();
}


void CNetwork::SetMqttPassword(const char* strPassword)
{
  CEepromSettings::StoreString(EEPROM_MQTT_PASSWORD, m_strMqttPassword, MQTT_PASSWORD_MAX_SIZE, strPassword);

  ReinitMqtt();
}


void CNetwork::MqttPublishValues()
{
  // Note: Values are static so only published on request
  if (!m_bMqttRepublish || !IsMqttConnected())
    return;

  m_mqttClient.PublishMessage(MQTT_WIFI_SSID, m_strWifiSsid);

  char strBuf[16]; // Enough room for a standard IPv4 address
  m_mqttClient.PublishMessage(MQTT_IP_ADDRESS, FormatIp(WiFi.localIP(), strBuf, sizeof(strBuf)));

//  snprintf(strBuf, sizeof(strBuf), "%u.%u.%u.%u", WiFi.net()[0], WiFi.net()[1], WiFi.net()[2], WiFi.net()[3]);
//  m_mqttClient.PublishMessage(MQTT_IP_NETMASK, strBuf);

  m_bMqttRepublish = false;
}


#ifdef SOCKET_SERVER_PORT
WiFiClient& CNetwork::GetSocketServerClient()
{
  // Check for new client connections
  WiFiClient newClient = m_socketServer.accept();

  if (newClient)
  {
    if (m_socketServerClient && m_socketServerClient.connected())
    {
#ifdef WIFI_DEBUG
      CTerminal::println("Terminating existing client connection for new client");
#endif
      m_socketServerClient.stop();
    }

    m_socketServerClient = newClient;

    if (m_socketServerClient)
    {
      CTerminal::SetSocketClient(m_socketServerClient);
#ifdef WIFI_DEBUG
      CTerminal::print("Accepting connection from client: ");

      char strBuf[16]; // Enough room for a standard IPv4 address
      CTerminal::println(FormatIp(m_socketServerClient.remoteIP(), strBuf, sizeof(strBuf)));
#endif
    }
  }

  return m_socketServerClient;
}
#endif


bool CNetwork::HandleMqttClient()
{
  // Handle MQTT client-server connection
  if (IPAddress(m_mqttIpAddr) != IPAddress(0, 0, 0, 0) && m_bWifiConnected)
  {
    if (m_mqttClient.connected())
    {
      m_mqttTimeoutTimer = 0;
    }
    else if (m_mqttTimeoutTimer > MQTT_CONNECT_TIMEOUT)
    {
      m_mqttTimeoutTimer = 0;

      if (m_mqttClient.ServerConnect())
      {
        return true; // Reconnected
      }
    }
  }

  return false; // No reconnection
}


void CNetwork::Loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!m_bWifiConnected)
    {
      // Need to explicitly set hostname as ArduinoOTA also handles mdns
      ArduinoOTA.setHostname(m_strHostName);

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

      m_bWifiConnected = true;

#ifdef WIFI_DEBUG
      TERM_SERIAL.println("");
      TERM_SERIAL.print(PSTR("WiFi connected with IP address: "));

      char strBuf[16]; // Enough room for a standard IPv4 address
      TERM_SERIAL.println(FormatIp(WiFi.localIP(), strBuf, sizeof(strBuf)));
#endif

#ifdef SOCKET_SERVER_PORT
      // Init the socket server
      m_socketServer.begin();
      m_socketServer.setNoDelay(true);

#ifdef WIFI_DEBUG
      TERM_SERIAL.println("Listening for terminal connections on TCP port: " STRINGIZE(SOCKET_SERVER_PORT));
#endif
#endif
    }

    // Handle OTA (& mdns) updates
    ArduinoOTA.handle();

    m_wifiTimeoutTimer = 0;
  }
  else
  {
    if (m_bWifiConnected)
    {
      // Connection lost
      m_bWifiConnected = false;

#ifdef WIFI_DEBUG
      TERM_SERIAL.println("");
      TERM_SERIAL.println(PSTR("WiFi connection lost"));
#endif
    }

    if (m_wifiTimeoutTimer > WIFI_CONNECT_TIMEOUT)
    {
      InitWifi(true);
    }
  }

  // Always perform mqtt loop to detect connection failures
  m_mqttClient.loop();

  MqttPublishValues();
}