#include <Arduino.h>
#include <ArduinoJson.h>

#include "MqttClient.h"
#include "TermPrint.h"
#include "util.h"


void CMqttClient::PrintDataError(void)
{
#ifdef MQTT_DEBUG
  CTermPrint::println("ERROR: Invalid MQTT data for topic");
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

  char strBuf[MQTT_MAX_TOPIC_ITEM_SIZE + sizeof(HOST_NAME) + 2];
  snprintf(strBuf, sizeof(strBuf), HOST_NAME "/%s", strItem);
  root["state_topic"] = strBuf;

  root["name"] = strFriendlyItem;

  char strBuf2[MQTT_MAX_TOPIC_ITEM_SIZE + sizeof(HOST_NAME) + 2];
  snprintf(strBuf2, sizeof(strBuf2), HOST_NAME "_%s", strItem);
  root["unique_id"] = strBuf2;
  
  root["retain"] = true;
  root["qos"] = 1;

  JsonObject device = root["device"].to<JsonObject>();
  device["name"] = HA_DEVICE_NAME;
  device["model"] = HA_DEVICE_MODEL;
  device["manufacturer"] = HA_MANUFACTURER;
  device["identifiers"][0] = HA_DEVICE_NAME;
  device["sw_version"] = MY_VERSION;
}


void CMqttClient::PublishConfig(JsonDocument& root, const char* strItem, const char* strTopicType)
{
  // Serialize JSON for MQTT
  char strMessage[MQTT_MAX_MESSAGE_SIZE];
  serializeJson(root, strMessage);

#ifdef MQTT_DEBUG
  CTermPrint::println(message); //Prints it out on one line
#endif

  char strTopic[MQTT_MAX_CONFIG_TOPIC_SIZE + 1];
  snprintf(strTopic, sizeof(strTopic), "homeassistant/%s/%s/%s/config", strTopicType, MQTT_NAME, strItem);
  publish(strTopic, strMessage, true);
}


void CMqttClient::PublishSetterConfig(JsonDocument& root, const char* strItem, const char* strTopicType)
{
  ConstructConfigMessage(root, strItem);

  PublishConfig(root, strItem, strTopicType);

  // Subscribe to /set messages
  char strBuf[MQTT_MAX_TOPIC_ITEM_SIZE + sizeof(HOST_NAME) + 6];
  snprintf(strBuf, sizeof(strBuf), HOST_NAME "/%s/set", strItem);

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
  JsonDocument root;

  PublishConfig(root, strItem, strTopicType);

  if (bSetter)
  {
    // Unsubscribe setter
    char strBuf[MQTT_MAX_TOPIC_ITEM_SIZE + sizeof(HOST_NAME) + 6];
    snprintf(strBuf, sizeof(strBuf), HOST_NAME "/%s/set", strItem);

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

  char strBuf[MQTT_MAX_TOPIC_ITEM_SIZE + sizeof(HOST_NAME) + 6];
  snprintf(strBuf, sizeof(strBuf), HOST_NAME "/%s/set", strItem);

  root["command_topic"] = strBuf;
  root["payload_on"] = "1";
  root["payload_off"] = "0";
  root["state_on"] = "1";
  root["state_off"] = "0";
//  root["value_template"] = "{{ value_json.state }}"; // Not used

  PublishSetterConfig(root, strItem, "switch");
}


void CMqttClient::PublishNumberConfig(const char* strItem, const char* strStep /* = "" */, const char* strMin /* = "" */, const char* strMax /* = "" */, const bool bBox /* = true */)
{
  JsonDocument root;

  char strBuf[MQTT_MAX_TOPIC_ITEM_SIZE + sizeof(HOST_NAME) + 6];
  snprintf(strBuf, sizeof(strBuf), HOST_NAME "/%s/set", strItem);
  root["command_topic"] = strBuf;

  if (strlen(strMin) != 0)
  {
    root["min"] = strMin;
  }

  if (strlen(strMax) != 0)
  {
    root["max"] = strMax;
  }

  if (strlen(strStep) != 0)
  {
    root["step"] = strStep;
  }

  if (bBox)
  {
    root["mode"] = "box";
  }

  PublishSetterConfig(root, strItem, "number");
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
  char strTopic[MQTT_MAX_TOPIC_ITEM_SIZE + sizeof(MQTT_NAME) + 2];
  snprintf(strTopic, sizeof(strTopic), MQTT_NAME "/%s", strItem);

  return publish(strTopic, strPayload, bRetained);
}


void CMqttClient::Init(const uint8_t* serverIp)
{
  memcpy(m_serverIp, serverIp, 4);

  setBufferSize(MQTT_MAX_MESSAGE_SIZE);
  setServer(m_serverIp, MQTT_PORT);
}


bool CMqttClient::ServerConnect()
{
  char strBuf[16]; // Enough for IPv4 address / hostname-xxxx

#ifdef MQTT_DEBUG
  CTermPrint::print("Connecting to MQTT server: ");

  snprintf(strBuf, sizeof(strBuf), "%u.%u.%u.%u", m_serverIp[0], m_serverIp[1], m_serverIp[2], m_serverIp[3]);
  CTermPrint::print(strBuf);

  CTermPrint::print(":" STRINGIZE(MQTT_PORT) "...");
#endif
  // Create a random client ID
  snprintf(strBuf, sizeof(strBuf), HOST_NAME "-%lx", random(0xffff));
  // Attempt to connect
//  if (connect(strBuf, NULL, NULL, "test", 0, false, "not connected", false))
  if (!connect(strBuf))
  {
#ifdef MQTT_DEBUG
    CTermPrint::print("ERROR, rc=");
    CTermPrint::println(state());
#endif
    return false;
  }

#ifdef MQTT_DEBUG
  CTermPrint::println("OK");
#endif

  return true;
}
