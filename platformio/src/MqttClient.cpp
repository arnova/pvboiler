/* 
  MqttClient Class
  (C) Copyright 2026

  Written by       : Arno van Amersfoort
  Dependencies     : PubSubClient ArduinoJson Terminal util
  Initial date     : July 30, 2026
  Last modified    : October 22, 2026
*/

#include <Arduino.h>

#include "MqttClient.h"
#include "Network.h"
#include "Terminal.h"
#include "util.h"
#include "MqttTopics.h"

// Topic buffer sizes (including null char)
#define MQTT_ITEM_TOPIC_SIZE    (HOST_NAME_MAX_SIZE + MQTT_MAX_TOPIC_ITEM_SIZE + 2) // "<host>/<item>"
#define MQTT_SETTER_TOPIC_SIZE  (MQTT_ITEM_TOPIC_SIZE + 4)                          // "<host>/<item>/set"
#define MQTT_STATUS_TOPIC_SIZE  (HOST_NAME_MAX_SIZE + 8)                            // "<host>/status"
#define MQTT_CONFIG_TOPIC_SIZE  (MQTT_MAX_CONFIG_TOPIC_SIZE + 1)                    // "homeassistant/<type>/<host>/<item>/config"


void CMqttClient::PrintDataError(void)
{
#ifdef MQTT_DEBUG
  CTerminal::println("ERROR: Invalid MQTT data for topic");
#endif
}


void CMqttClient::GetFriendlyName(const char* strName, char* strFriendly, const size_t iMaxSize)
{
  bool bSpace = true;

  size_t iPos;
  for (iPos = 0; iPos < strlen(strName) && iPos < iMaxSize - 1; iPos++)
  {
    if (strName[iPos] == '_')
    {
      strFriendly[iPos] = ' ';
      bSpace = true;
    }
    else
    {
      if (bSpace)
      {
        bSpace = false;
        strFriendly[iPos] = (char) toupper((unsigned char) strName[iPos]);
      }
      else
      {
        strFriendly[iPos] = strName[iPos];
      }
    }
  }
  strFriendly[iPos] = '\0';
}


const char* CMqttClient::GetItemTopic(const char* strItem, char* strBuf, const size_t iBufSize) const
{
  snprintf(strBuf, iBufSize, "%s/%s", m_strHostName, strItem);
  return strBuf;
}


const char* CMqttClient::GetSetterTopic(const char* strItem, char* strBuf, const size_t iBufSize) const
{
  snprintf(strBuf, iBufSize, "%s/%s/set", m_strHostName, strItem);
  return strBuf;
}


const char* CMqttClient::GetStatusTopic(char* strBuf, const size_t iBufSize) const
{
  snprintf(strBuf, iBufSize, "%s/status", m_strHostName);
  return strBuf;
}


const char* CMqttClient::GetConfigTopic(const char* strItem, const char* strTopicType, char* strBuf, const size_t iBufSize) const
{
  snprintf(strBuf, iBufSize, "homeassistant/%s/%s/%s/config", strTopicType, m_strHostName, strItem);
  return strBuf;
}


void CMqttClient::ConstructConfigMessage(JsonDocument& root, const char* strItem)
{
  char strFriendlyItem[MQTT_MAX_TOPIC_ITEM_SIZE + 1];
  GetFriendlyName(strItem, strFriendlyItem, sizeof(strFriendlyItem));

  root["name"] = strFriendlyItem;

  char strStateTopic[MQTT_ITEM_TOPIC_SIZE];
  root["state_topic"] = GetItemTopic(strItem, strStateTopic, sizeof(strStateTopic));

  char strUniqueId[MQTT_ITEM_TOPIC_SIZE];
  snprintf(strUniqueId, sizeof(strUniqueId), "%s_%s", m_strHostName, strItem);
  root["unique_id"] = strUniqueId;

  char strAvail[MQTT_STATUS_TOPIC_SIZE];
  root["availability_topic"] = GetStatusTopic(strAvail, sizeof(strAvail));
  root["payload_available"] = "online";
  root["payload_not_available"] = "offline";

  root["qos"] = 1;

  JsonObject device = root["device"].to<JsonObject>();
  device["identifiers"][0] = m_strHostName;
  device["name"] = m_strHostName;
  device["model"] = DEVICE_MODEL;
  device["manufacturer"] = MANUFACTURER;
  device["sw_version"] = MY_VERSION;
}


