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
    void PublishNumberConfig(const char* strItem, const char* strStep = "", const char* strMin = "", const char* strMax = "", const bool bBox = true);
    void PublishBinarySensorConfig(const char* strItem, const bool bDiag = false);
    void PublishSensorConfig(const char* strItem, const char* strUnit = "", const char* strDeviceClass = "", const char* strStateClass = "", const bool bDiag = false);

    void UnpublishSwitchConfig(const char* strItem);
    void UnpublishNumberConfig(const char* strItem);
    void UnpublishBinarySensorConfig(const char* strItem);
    void UnpublishSensorConfig(const char* strItem);

    bool PublishMessage(const char* strItem, const String& strPayload, const bool bRetained = true);

    void Init(const uint8_t* serverIp);
    bool ServerConnect();

  private:
    void ConstructConfigMessage(JsonDocument& root, const char* strItem, const char* strTopicType);
    void PublishConfig(JsonDocument& root, const char* strItem, const char* strTopicType);
    void PublishSetterConfig(JsonDocument& root, const char* strItem, const char* strTopicType);
    void PublishGetterConfig(JsonDocument& root, const char* strItem, const char* strTopicType, const bool bDiag = false);
    void UnpublishConfig(const char* strItem, const char* strTopicType, const bool& bSetter = false);

    uint8_t m_serverIp[4] = { 0 };
    const String m_strName = MQTT_NAME;
};
#endif // MQTT_CLIENT_H