/*
  ESP PV-Boiler - ESP Controlled PV Boiler
  Last update: April 24, 2026
  (C) Copyright 2026 by Arno van Amersfoort
  Web                   : https://github.com/arnova/ctrl4dkn
  Email                 : a r n o DOT v a n DOT a m e r s f o o r t AT g m a i l DOT c o m
                          (note: you must remove all spaces and substitute the @ and the . at the proper locations!)
  ----------------------------------------------------------------------------------------------------------------------
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  ---------------------------------------------------------------------------------------------------------------------- 
*/

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>
#include <ESPmDNS.h>
#endif
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#include "pvboiler.h"
#include "mqttutil.h"
#include "util.h"
#include "system.h"

// Version string:
#define MY_VERSION "0.11"

// Globals
WiFiClient g_wifiClient;
PubSubClient g_MQTTClient(g_wifiClient);
CMqttUtil g_MQTTUtil(g_MQTTClient);
CPVBoiler g_pvBoiler(g_MQTTClient);

volatile uint32_t g_iLastZeroCrossTime = 0;
volatile uint32_t g_iPhaseCorrectionTime = 300; // Default = 300 uS
volatile uint32_t g_iZeroCrossTime = 0;
volatile bool g_bZeroCrossTimeUpdated = false;
volatile bool g_bTriacOn = false;
volatile float g_fTriacAngleFactor = 1.0f; // Off
volatile uint8_t g_iOutputPercentage = 0;
volatile uint8_t g_iSSRPeriodCounter = 0;


// Interrupt generated when crossing zero in either direction
void IRAM_ATTR ZeroCrossISR()
{
  const uint32_t iNow = micros();

  if (digitalRead(ZERO_CROSS_INPUT)) // Rising edge
  {
    // filter noise
    if (g_iLastZeroCrossTime == 0 || iNow - g_iLastZeroCrossTime > ZERO_CROSS_EDGE_MARGIN_US * 10)
    {
      if (g_iLastZeroCrossTime != 0)
      {
        g_iZeroCrossTime = iNow - g_iLastZeroCrossTime;
        g_bZeroCrossTimeUpdated = true;
      }
      g_iLastZeroCrossTime = iNow;
    }

#ifdef SSR_STYLE_MODE
    if (g_iOutputPercentage == 0)
    {
      digitalWrite(TRIAC_OUTPUT, LOW); // Always off
    }
    else
    {
      g_iSSRPeriodCounter++;
      if ((g_iSSRPeriodCounter * 100) / SSR_PERIOD_COUNT <= g_iOutputPercentage)
      {
        // Timer1 at DIV1 (80 MHz clock) → 80 ticks per µs
        // Maximum ~104 ms at this prescaler; no need for DIV256 in our range.
        const uint32_t iTriacDelayTicks = g_iPhaseCorrectionTime * 80; // fixme: underflow

        g_bTriacOn = true;
        timer1_write(iTriacDelayTicks);
      }
      else
      {
        digitalWrite(TRIAC_OUTPUT, LOW); // Off
      }

      if (g_iSSRPeriodCounter >= SSR_PERIOD_COUNT)
      {
        g_iSSRPeriodCounter = 0;
      }
    }
#else
    digitalWrite(TRIAC_OUTPUT, LOW); // Off

    // NOTE: Don't turn on triac near 0% to prevent excessive EMI due to misfiring
    if (g_fTriacAngleFactor <= 0.98f)
    {
      // Timer1 at DIV1 (80 MHz clock) → 80 ticks per µs
      // Maximum ~104 ms at this prescaler; no need for DIV256 in our range.
      const uint32_t iTriacDelayTicks = ((g_fTriacAngleFactor * g_iZeroCrossTime) + g_iPhaseCorrectionTime) * 80;

      g_bTriacOn = true;
      timer1_write(iTriacDelayTicks);
    }
#endif
  }
  else // Falling edge
  {
    // filter noise
    if (g_iPhaseCorrectionTime == 0 || iNow - g_iLastZeroCrossTime > ZERO_CROSS_EDGE_MARGIN_US)
    {
      if (g_iLastZeroCrossTime != 0)
      {
        // NOTE: The time between rising edge and falling edge is used (/2) for phase correction
        g_iPhaseCorrectionTime = (iNow - g_iLastZeroCrossTime) / 2;
      }
    }
  }
}


// Timer interrupt for triggering triac gate
void IRAM_ATTR TriacTimerISR()
{
  if (g_bTriacOn)
  {
    digitalWrite(TRIAC_OUTPUT, HIGH); // On

    // Setup timer to turn off trigger pulse after 100uS:
    // Timer1 at DIV1 (80 MHz clock) → 80 ticks per µs
    // Maximum ~104 ms at this prescaler; no need for DIV256 in our range.
    const uint32_t iTriacDelayTicks = GATE_PULSE_WIDTH * 80;

    g_bTriacOn = false;
    timer1_write(iTriacDelayTicks);
  }
  else
  {
    digitalWrite(TRIAC_OUTPUT, LOW); // Off
  }
}


void MQTTCallback(char* topic, byte *payload, const unsigned int length)
{
  Serial.println("-------new message from broker-----");
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("data: ");
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char) payload[i]);
  }
  Serial.println();

  //float fVal;
  //const bool bValidFloat = BytesToFloat(payload, length, fVal);

  int32_t iVal;
  const bool bValidInt = BytesToInt32(payload, length, iVal);

  if (STRIEQUALS(topic, MQTT_NAME "/" MQTT_CONTROLLER_ON_OFF "/set"))
  {
    if (bValidInt || length == 0)
    {
      if (iVal == 0 || iVal == 1 || length == 0)
      {
        g_pvBoiler.SetCtrlOnOff((iVal == 1 || length == 0) ? true : false);
      }
      else
      {
        CMqttUtil::PrintDataError();
      }
    }
    else
    {
      CMqttUtil::PrintDataError();
    }
  }