void CMqttClient::PublishConfig(JsonDocument& root, const char* strItem, const char* strTopicType)
{
  // Serialize JSON for MQTT
  char strMessage[MQTT_MAX_MESSAGE_SIZE];
  serializeJson(root, strMessage);

  char strTopic[MQTT_CONFIG_TOPIC_SIZE];
  publish(GetConfigTopic(strItem, strTopicType, strTopic, sizeof(strTopic)), strMessage, true);

#ifdef MQTT_DEBUG
  CTerminal::print(strTopic);
  CTerminal::print(": ");
  CTerminal::println(strMessage);
#endif
}


void CMqttClient::PublishSetterConfig(JsonDocument& root, const char* strItem, const char* strTopicType, const bool bRetain /* = true */)
{
  ConstructConfigMessage(root, strItem);

  char strSetterTopic[MQTT_SETTER_TOPIC_SIZE];
  root["command_topic"] = GetSetterTopic(strItem, strSetterTopic, sizeof(strSetterTopic));

  // Whether HA publishes commands (/set) retained
  root["retain"] = bRetain;

  PublishConfig(root, strItem, strTopicType);

  // Subscribe to /set messages
  subscribe(strSetterTopic, 1);
}


void CMqttClient::PublishGetterConfig(JsonDocument& root, const char* strItem, const char* strTopicType, const bool bDiag /* = false */)
{
  ConstructConfigMessage(root, strItem);

  if (bDiag)
    root["entity_category"] = "diagnostic";

  PublishConfig(root, strItem, strTopicType);
}


void CMqttClient::UnpublishConfig(const char* strItem, const char* strTopicType, const bool bSetter /* = false */)
{
  char strTopic[MQTT_CONFIG_TOPIC_SIZE];
  publish(GetConfigTopic(strItem, strTopicType, strTopic, sizeof(strTopic)), "", true);

#ifdef MQTT_DEBUG
  CTerminal::print(strTopic);
  CTerminal::println(": \"\"");
#endif

  if (bSetter)
  {
    // Unsubscribe setter
    char strBuf[MQTT_SETTER_TOPIC_SIZE];
    unsubscribe(GetSetterTopic(strItem, strBuf, sizeof(strBuf)));

#ifdef MQTT_DEBUG
    CTerminal::print("Unsubscribe: ");
    CTerminal::println(strBuf);
#endif
  }
}


void CMqttClient::UnpublishSwitchConfig(const char* strItem)
{
  UnpublishConfig(strItem, "switch", true);
}


void CMqttClient::UnpublishNumberConfig(const char* strItem)
{
  UnpublishConfig(strItem, "number", true);
}


void CMqttClient::UnpublishSensorConfig(const char* strItem)
{
  UnpublishConfig(strItem, "sensor");
}


void CMqttClient::UnpublishBinarySensorConfig(const char* strItem)
{
  UnpublishConfig(strItem, "binary_sensor");
}


void CMqttClient::PublishSwitchConfig(const char* strItem, const bool bRetain /* = true */)
{
  JsonDocument root;

  root["payload_on"] = "1";
  root["payload_off"] = "0";
  root["state_on"] = "1";
  root["state_off"] = "0";
//  root["value_template"] = "{{ value_json.state }}"; // Not used

  PublishSetterConfig(root, strItem, "switch", bRetain);
}


void CMqttClient::PublishNumberConfig(const char* strItem, const float fStep /* = 1.0f */, const float fMin /* = 0.0f */, const float fMax /* = 100.0f */, const char* strUnit /* = "" */, const char* strDeviceClass /* = "" */, const bool bBox /* = true */, const bool bRetain /* = true */)
{
  JsonDocument root;

  root["min"] = fMin;
  root["max"] = fMax;
  root["step"] = fStep;

  if (strlen(strUnit) != 0)
    root["unit_of_measurement"] = strUnit;

  if (strlen(strDeviceClass) != 0)
    root["device_class"] = strDeviceClass;

  root["mode"] = bBox ? "box" : "slider";

  PublishSetterConfig(root, strItem, "number", bRetain);
}


void CMqttClient::PublishSelectConfig(const char* strItem, const char* const* strValues, const uint8_t iCount, const bool bRetain /* = true */)
{
  JsonDocument root;

  for (uint8_t it = 0; it < iCount; it++)
  {
    root["options"][it] = strValues[it];
  }

  PublishSetterConfig(root, strItem, "select", bRetain);
}


void CMqttClient::PublishBinarySensorConfig(const char* strItem, const bool bDiag /* = false */)
{
  JsonDocument root;

  root["payload_on"] = "1";
  root["payload_off"] = "0";

  PublishGetterConfig(root, strItem, "binary_sensor", bDiag);
}


