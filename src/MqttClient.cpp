#include <Arduino.h>
#include <ArduinoJson.h>

#include "MqttClient.h"
#include "TermPrint.h"


void CMqttClient::PrintDataError(void)
{
  CTermPrint::println("ERROR: Invalid MQTT data for topic");
}


void CMqttClient::GetFriendlyName(const String& strName, String& strFriendly)
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


void CMqttClient::PublishConfig(JsonDocument& root, const char* strItem, const char* strTopicType)
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
  CTermPrint::println("");

  // Serialize JSON for MQTT
  char message[MQTT_MAX_SIZE];
  serializeJson(root, message);
  CTermPrint::println(message); //Prints it out on one line

  String strTopic = String("homeassistant/") + strTopicType + "/" + m_strName + "/" + strItem + "/config";

  subscribe((m_strName + "/" + strItem + "/set").c_str(), 1);
  publish(strTopic.c_str(), message, true);
}


void CMqttClient::PublishSwitchConfig(const char* strItem)
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


void CMqttClient::PublishNumberConfig(const char* strItem, const char* strMin, const char* strMax, const char* strStep)
{
  JsonDocument root;

  root["command_topic"] = m_strName + "/" + strItem + "/set";
  root["min"] = strMin;
  root["max"] = strMax;
  root["step"] = strStep;

  PublishConfig(root, strItem, "number");
}


void CMqttClient::PublishBinarySensorConfig(const char* strItem)
{
  JsonDocument root;

  root["payload_on"] = "1";
  root["payload_off"] = "0";

  PublishConfig(root, strItem, "binary_sensor");
}


void CMqttClient::PublishSensorConfig(const char* strItem, const char* strUnit, const char* strCla)
{
  JsonDocument root;

  root["unit_of_measurement"] = strUnit;
  root["device_class"] = strCla;

  PublishConfig(root, strItem, "sensor");
}


bool CMqttClient::Reconnect()
{
  CTermPrint::print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "ESPBut-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
//    if (connect(clientId.c_str(), NULL, NULL, "test", 0, false, "not connected", false))
  if (connect(clientId.c_str()))
  {
    CTermPrint::print("failed, rc=");
    CTermPrint::print(state());
    return false;
  }

  CTermPrint::println("connected");

  return true;
}
