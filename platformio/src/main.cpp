/*
  ESP PV-Boiler - ESP Controlled PV Boiler
  Last update: October 9, 2026
  (C) Copyright 2026 by Arno van Amersfoort
  Web                   : https://github.com/arnova/pvboiler
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
#include <EEPROM.h>

#include "Terminal.h"
#include "util.h"
#include "App.h"
#include "system.h"
#include "SettingLimits.h"
#include "EepromLayout.h"
#include "MqttTopics.h"

// Application global instance
CApp g_app;

// Interrupt generated when crossing zero in either direction
void IRAM_ATTR ZeroCrossISR()
{
  g_app.ZeroCrossHandler();
}


// Timer interrupt for triggering triac gate
void IRAM_ATTR TriacTimerISR()
{
  g_app.TriacGateHandler();
}


void MqttCallback(char* topic, byte *payload, const unsigned int length)
{
#ifdef MQTT_DEBUG
  CTerminal::println("-------new message from broker-----");
  CTerminal::print("topic: ");
  CTerminal::println(topic);
  CTerminal::print("data: ");
  for (unsigned int i = 0; i < length; i++)
  {
    CTerminal::print((char) payload[i]);
  }
  CTerminal::println("");
#endif

  float fVal;
  const bool bValidFloat = bytes_to_float(payload, length, &fVal);

  int32_t iVal;
  const bool bValidInt = bytes_to_int32(payload, length, &iVal);

  if (STRIEQUALS(topic, MQTT_HA_STATUS_TOPIC))
  {
    // Home Assistant (re)started: it lost our (non-retained) states so republish them
    if (length == 6 && strncasecmp((const char*) payload, "online", 6) == 0)
    {
      g_app.MqttRequestRepublish();
    }
    return;
  }

  // Got a message so network is still ok. Note: Not for Home Assistant's status, as that doesn't feed us any data
  g_app.GetPvBoiler().TrigNetworkWatchdog();

  if (STRIEQUALS(topic, MQTT_DSMR_ELECTRICITY_DELIVERED))
  {
    if (bValidFloat)
    {
      g_app.GetPvBoiler().SetPowerDelivered(lroundf(fVal * 1000.0f)); // kW -> Watt
    }
    else
    {
      CMqttClient::PrintDataError();
    }
    return;
  }

  if (STRIEQUALS(topic, MQTT_DSMR_ELECTRICITY_RETURNED))
  {
    if (bValidFloat)
    {
      g_app.GetPvBoiler().SetPowerReturned(lroundf(fVal * 1000.0f)); // kW -> Watt
    }
    else
    {
      CMqttClient::PrintDataError();
    }
    return;
  }

  // Use the MQTT client's hostname: it's what we subscribed with (a changed hostname only applies after MQTT reinit)
  const char* strHostName = g_app.GetNetwork().GetMqttClient().GetHostName();
  const size_t iLen = strlen(strHostName);
  if (strncmp(topic, strHostName, iLen) != 0 ||
      strlen(topic) < iLen + 2)
    return; // Hostname mismatch or length too small

  char strVal[32] = { 0 };
  memcpy(strVal, payload, (length < sizeof(strVal)) ? length : sizeof(strVal) - 1);

  // Get rid of host prefix
  char* subTopic = topic + iLen;

  if (STRIEQUALS(subTopic, "/" MQTT_SET_POWER_BUDGET "/set"))
  {
    if (bValidInt)
    {
      g_app.GetPvBoiler().SetPowerBudget(iVal);
    }
    else
    {
      CMqttClient::PrintDataError();
    }
  }
  else if (STRIEQUALS(subTopic, "/" MQTT_SET_POWER_PERCENTAGE "/set"))
  {
    if (bValidInt && iVal >=0 && iVal <= 100)
    {
      g_app.GetPvBoiler().SetPowerPercentage(iVal);
    }
    else
    {
      CMqttClient::PrintDataError();
    }
  }
  else if (STRIEQUALS(subTopic, "/" MQTT_SET_THERMOSTAT_SETPOINT "/set"))
  {
    if (bValidInt && iVal >= THERMOSTAT_SETPOINT_MIN && iVal <= THERMOSTAT_SETPOINT_MAX)
    {
      g_app.GetPvBoiler().SetThermostatSetpoint(iVal);
    }
    else
    {
      CMqttClient::PrintDataError();
    }
  }
  else if (STRIEQUALS(subTopic, "/" MQTT_LEGIONELLA_DISINFECTION_AGE "/set"))
  {
    if (bValidInt && iVal >=0)
    {
      g_app.GetPvBoiler().SetLegionellaDisinfectionAgeHours(iVal);
    }
    else
    {
      CMqttClient::PrintDataError();
    }
  }
  else if (STRIEQUALS(subTopic, "/" MQTT_LEGIONELLA_IN_DANGER_ZONE "/set"))
  {
    if (bValidInt && iVal >=0)
    {
      g_app.GetPvBoiler().SetLegionellaInDangerZoneHours(iVal);
    }
    else
    {
      CMqttClient::PrintDataError();
    }
  }
  else if (STRIEQUALS(subTopic, "/" MQTT_SET_MODE "/set"))
  {
    CPvBoiler::mode_t mode;
    if (CPvBoiler::GetModeFromName(strVal, mode))
    {
      g_app.GetPvBoiler().SetMode(mode);
    }
    else
    {
      CMqttClient::PrintDataError();
    }
  }
}


void setup()
{
  EEPROM.begin(EEPROM_MAX_SIZE); // Reserve room for eeprom settings

  randomSeed(micros());

  // Outputs
#ifdef STATUS_LED
  pinMode(STATUS_LED, OUTPUT);
#endif

  pinMode(TRIAC_OUTPUT, OUTPUT);
  digitalWrite(TRIAC_OUTPUT, LOW);

  pinMode(ZERO_CROSS_INPUT, INPUT_PULLUP);

  TERM_SERIAL.begin(BAUD_RATE);
  TERM_SERIAL.setTimeout(2000);

  // Init g_app instance
  g_app.Init();

  // Setup the MQTT client callback
  g_app.GetNetwork().GetMqttClient().setCallback(MqttCallback);

#ifdef ESP8266
  timer1_isr_init();
  timer1_attachInterrupt(TriacTimerISR);
  timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);   // TIM_SINGLE = single shot
#else // ESP32
  hw_timer_t* hTriacTimer = timerBegin(0, 80, true); // timer 0, /80 prescaler of 80 MHz APB → 1 MHz tick
  timerAttachInterrupt(hTriacTimer, &TriacTimerISR, true);
  g_app.SetTimerHandle(hTriacTimer);
#endif

  attachInterrupt(digitalPinToInterrupt(ZERO_CROSS_INPUT), ZeroCrossISR, CHANGE);

  // Allow the hardware to sort itself out
  delay(1500);

  // Have the terminal start with a newline
  CTerminal::println("");

  // Print out version info
  CTerminal::println(FPSTR(VER_STR_P));
}


void loop()
{
  g_app.Loop();
}
