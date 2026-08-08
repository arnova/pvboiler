#pragma once
#ifndef NETWORK_H
#define NETWORK_H

#include "system.h"
#include "MqttClient.h"

#include <elapsedMillis.h>

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


class CNetwork
{
  public:
    CNetwork(); // Ctor
    ~CNetwork() {}; // Emtpy dtor

    void Init();

    void Loop();

    void LoadSettings();
    void InitWifi(const bool bReconnect);
    void SetWifiSsid(const char* strSsid);
    void SetWifiPassword(const char* strPassword);
    void SetIpAddr(const uint8_t* ipAddress);
    void SetNetMask(const uint8_t* ipNetMask);
    void SetMqttServerIp(const uint8_t* ipAddress);

    void MqttPublishValues();

    const char* GetWifiSsid() { return m_strWifiSsid; };
    const char* GetWifiPassword() { return m_strWifiPassword; };
    const uint8_t* GetIpAddr() { return m_ipAddr; };
    const uint8_t* GetNetMask() { return m_ipNetmask; };
    const uint8_t* GetServerIp() { return m_serverIpAddr; };

    const bool IsConnected() { return m_bWifiConnected; };
    const bool IsMqttConnected() { return m_bWifiConnected && m_mqttClient.connected(); };
    bool HandleMqttClient();

#ifdef SOCKET_SERVER_PORT
    WiFiClient& GetSocketServerClient();
#endif

    CMqttClient& GetMqttClient() { return m_mqttClient; };

  private:
    char m_strWifiSsid[WIFI_SSID_MAX_SIZE + 1] = { 0 };
    char m_strWifiPassword[WIFI_PASSWORD_MAX_SIZE + 1] = { 0 };
    uint8_t m_ipAddr[4] = { 0 };
    uint8_t m_ipNetmask[4] = { 0 };
    uint8_t m_serverIpAddr[4] = { 0 };

    bool m_bWifiConnected = false;
    elapsedMillis m_wifiTimeoutTimer = 0;
    elapsedMillis m_mqttTimeoutTimer = 0;

#ifdef SOCKET_SERVER_PORT
    WiFiServer m_socketServer;
    WiFiClient m_socketServerClient;
#endif

    WiFiClient m_wifiClient;
    CMqttClient m_mqttClient;
};
#endif // NETWORK_H