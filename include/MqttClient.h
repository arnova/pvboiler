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

    void Init(const uint8_t* serverIp);
    bool ServerConnect();

    void PublishSwitchConfig(const char* strItem);
    void PublishNumberConfig(const char* strItem, const char* strMin, const char* strMax, const char* strStep);
    void PublishBinarySensorConfig(const char* strItem);
    void PublishSensorConfig(const char* strItem, const char* strUnit, const char* strCla);

  private:
    void PublishConfig(JsonDocument& root, const char* strItem, const char* strTopicType);

    uint8_t m_serverIp[4] = { 0 };
    const String m_strName = MQTT_NAME;
};
#endif // MQTT_CLIENT_H