#ifndef MQTTUTIL_H
#define MQTTUTIL_H

#include <PubSubClient.h>
#include "system.h"

class CMqttUtil
{
  public:
    CMqttUtil(PubSubClient& MQTTClient) { m_pMQTTClient = &MQTTClient; }; // Constructor

    static void PrintDataError(void);
    static void GetFriendlyName(const String& strName, String& strFriendly);
 
    bool Reconnect();
    void PublishSwitchConfig(const char* strItem);
    void PublishNumberConfig(const char* strItem, const char* strMin, const char* strMax, const char* strStep);
    void PublishBinarySensorConfig(const char* strItem);
    void PublishSensorConfig(const char* strItem, const char* strUnit, const char* strCla);

  private:
    void PublishConfig(const char* strItem, const char* strTopicType, JsonDocument& root);

    PubSubClient* m_pMQTTClient;
    const String m_strName = MQTT_NAME;
};
#endif // MQTTUTIL_H