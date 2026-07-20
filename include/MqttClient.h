#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "system.h"

class CMqttClient : public PubSubClient
{
  public:
    static void PrintDataError(void);
    static void GetFriendlyName(const String& strName, String& strFriendly);

    void PublishSwitchConfig(const char* strItem);
    void PublishNumberConfig(const char* strItem, const char* strStep, const char* strMin = "", const char* strMax = "");
    void PublishBinarySensorConfig(const char* strItem, const bool bDiag = false);
    void PublishSensorConfig(const char* strItem, const char* strUnit = "", const char* strCla = "", const bool bDiag = false);

    bool PublishData(const char* strItem, const String& strPayload, const bool bRetained = true);

    void Init(const uint8_t* serverIp);
    bool ServerConnect();

  private:
    void PublishConfig(JsonDocument& root, const char* strItem, const char* strTopicType, const bool bDiag = false);

    uint8_t m_serverIp[4] = { 0 };
    const String m_strName = MQTT_NAME;
};
#endif // MQTT_CLIENT_H