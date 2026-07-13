/*
  ESP PV-Boiler - ESP Controlled PV Boiler
  Last update: July 10, 2026
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

#include <ArduinoOTA.h>
#include <EEPROM.h>

#include "CommandParser.h"
#include "PvBoilerCommandHandler.h"
#include "pvboiler.h"
#include "TermPrint.h"
#include "MqttClient.h"
#include "ssd1306.h"
#include "util.h"
#include "Network.h"
#include "App.h"
#include "system.h"

CApp g_app;
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
    if (iNow - g_iLastZeroCrossTime < ZERO_CROSS_EDGE_MARGIN_US * 10)
    {
      return;
    }

    g_iZeroCrossTime = iNow - g_iLastZeroCrossTime;
    g_bZeroCrossTimeUpdated = true;
    g_iLastZeroCrossTime = iNow;

    if (g_app.GetPvBoiler().GetDimStyle() == CPVBoiler::DIM_STYLE_SSR)
    {
      if (g_iOutputPercentage == 0)
      {
        digitalWrite(TRIAC_OUTPUT, LOW); // Always off
      }
      else
      {
        g_iSSRPeriodCounter++;
        if ((g_iSSRPeriodCounter * 100) / g_app.GetPvBoiler().GetSsrPeriodCount() <= g_iOutputPercentage)
        {
          // Timer1 at DIV1 (80 MHz clock) → 80 ticks per µs
          // Maximum ~104 ms at this prescaler; no need for DIV256 in our range.
          const uint32_t iTriacDelayTicks = (g_iPhaseCorrectionTime + ZERO_CROSS_EDGE_MARGIN_US) * 80;

          g_bTriacOn = true;
          timer1_write(iTriacDelayTicks);
        }
        else
        {
          digitalWrite(TRIAC_OUTPUT, LOW); // Off
        }

        if (g_iSSRPeriodCounter >= g_app.GetPvBoiler().GetSsrPeriodCount())
        {
          g_iSSRPeriodCounter = 0;
        }
      }
    }
    else
    {
      digitalWrite(TRIAC_OUTPUT, LOW); // Off

      const float fDelay = max((g_fTriacAngleFactor * g_iZeroCrossTime), ZERO_CROSS_EDGE_MARGIN_US); // Make sure we trigger not too close to zero cross

      // NOTE: Only turn on triac when NOT near 0% to prevent excessive EMI due to misfiring
      if (fDelay + ZERO_CROSS_EDGE_MARGIN_US + GATE_PULSE_WIDTH <= g_iZeroCrossTime)
      {
        // Timer1 at DIV1 (80 MHz clock) → 80 ticks per µs
        // Maximum ~104 ms at this prescaler; no need for DIV256 in our range.
        const uint32_t iTriacDelayTicks = (fDelay + g_iPhaseCorrectionTime) * 80;

        g_bTriacOn = true;
        timer1_write(iTriacDelayTicks);
      }
    }
  }
  else // Falling edge
  {
    // filter noise
    if (iNow - g_iLastZeroCrossTime < ZERO_CROSS_EDGE_MARGIN_US)
    {
      return;
    }

    // NOTE: The time between rising edge and falling edge is used (/2) for phase correction
    g_iPhaseCorrectionTime = (iNow - g_iLastZeroCrossTime) / 2;
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
  CTermPrint::println("-------new message from broker-----");
  CTermPrint::print("topic: ");
  CTermPrint::println(topic);
  CTermPrint::print("data: ");
  for (unsigned int i = 0; i < length; i++)
  {
    CTermPrint::print((char) payload[i]);
  }
  CTermPrint::println("");

  //float fVal;
  //const bool bValidFloat = BytesToFloat(payload, length, fVal);

  int32_t iVal;
  const bool bValidInt = bytes_to_int32(payload, length, &iVal);

  if (STRIEQUALS(topic, MQTT_NAME "/" MQTT_CONTROLLER_ON_OFF "/set"))
  {
    if (bValidInt || length == 0)
    {
      if (iVal == 0 || iVal == 1 || length == 0)
      {
        g_app.GetPvBoiler().SetOnOff((iVal == 1 || length == 0) ? true : false);
      }
      else
      {
        CMqttClient::PrintDataError();
      }
    }
    else
    {
      CMqttClient::PrintDataError();
    }
  }
  else if (!g_app.GetPvBoiler().GetLogicMode() == CPVBoiler::LOGIC_MODE_BUDGET && STRIEQUALS(topic, MQTT_NAME "/" MQTT_SET_POWER_BUDGET "/set"))
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
  else if (g_app.GetPvBoiler().GetLogicMode() == CPVBoiler::LOGIC_MODE_PERCENTAGE && STRIEQUALS(topic, MQTT_NAME "/" MQTT_SET_POWER_PERCENTAGE "/set"))
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

  // Got a message so network is still ok:
  g_app.GetPvBoiler().TriggerWatchdog();
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

  g_iLastZeroCrossTime = g_iZeroCrossTime = micros();

  timer1_isr_init();
  timer1_attachInterrupt(TriacTimerISR);
  timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);   // TIM_SINGLE = single shot

  EEPROM.begin(128); // Reserve room for eeprom settings

  randomSeed(micros());

  Serial.begin(BAUD_RATE);
  Serial.setTimeout(2000);

  g_app.GetNetwork().LoadSettings();
  g_app.GetPvBoiler().LoadSettings();

  delay(10);

  g_app.GetNetwork().InitWifi(false);

  if (IPAddress(g_app.GetNetwork().GetServerIp()) != IPAddress(0, 0, 0, 0))
  {
    g_app.GetNetwork().GetMqttClient().setServer(g_app.GetNetwork().GetServerIp(), MQTT_PORT);
    g_app.GetNetwork().GetMqttClient().setBufferSize(MQTT_MAX_SIZE);
    g_app.GetNetwork().GetMqttClient().setCallback(MQTTCallback);
  }

  // Allow the hardware to sort itself out
  delay(1500);

  g_app.MQTTReconnect();

    // Have the terminal start with a newline
  CTermPrint::println("");

  // Print out version info
  CTermPrint::println(VER_STR);
}


#if 0
  //FIXME
    case CMD_RESPONSE_SERVER:
    {
      g_heliumPurityMeter.SetResponseServerOK(true);
    }
    break;
#endif


void loop()
{
  if (g_app.CheckNetwork())
  {
    // Handle OTA-updates
    ArduinoOTA.handle();

    // Poll ethernet for commands
    g_app.pollEthernet();
  }

  // Poll serial for commands
  g_app.pollSerial();

  // FIXME: Remove this from loop()?
  if (g_bZeroCrossTimeUpdated)
  {
    noInterrupts(); // Enter critical section

    g_bZeroCrossTimeUpdated = false;

    // FIXME: Perhaps handle this in timed loop?
    // Get updated values for triac drive
    g_fTriacAngleFactor = g_app.GetPvBoiler().GetTriacAngleFactor();
    g_iOutputPercentage = g_app.GetPvBoiler().GetOutputPercentage();

    interrupts(); // Leave critical section
  }

  g_app.GetPvBoiler().loop();
}
