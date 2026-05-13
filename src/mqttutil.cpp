#include <Arduino.h>
#include <ArduinoJson.h>

#include "mqttutil.h"

void CMqttUtil::PrintDataError(void)
{
  Serial.println("ERROR: Invalid MQTT data for topic");
}


void CMqttUtil::GetFriendlyName(const String& strName, String& strFriendly)
{
  bool bSpace = true;

  for (uint8_t it = 0; it < strName.length(); it++)
  {
    if (strName[it] == '_')
    {
      strFriendly += ' ';
      bSpace = true;
    }
    else
    {
      if (bSpace)
      {
        bSpace = false;
        strFriendly += (char) toupper(strName[it]);
      }
      else
      {
        strFriendly += strName[it];
      }
    }
  }
}


void CMqttUtil::PublishConfig(JsonDocument& root, const char* strItem, const char* strTopicType)
{
  String strFriendlyItem;
  GetFriendlyName(strItem, strFriendlyItem);

  root["state_topic"] = m_strName + "/" + strItem;
  root["name"] = strFriendlyItem;
  root["unique_id"] = m_strName + "_" + strItem; // Optional
  root["retain"] = true;
  root["qos"] = 1;

  JsonObject device = root["device"].to<JsonObject>();
  device["name"] = HA_DEVICE_NAME;
  device["model"] = HA_DEVICE_MODEL;
  device["manufacturer"] = HA_MANUFACTURER;
  device["identifiers"] = HA_DEVICE_NAME;

  // Output to console
  serializeJsonPretty(root, Serial);
  Serial.println();

  // Serialize JSON for MQTT
  char message[MQTT_MAX_SIZE];
  serializeJson(root, message);
  Serial.println(message); //Prints it out on one line.

  String strTopic = String("homeassistant/") + strTopicType + "/" + m_strName + "/" + strItem + "/config";

  m_pMQTTClient->subscribe((m_strName + "/" + strItem + "/set").c_str(), 1);
  m_pMQTTClient->publish(strTopic.c_str(), message, true);
}


void CMqttUtil::PublishSwitchConfig(const char* strItem)
{
  JsonDocument root;

  root["command_topic"] = m_strName + "/" + strItem + "/set";
  root["payload_on"] = "1";
  root["payload_off"] = "0";
  root["state_on"] = "1";
  root["state_off"] = "0";
//  root["value_template"] = "{{ value_json.state }}"; // Not used

  PublishConfig(root, strItem, "switch");
}


void CMqttUtil::PublishNumberConfig(const char* strItem, const char* strMin, const char* strMax, const char* strStep)
{
  JsonDocument root;

  root["command_topic"] = m_strName + "/" + strItem + "/set";
  root["min"] = strMin;
  root["max"] = strMax;
  root["step"] = strStep;

  PublishConfig(root, strItem, "number");
}


void CMqttUtil::PublishBinarySensorConfig(const char* strItem)
{
  JsonDocument root;

  root["payload_on"] = "1";
  root["payload_off"] = "0";

  PublishConfig(root, strItem, "binary_sensor");
}


void CMqttUtil::PublishSensorConfig(const char* strItem, const char* strUnit, const char* strCla)
{
  JsonDocument root;

  root["unit_of_measurement"] = strUnit;
  root["device_class"] = strCla;

  PublishConfig(root, strItem, "sensor");
}


bool CMqttUtil::Reconnect()
{
  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "ESPBut-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
//    if (MQTTClient.connect(clientId.c_str(), NULL, NULL, "test", 0, false, "not connected", false))
  if (!m_pMQTTClient->connect(clientId.c_str()))
  {
    Serial.print("failed, rc=");
    Serial.print(m_pMQTTClient->state());
    return false;
  }

  Serial.println("connected");

  return true;
}