#ifdef MQTT_SET_POWER_BUDGET
  else if (STRIEQUALS(topic, MQTT_NAME "/" MQTT_SET_POWER_BUDGET "/set"))
  {
    if (bValidInt && iVal >=0)
    {
      g_pvBoiler.SetCtrlSetPowerBudget(iVal);
    }
    else
    {
      CMqttUtil::PrintDataError();
    }
  }
#endif
#ifdef MQTT_SET_POWER_PERCENTAGE
  else if (STRIEQUALS(topic, MQTT_NAME "/" MQTT_SET_POWER_PERCENTAGE "/set"))
  {
    if (bValidInt && iVal >=0 && iVal <= 100)
    {
      g_pvBoiler.SetCtrlSetPowerPercentage(iVal);
    }
    else
    {
      CMqttUtil::PrintDataError();
    }
  }
#endif

  // Got a message so network is still ok:
  g_pvBoiler.TriggerWatchdog();
}


bool MQTTReconnect()
{
  if (!g_MQTTUtil.Reconnect())
    return false;

  // Publish MQTT config for eg. HA discovery and subscribe to control topics
  g_MQTTUtil.PublishSwitchConfig(MQTT_CONTROLLER_ON_OFF);

#ifdef MQTT_SET_POWER_BUDGET
  g_MQTTUtil.PublishNumberConfig(MQTT_SET_POWER_BUDGET, "-10000.0", "10000.0", "0.1");
#endif

#ifdef MQTT_SET_POWER_PERCENTAGE
  g_MQTTUtil.PublishNumberConfig(MQTT_SET_POWER_PERCENTAGE, "0", "100", "1");
#endif

  g_MQTTUtil.PublishSensorConfig(MQTT_OUTPUT_POWER, "W", "power");

  g_MQTTUtil.PublishSensorConfig(MQTT_OUTPUT_PERCENTAGE, "%", "power_factor");

  // Publish our f/w version
  g_MQTTClient.publish(MQTT_NAME "/" MQTT_FW_VERSION, MY_VERSION, true);

  return true;
}


void SetupWifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize mDNS
  if (!MDNS.begin(HOSTNAME))
  {
    Serial.println("Error setting up MDNS responder!");
  }
  else
  {
    Serial.println("mDNS responder started");
  }

  // Need to explicitly set hostname as ArduinoOTA will override our mdns-name set above
  ArduinoOTA.setHostname(HOSTNAME);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  randomSeed(micros());
}


void setup()
{
  // Outputs
#ifdef STATUS_LED
  pinMode(STATUS_LED, OUTPUT);
#endif

  pinMode(TRIAC_OUTPUT, OUTPUT);
  digitalWrite(TRIAC_OUTPUT, LOW);

  pinMode(ZERO_CROSS_INPUT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ZERO_CROSS_INPUT), ZeroCrossISR, CHANGE);

  timer1_isr_init();
  timer1_attachInterrupt(TriacTimerISR);
  timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);   // TIM_SINGLE = single shot

  Serial.begin(9600);
  Serial.setTimeout(2000);

  SetupWifi();

  g_MQTTClient.setServer(mqtt_server, MQTT_PORT);
  g_MQTTClient.setBufferSize(MQTT_MAX_SIZE);
  g_MQTTClient.setCallback(MQTTCallback);

  // Allow the hardware to sort itself out
  delay(1500);

  MQTTReconnect();
}


void loop()
{
  static elapsedMillis MQTTReconnectTimer = 0;
  static elapsedMillis WifiReconnectTimer = 0;
  static elapsedMillis ledTimer = 0;

  if (WiFi.status() != WL_CONNECTED) // Check for wifi disconnects
  {
#ifdef STATUS_LED
    digitalWrite(STATUS_LED, LOW); // Always on: failure
#endif

    if (WifiReconnectTimer > 5000)
    {
      Serial.print(millis());
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      MQTTReconnect();
      WifiReconnectTimer = 0;
      MQTTReconnectTimer = 0;
    }
  }
  else if (!g_MQTTClient.connected()) // Check for MQTT disconnects
  {
#ifdef STATUS_LED
    digitalWrite(STATUS_LED, LOW); // Always on: failure
#endif

    if (MQTTReconnectTimer > 5000)
    {
      MQTTReconnect();
      MQTTReconnectTimer = 0;
    }
  }
  else
  {
    // Indicate we're running:
#ifdef STATUS_LED
    if (ledTimer > 2000)
    {
      digitalWrite(STATUS_LED, HIGH); // Off
      ledTimer = 0;
    }
    else if (ledTimer > 1000)
    {
      digitalWrite(STATUS_LED, LOW); // On
    }

#endif

    g_MQTTClient.loop();

    // Handle OTA-updates
    ArduinoOTA.handle();
  }

  if (g_bZeroCrossTimeUpdated)
  {
    noInterrupts(); // Enter critical section

    g_bZeroCrossTimeUpdated = false;

    // FIXME: Perhaps handle this in timed loop?
    // Get updated values for triac drive
    g_fTriacAngleFactor = g_pvBoiler.GetTriacAngleFactor();
    g_iOutputPercentage = g_pvBoiler.GetOutputPercentage();

    interrupts(); // Leave critical section
  }

  g_pvBoiler.loop();
}