void CMqttClient::PublishSensorConfig(const char* strItem, const char* strUnit /* = "" */, const char* strDeviceClass /* = "" */, const char* strStateClass /* = "" */, const bool bDiag /* = false */)
{
  JsonDocument root;

  if (strlen(strUnit) != 0)
    root["unit_of_measurement"] = strUnit;

  if (strlen(strDeviceClass) != 0)
    root["device_class"] = strDeviceClass;

  if (strlen(strStateClass) != 0)
    root["state_class"] = strStateClass;

  PublishGetterConfig(root, strItem, "sensor", bDiag);
}


void CMqttClient::PublishEnumSensorConfig(const char* strItem, const char* const* strValues, const uint8_t iCount, const bool bDiag /* = false */)
{
  JsonDocument root;

  root["device_class"] = "enum";

  for (uint8_t it = 0; it < iCount; it++)
  {
    root["options"][it] = strValues[it];
  }

  PublishGetterConfig(root, strItem, "sensor", bDiag);
}


bool CMqttClient::PublishMessage(const char* strItem, const char* strPayload, const bool bRetain /* = false */)
{
  char strTopic[MQTT_ITEM_TOPIC_SIZE];
  return publish(GetItemTopic(strItem, strTopic, sizeof(strTopic)), strPayload, bRetain);
}


bool CMqttClient::PublishUInt(const char* strItem, const uint32_t iValue, const bool bRetain /* = false */)
{
  char strBuf[12]; // Enough room for an unsigned 32 bit number
  snprintf(strBuf, sizeof(strBuf), "%u", iValue);
  return PublishMessage(strItem, strBuf, bRetain);
}


bool CMqttClient::PublishInt(const char* strItem, const int32_t iValue, const bool bRetain /* = false */)
{
  char strBuf[12]; // Enough room for a signed 32 bit number
  snprintf(strBuf, sizeof(strBuf), "%d", iValue);
  return PublishMessage(strItem, strBuf, bRetain);
}


bool CMqttClient::PublishFloat(const char* strItem, const float fValue, const uint8_t iDecimals)
{
  char strBuf[24]; // Enough room for our floats with 4 digit precision
  snprintf(strBuf, sizeof(strBuf), "%.*f", iDecimals, fValue);
  return PublishMessage(strItem, strBuf);
}


bool CMqttClient::PublishBool(const char* strItem, const bool bValue)
{
  return PublishMessage(strItem, bValue ? "1" : "0");
}


void CMqttClient::Init(const uint8_t* serverIp, const char* strHostName, const char* strUser, const char* strPassword)
{
  memcpy(m_serverIp, serverIp, 4);

  setBufferSize(MQTT_MAX_MESSAGE_SIZE);
  setServer(m_serverIp, MQTT_PORT);

  strcpy(m_strHostName, strHostName);

  if (strUser != NULL)
  {
    strcpy(m_strUser, strUser);
  }

  if (strPassword != NULL)
  {
    strcpy(m_strPassword, strPassword);
  }
}


bool CMqttClient::ServerConnect()
{
#ifdef MQTT_DEBUG
  CTerminal::print("Connecting to MQTT server: ");

  char strBuf[16]; // Enough for an IPv4 address + null char
  CTerminal::print(CNetwork::FormatIp(IPAddress(m_serverIp), strBuf, sizeof(strBuf)));

  CTerminal::print(":" STRINGIZE(MQTT_PORT) "...");
#endif
  // Use a fixed client ID so the broker takes over (and ends) a stale session from before a reboot,
  // instead of firing its "offline" will later on and overriding our "online" status
  const char* strclientId = m_strHostName;

  char* strUser = NULL;
  if (strlen(m_strUser) != 0)
  {
    strUser = m_strUser;
  }

  char* strPassword = NULL;
  if (strlen(m_strPassword) != 0)
  {
    strPassword = m_strPassword;
  }

  char strWillTopic[MQTT_STATUS_TOPIC_SIZE];
  GetStatusTopic(strWillTopic, sizeof(strWillTopic));

  // Attempt to connect
  if (!connect(strclientId, strUser, strPassword, strWillTopic, 1, true, "offline"))
  {
#ifdef MQTT_DEBUG
    CTerminal::print("ERROR, rc=");
    CTerminal::println(state());
#endif
    return false;
  }

  publish(strWillTopic, "online", true);

  // Get notified when Home Assistant (re)starts so we can republish our (non-retained) states
  subscribe(MQTT_HA_STATUS_TOPIC, 1);

#ifdef MQTT_DEBUG
  CTerminal::println("OK");
#endif

  return true;
}


void CMqttClient::ServerDisconnect()
{
  if (connected())
  {
    // Clean disconnect: will isn't sent by the broker, so explicitly report offline
    char strWillTopic[MQTT_STATUS_TOPIC_SIZE];
    publish(GetStatusTopic(strWillTopic, sizeof(strWillTopic)), "offline", true);

    disconnect();
  }
}
