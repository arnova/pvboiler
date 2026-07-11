#ifndef NETWORK_H
#define NETWORK_H

#include "system.h"

class CNetwork
{
  public:
    CNetwork(PubSubClient& MQTTClient) : m_socketServer(SOCKET_SERVER_PORT), m_MQTTClient(MQTTClient) {};

    void LoadSettings();
    void InitWifi(const bool bReconnect);
    void SetWifiSsid(const char* strSsid);
    void SetWifiPassword(const char* strPassword);
    void SetIpAddr(uint8_t* ipAddress);
    void SetNetMask(uint8_t* ipNetMask);
    void SetServerIp(uint8_t* ipAddress);

    const char* GetWifiSsid() { return m_strWifiSsid; };
    const char* GetWifiPassword() { return m_strWifiPassword; };
    const uint8_t* GetIpAddr() { return m_ipAddr; };
    const uint8_t* GetNetMask() { return m_ipNetmask; };
    const uint8_t* GetServerIp() { return m_serverIpAddr; };
    
    WiFiServer& GetSocketServer() { return m_socketServer; };
    WiFiClient& GetSocketServerClient() { return m_socketServerClient; };

  private:
    char m_strWifiSsid[WIFI_SSID_MAX_SIZE + 1] = { 0 };
    char m_strWifiPassword[WIFI_PASSWORD_MAX_SIZE + 1] = { 0 };
    uint8_t m_ipAddr[4] = { 0 };
    uint8_t m_ipNetmask[4] = { 0 };
    uint8_t m_serverIpAddr[4] = { 0 };

    WiFiServer m_socketServer;
    WiFiClient m_socketServerClient;

    PubSubClient& m_MQTTClient;
};
#endif // NETWORK_H