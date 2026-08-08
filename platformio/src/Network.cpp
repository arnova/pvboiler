/* 
  Network Class
  (C) Copyright 2026

  Written by       : Arno van Amersfoort
  Dependencies     : elapsedMillis Network MqttClient Terminal util
  Initial date     : July 11, 2026
  Last modified    : July 30, 2026
*/

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
#include "Terminal.h"
#include "util.h"


CNetwork::CNetwork()
#ifdef SOCKET_SERVER_PORT
  : m_socketServer(SOCKET_SERVER_PORT)
#endif
{
}


void CNetwork::Init()
{
  m_mqttClient.setClient(m_wifiClient);
  LoadSettings();

  InitWifi(false);

  if (IPAddress(m_serverIpAddr) != IPAddress(0, 0, 0, 0))
  {
    m_mqttClient.Init(m_serverIpAddr);
  }
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
  m_wifiTimeoutTimer = 0;
  m_bWifiConnected = false;

  if (bReconnect)
  {
#ifdef WIFI_DEBUG
    //CTerminal::println("");
    //CTerminal::println(PSTR("Disconnecting WiFi"));
#endif

    if (m_socketServerClient)
    {
      m_socketServerClient.stop();
    }

    m_socketServer.stop();

    MDNS.end(); // Need to deinit MDNS (and init again below else it may stop working)

    WiFi.disconnect();
  }

  if (strlen(m_strWifiSsid) == 0)
    return;

#ifdef WIFI_DEBUG
  CTerminal::println("");
  CTerminal::print(PSTR("(Re)connecting to WiFi network: "));
  CTerminal::println(m_strWifiSsid);
#endif

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
  WiFi.setHostname(HOST_NAME);
  WiFi.begin(m_strWifiSsid, m_strWifiPassword);
}


void CNetwork::SetWifiSsid(const char* strSsid)
{
  if (!STREQUALS(strSsid, m_strWifiSsid))
  {
    memset(m_strWifiSsid, 0x00, WIFI_SSID_MAX_SIZE + 1);
    strcpy(m_strWifiSsid, strSsid);

    EEPROM.put(EEPROM_WIFI_SSID, m_strWifiSsid);
    EEPROM.commit();
  }

  InitWifi(true);
}


void CNetwork::SetWifiPassword(const char* strPassword)
{
  if (!STREQUALS(strPassword, m_strWifiPassword))
  {
    memset(m_strWifiPassword, 0x00, WIFI_PASSWORD_MAX_SIZE + 1);
    strcpy(m_strWifiPassword, strPassword);

    EEPROM.put(EEPROM_WIFI_PASSWORD, m_strWifiPassword);
    EEPROM.commit();
  }

  InitWifi(true);
}


void CNetwork::SetIpAddr(const uint8_t* ipAddress)
{
  if (memcmp(ipAddress, m_ipAddr, 4) != 0)
  {
    memcpy(m_ipAddr, ipAddress, sizeof(m_ipAddr));

    EEPROM.put(EEPROM_IP_ADDR, m_ipAddr);
    EEPROM.commit();
  }

  InitWifi(true);
}


void CNetwork::SetNetMask(const uint8_t* ipNetMask)
{
  if (memcmp(ipNetMask, m_ipNetmask, 4) != 0)
  {
    memcpy(m_ipNetmask, ipNetMask, sizeof(m_ipNetmask));

    EEPROM.put(EEPROM_IP_NETMASK, m_ipNetmask);
    EEPROM.commit();
  }

  InitWifi(true);
}


void CNetwork::MqttPublishValues()
{
  m_mqttClient.PublishMessage(MQTT_WIFI_SSID, m_strWifiSsid);

  char strBuf[16]; // Enough room for a standard IPv4 address
  snprintf(strBuf, sizeof(strBuf), "%u.%u.%u.%u", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  m_mqttClient.PublishMessage(MQTT_IP_ADDRESS, strBuf);

//  snprintf(strBuf, sizeof(strBuf), "%u.%u.%u.%u", WiFi.net()[0], WiFi.net()[1], WiFi.net()[2], WiFi.net()[3]);
//  m_mqttClient.PublishMessage(MQTT_IP_NETMASK, strBuf);
}


void CNetwork::SetMqttServerIp(const uint8_t* ipAddress)
{
  if (memcmp(ipAddress, m_serverIpAddr, 4) != 0)
  {
    memcpy(m_serverIpAddr, ipAddress, sizeof(m_serverIpAddr));

    EEPROM.put(EEPROM_SERVER_IP_ADDR, m_serverIpAddr);
    EEPROM.commit();
  }

  // Disconnect to current server
  if (m_mqttClient.connected())
  {
    m_mqttClient.disconnect();
  }

  m_mqttClient.Init(m_serverIpAddr);
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
      CTerminal::print("Accepting connection from client: ");

      char strBuf[16]; // Enough room for a standard IPv4 address
      snprintf(strBuf, sizeof(strBuf), "%u.%u.%u.%u", m_socketServerClient.remoteIP()[0], m_socketServerClient.remoteIP()[1], m_socketServerClient.remoteIP()[2], m_socketServerClient.remoteIP()[3]);
      CTerminal::println(strBuf);

      CTerminal::SetSocketClient(m_socketServerClient);
    }
#endif
  }

  return m_socketServerClient;
}
#endif


bool CNetwork::HandleMqttClient()
{
  // Handle MQTT client-server connection
  if (IPAddress(m_serverIpAddr) != IPAddress(0, 0, 0, 0) && m_bWifiConnected)
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
      // Initialize mDNS
      if (!MDNS.begin(HOST_NAME))
      {
        CTerminal::println("ERROR: Unable to start MDNS responder!");
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

      m_bWifiConnected = true;

#ifdef WIFI_DEBUG
      TERM_SERIAL.println("");
      TERM_SERIAL.print(PSTR("WiFi connected with IP address: "));

      char strBuf[16]; // Enough room for a standard IPv4 address
      snprintf(strBuf, sizeof(strBuf), "%u.%u.%u.%u", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
      TERM_SERIAL.println(strBuf);
#endif

#ifdef SOCKET_SERVER_PORT
      // Init the socket server
      m_socketServer.begin();
      m_socketServer.setNoDelay(true);

#ifdef WIFI_DEBUG
      CTerminal::println("Listening for terminal connections on TCP port: " STRINGIZE(SOCKET_SERVER_PORT));
#endif
#endif
    }

    // Handle OTA-updates
    ArduinoOTA.handle();
#ifdef ESP8266
    MDNS.update();
#endif
    m_wifiTimeoutTimer = 0;
  }
  else
  {
    if (m_wifiTimeoutTimer > WIFI_CONNECT_TIMEOUT)
    {
      InitWifi(true);
    }
  }

  // Always perform mqtt loop to detect connection failures
  m_mqttClient.loop();
}