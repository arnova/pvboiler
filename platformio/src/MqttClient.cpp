/* 
  MqttClient Class
  (C) Copyright 2026

  Written by       : Arno van Amersfoort
  Dependencies     : PubSubClient ArduinoJson Terminal util
  Initial date     : July 30, 2026
  Last modified    : September 22, 2026
*/

#include <Arduino.h>

#include "MqttClient.h"
#include "Terminal.h"
#include "util.h"


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


void CMqttClient::ConstructConfigMessage(JsonDocument& root, const char* strItem)
{
  char strFriendlyItem[MQTT_MAX_TOPIC_ITEM_SIZE + 1];
  GetFriendlyName(strItem, strFriendlyItem, sizeof(strFriendlyItem));

  root["name"] = strFriendlyItem;

  char strStateTopic[MQTT_MAX_TOPIC_ITEM_SIZE + HOST_NAME_MAX_SIZE + 2];
  snprintf(strStateTopic, sizeof(strStateTopic), "%s/%s", m_strHostName, strItem);
  root["state_topic"] = strStateTopic;

  char strUniqueId[MQTT_MAX_TOPIC_ITEM_SIZE + HOST_NAME_MAX_SIZE + 2];
  snprintf(strUniqueId, sizeof(strUniqueId), "%s_%s", m_strHostName, strItem);
  root["unique_id"] = strUniqueId;

  char strAvail[HOST_NAME_MAX_SIZE + 8];
  snprintf(strAvail, sizeof(strAvail), "%s/status", m_strHostName);
  root["availability_topic"] = strAvail;
  root["payload_available"] = "online";
  root["payload_not_available"] = "offline";

  root["retain"] = true;
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

#ifdef MQTT_DEBUG
  CTerminal::println(message); //Prints it out on one line
#endif

  char strTopic[MQTT_MAX_CONFIG_TOPIC_SIZE + 1];
  snprintf(strTopic, sizeof(strTopic), "homeassistant/%s/%s/%s/config", strTopicType, m_strHostName, strItem);
  publish(strTopic, strMessage, true);
}


void CMqttClient::PublishSetterConfig(JsonDocument& root, const char* strItem, const char* strTopicType)
{
  ConstructConfigMessage(root, strItem);

  PublishConfig(root, strItem, strTopicType);

  // Subscribe to /set messages
  char strBuf[MQTT_MAX_TOPIC_ITEM_SIZE + HOST_NAME_MAX_SIZE + 6];
  snprintf(strBuf, sizeof(strBuf), "%s/%s/set", m_strHostName, strItem);
  subscribe(strBuf, 1);
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
#ifdef MQTT_DEBUG
  CTerminal::println(message); //Prints it out on one line
#endif

  char strTopic[MQTT_MAX_CONFIG_TOPIC_SIZE + 1];
  snprintf(strTopic, sizeof(strTopic), "homeassistant/%s/%s/%s/config", strTopicType, m_strHostName, strItem);
  publish(strTopic, "", true);

  if (bSetter)
  {
    // Unsubscribe setter
    char strBuf[MQTT_MAX_TOPIC_ITEM_SIZE + HOST_NAME_MAX_SIZE + 6];
    snprintf(strBuf, sizeof(strBuf), "%s/%s/set", m_strHostName, strItem);
    unsubscribe(strBuf);
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


void CMqttClient::PublishSwitchConfig(const char* strItem)
{
  JsonDocument root;

  char strBuf[MQTT_MAX_TOPIC_ITEM_SIZE + HOST_NAME_MAX_SIZE + 6];
  snprintf(strBuf, sizeof(strBuf), "%s/%s/set", m_strHostName, strItem);
  root["command_topic"] = strBuf;

  root["payload_on"] = "1";
  root["payload_off"] = "0";
  root["state_on"] = "1";
  root["state_off"] = "0";
//  root["value_template"] = "{{ value_json.state }}"; // Not used

  PublishSetterConfig(root, strItem, "switch");
}


void CMqttClient::PublishNumberConfig(const char* strItem, const float fStep /* = 1 */, const float fMin /* = 0 */, const float fMax /* = 100 */, const bool bBox /* = true */)
{
  JsonDocument root;

  char strBuf[MQTT_MAX_TOPIC_ITEM_SIZE + HOST_NAME_MAX_SIZE + 6];
  snprintf(strBuf, sizeof(strBuf), "%s/%s/set", m_strHostName, strItem);
  root["command_topic"] = strBuf;

  root["min"] = fMin;
  root["max"] = fMax;
  root["step"] = fStep;

  root["mode"] = bBox ? "box" : "slider";

  PublishSetterConfig(root, strItem, "number");
}


void CMqttClient::PublishSelectConfig(const char* strItem, const char** strValues, const uint8_t iCount)
{
  JsonDocument root;

  char strBuf[MQTT_MAX_TOPIC_ITEM_SIZE + HOST_NAME_MAX_SIZE + 6];
  snprintf(strBuf, sizeof(strBuf), "%s/%s/set", m_strHostName, strItem);
  root["command_topic"] = strBuf;

  for (uint8_t it = 0; it < iCount; it++)
  {
    root["options"][it] = strValues[it];
  }

  PublishSetterConfig(root, strItem, "select");
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


bool CMqttClient::PublishMessage(const char* strItem, const char* strPayload, const bool bRetained /* = true */)
{
  char strTopic[MQTT_MAX_TOPIC_ITEM_SIZE + HOST_NAME_MAX_SIZE + 2];
  snprintf(strTopic, sizeof(strTopic), "%s/%s", m_strHostName, strItem);
  return publish(strTopic, strPayload, bRetained);
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
  snprintf(strBuf, sizeof(strBuf), "%u.%u.%u.%u", m_serverIp[0], m_serverIp[1], m_serverIp[2], m_serverIp[3]);
  CTerminal::print(strBuf);

  CTerminal::print(":" STRINGIZE(MQTT_PORT) "...");
#endif
  // Create a random client ID
  char strclientId[HOST_NAME_MAX_SIZE + 6]; // Enough for hostname-xxxx
  snprintf(strclientId, sizeof(strclientId), "%s-%lx", m_strHostName, random(0xffff));

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

  char strWillTopic[HOST_NAME_MAX_SIZE + 8];
  snprintf(strWillTopic, sizeof(strWillTopic), "%s/status", m_strHostName);

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

#ifdef MQTT_DEBUG
  CTerminal::println("OK");
#endif

  return true;
}
