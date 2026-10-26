#pragma once
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "system.h"
#include "EepromLayout.h"

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_MAX_MESSAGE_SIZE
#define MQTT_MAX_MESSAGE_SIZE 1024
#endif

#ifndef MQTT_MAX_TOPIC_ITEM_SIZE
#define MQTT_MAX_TOPIC_ITEM_SIZE 64
#endif

#ifndef MQTT_MAX_CONFIG_TOPIC_SIZE
#define MQTT_MAX_CONFIG_TOPIC_SIZE 128
#endif

class CMqttClient : public PubSubClient
{
  public:
    static void PrintDataError(void);
    static void GetFriendlyName(const char* strName, char* strFriendly, const size_t iMaxSize);

    void PublishSwitchConfig(const char* strItem, const bool bRetain = true);
    void PublishNumberConfig(const char* strItem, const float fStep = 1.0f, const float fMin = 0.0f, const float fMax = 100.0f, const char* strUnit = "", const char* strDeviceClass = "", const bool bBox = true, const bool bRetain = true);
    void PublishSelectConfig(const char* strItem, const char* const* strValues, const uint8_t iCount, const bool bRetain = true);
    void PublishBinarySensorConfig(const char* strItem, const bool bDiag = false);
    void PublishSensorConfig(const char* strItem, const char* strUnit = "", const char* strDeviceClass = "", const char* strStateClass = "", const bool bDiag = false);
    void PublishEnumSensorConfig(const char* strItem, const char* const* strValues, const uint8_t iCount, const bool bDiag = false);

    void UnpublishSwitchConfig(const char* strItem);
    void UnpublishNumberConfig(const char* strItem);
    void UnpublishBinarySensorConfig(const char* strItem);
    void UnpublishSensorConfig(const char* strItem);

    bool PublishMessage(const char* strItem, const char* strPayload, const bool bRetain = false);
    bool PublishUInt(const char* strItem, const uint32_t iValue, const bool bRetain = false);
    bool PublishInt(const char* strItem, const int32_t iValue, const bool bRetain = false);
    bool PublishFloat(const char* strItem, const float fValue, const uint8_t iDecimals);
    bool PublishBool(const char* strItem, const bool bValue);

    void Init(const uint8_t* serverIp, const char* strHostName, const char* strUser, const char* strPassword);
    bool ServerConnect();
    void ServerDisconnect();

    const char* GetHostName() const { return m_strHostName; };

  private:
    const char* GetItemTopic(const char* strItem, char* strBuf, const size_t iBufSize) const;
    const char* GetSetterTopic(const char* strItem, char* strBuf, const size_t iBufSize) const;
    const char* GetStatusTopic(char* strBuf, const size_t iBufSize) const;
    const char* GetConfigTopic(const char* strItem, const char* strTopicType, char* strBuf, const size_t iBufSize) const;

    void ConstructConfigMessage(JsonDocument& root, const char* strItem);
    void PublishConfig(JsonDocument& root, const char* strItem, const char* strTopicType);
    void PublishSetterConfig(JsonDocument& root, const char* strItem, const char* strTopicType, const bool bRetain = true);
    void PublishGetterConfig(JsonDocument& root, const char* strItem, const char* strTopicType, const bool bDiag = false);
    void UnpublishConfig(const char* strItem, const char* strTopicType, const bool bSetter = false);

    uint8_t m_serverIp[4] = { 0 };
    char m_strHostName[HOST_NAME_MAX_SIZE + 1] = { 0 };
    char m_strUser[MQTT_USER_MAX_SIZE + 1] = { 0 };
    char m_strPassword[MQTT_PASSWORD_MAX_SIZE + 1] = { 0 };
};
#endif // MQTT_CLIENT_H