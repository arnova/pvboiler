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
#include <EEPROM.h>

#include "TermPrint.h"
#include "util.h"
#include "App.h"
#include "system.h"

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
  CTermPrint::println("-------new message from broker-----");
  CTermPrint::print("topic: ");
  CTermPrint::println(topic);
  CTermPrint::print("data: ");
  for (unsigned int i = 0; i < length; i++)
  {
    CTermPrint::print((char) payload[i]);
  }
  CTermPrint::println("");
#endif
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
  else if (g_app.GetPvBoiler().GetLogicMode() == CPVBoiler::LOGIC_MODE_PERCENT && STRIEQUALS(topic, MQTT_NAME "/" MQTT_SET_POWER_PERCENTAGE "/set"))
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
  EEPROM.begin(128); // Reserve room for eeprom settings

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

  // Setup the MQTT client callback
  g_app.GetNetwork().GetMqttClient().setCallback(MqttCallback);

  timer1_isr_init();
  timer1_attachInterrupt(TriacTimerISR);
  timer1_enable(TIM_DIV1, TIM_EDGE, TIM_SINGLE);   // TIM_SINGLE = single shot

  attachInterrupt(digitalPinToInterrupt(ZERO_CROSS_INPUT), ZeroCrossISR, CHANGE);

    // Allow the hardware to sort itself out
  delay(1500);

  // Have the terminal start with a newline
  CTermPrint::println("");

  // Print out version info
  CTermPrint::println(FPSTR(VER_STR_P));
}


void loop()
{
  g_app.Loop();
}
