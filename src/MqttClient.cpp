#include <Arduino.h>
#include <ArduinoJson.h>

#include "MqttClient.h"
#include "TermPrint.h"


void CMqttClient::PrintDataError(void)
{
#ifdef MQTT_DEBUG
  CTermPrint::println("ERROR: Invalid MQTT data for topic");
#endif
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


void CMqttClient::PublishConfig(JsonDocument& root, const char* strItem, const char* strTopicType, const bool bDiag /* = false */)
{
  String strFriendlyItem;
  GetFriendlyName(strItem, strFriendlyItem);

  root["state_topic"] = m_strName + "/" + strItem;
  root["name"] = strFriendlyItem;
  root["unique_id"] = m_strName + "_" + strItem; // Optional
  root["retain"] = true;
  root["qos"] = 1;

  if (bDiag)
    root["entity_category"] = "diagnostic";

  JsonObject device = root["device"].to<JsonObject>();
  device["name"] = HA_DEVICE_NAME;
  device["model"] = HA_DEVICE_MODEL;
  device["manufacturer"] = HA_MANUFACTURER;
  device["identifiers"] = HA_DEVICE_NAME;

  // Output to console
#ifdef MQTT_DEBUG
  serializeJsonPretty(root, TERM_SERIAL);
  CTermPrint::println("");
#endif

  // Serialize JSON for MQTT
  char message[MQTT_MAX_SIZE];
  serializeJson(root, message);

#ifdef MQTT_DEBUG
  CTermPrint::println(message); //Prints it out on one line
#endif

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


void CMqttClient::PublishNumberConfig(const char* strItem, const char* strStep /* = "" */, const char* strMin /* = "" */, const char* strMax /* = "" */)
{
  JsonDocument root;

  root["command_topic"] = m_strName + "/" + strItem + "/set";
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

  PublishConfig(root, strItem, "number");
}


void CMqttClient::PublishBinarySensorConfig(const char* strItem, const bool bDiag /* = false */)
{
  JsonDocument root;

  root["payload_on"] = "1";
  root["payload_off"] = "0";

  PublishConfig(root, strItem, "binary_sensor", bDiag);
}


void CMqttClient::PublishSensorConfig(const char* strItem, const char* strUnit, const char* strCla, const bool bDiag /* = false */)
{
  JsonDocument root;

  root["unit_of_measurement"] = strUnit;
  root["device_class"] = strCla;

  PublishConfig(root, strItem, "sensor", bDiag);
}


bool CMqttClient::PublishData(const char* strItem, const String& strPayload, const bool bRetained /* = true */)
{
  return publish((String(MQTT_NAME "/") + strItem).c_str(), strPayload.c_str(), bRetained);
}


void CMqttClient::Init(const uint8_t* serverIp)
{
  memcpy(m_serverIp, serverIp, 4);

  setBufferSize(MQTT_MAX_SIZE);
  setServer(m_serverIp, MQTT_PORT);
}


bool CMqttClient::ServerConnect()
{
  CTermPrint::print(String("Connecting to MQTT server: ") + IPAddress(m_serverIp).toString() + ":" + String(MQTT_PORT) + "...");
  // Create a random client ID
  String clientId = HOST_NAME "-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
//    if (connect(clientId.c_str(), NULL, NULL, "test", 0, false, "not connected", false))
  if (!connect(clientId.c_str()))
  {
    CTermPrint::print("ERROR, rc=");
    CTermPrint::println(state());
    return false;
  }

  CTermPrint::println("OK");

  return true;
}
