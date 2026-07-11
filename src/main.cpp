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
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#include "CommandParser.h"
#include "PvBoilerCommandHandler.h"
#include "pvboiler.h"
#include "mqttutil.h"
#include "ssd1306.h"
#include "util.h"
#include "Network.h"
#include "system.h"

// Globals
WiFiClient g_wifiClient;
PubSubClient g_MQTTClient(g_wifiClient);
CMqttUtil g_MQTTUtil(g_MQTTClient);
CNetwork g_network(g_MQTTClient);
CPVBoiler g_pvBoiler(g_MQTTClient);
CPVBoilerCommandHandler g_commandHandler(g_pvBoiler, g_network);

volatile uint32_t g_iLastZeroCrossTime = 0;
volatile uint32_t g_iPhaseCorrectionTime = 300; // Default = 300 uS
volatile uint32_t g_iZeroCrossTime = 0;
volatile bool g_bZeroCrossTimeUpdated = false;
volatile bool g_bTriacOn = false;
volatile float g_fTriacAngleFactor = 1.0f; // Off
volatile uint8_t g_iOutputPercentage = 0;
volatile uint8_t g_iSSRPeriodCounter = 0;

// Define print output
void PrintStrN(const char *str)
{
  Serial.println(str); // Always print to uart

  if (g_network.GetSocketServerClient().connected())
  {
    g_network.GetSocketServerClient().println(str);
  }
}


void PrintStr(const char *str)
{
  Serial.print(str); // Always print to uart

  if (g_network.GetSocketServerClient().connected())
  {
    g_network.GetSocketServerClient().print(str);
  }
}


void PrintChar(const char c)
{
  Serial.print(c); // Always print to uart

  if (g_network.GetSocketServerClient().connected())
  {
    g_network.GetSocketServerClient().print(c);
  }
}


void PrintInt32(const int32_t i)
{
  Serial.print(i); // Always print to uart

  if (g_network.GetSocketServerClient().connected())
  {
    g_network.GetSocketServerClient().print(i);
  }
}


void PrintFloat(const float f)
{
  Serial.print(f); // Always print to uart

  if (g_network.GetSocketServerClient().connected())
  {
    g_network.GetSocketServerClient().print(f);
  }
}


result_code_t CommandReboot()
{
  ESP.restart();

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CommandReset()
{
  // FIXME: Implementation

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CommandInfo()
{
  PrintStr("ssid=");
  PrintStr(g_network.GetWifiSsid());

  PrintStr(" pass=");
  PrintStr(g_network.GetWifiPassword());

  PrintStr(" ip=");
  PrintInt32(g_network.GetIpAddr()[0]);
  PrintChar('.');
  PrintInt32(g_network.GetIpAddr()[1]);
  PrintChar('.');
  PrintInt32(g_network.GetIpAddr()[2]);
  PrintChar('.');
  PrintInt32(g_network.GetIpAddr()[3]);

  PrintStr(" netmask=");
  PrintInt32(g_network.GetNetMask()[0]);
  PrintChar('.');
  PrintInt32(g_network.GetNetMask()[1]);
  PrintChar('.');
  PrintInt32(g_network.GetNetMask()[2]);
  PrintChar('.');
  PrintInt32(g_network.GetNetMask()[3]);

  PrintStr(" server=");
  PrintInt32(g_network.GetServerIp()[0]);
  PrintChar('.');
  PrintInt32(g_network.GetServerIp()[1]);
  PrintChar('.');
  PrintInt32(g_network.GetServerIp()[2]);
  PrintChar('.');
  PrintInt32(g_network.GetServerIp()[3]);

  PrintStr(" bprating=");
  PrintInt32(g_pvBoiler.GetBoilerPowerRating());
  PrintStr("W");

  PrintStr(" pbmargin=");
  PrintInt32(g_pvBoiler.GetPowerBudgetMargin());
  PrintStr("W");

  PrintStr(" ctrl_mode=");
  PrintStr(g_pvBoiler.GetLogicMode() ? "percentage" : "budget");

  PrintStr(" dstyle=");
  PrintStr(g_pvBoiler.GetDimStyle() == CPVBoiler::DIM_STYLE_PHASE_CUT ? "phase-cut" : "ssr");

  PrintStr(" ssrpc=");
  PrintInt32(g_pvBoiler.GetSSRPeriod());

  PrintStr(" egain=");
  PrintFloat(g_pvBoiler.GetErrorGain());

  PrintStrN("");

  return pack_result_code(ERR_CODE_OK);
}


result_code_t CommandStatus()
{
  PrintStr("on_off=");
  PrintStr(g_pvBoiler.GetCtrlOnOff() ? "on" : "off");

  PrintStr(" wifi_conn=");
  PrintStr(WiFi.status() == WL_CONNECTED ? "1" : "0");

  PrintStr(" wifi_ip=");
  PrintStr(WiFi.localIP().toString().c_str());

  PrintStr(" mqtt_conn=");
  PrintStr(g_MQTTClient.connected() ? "1" : "0");

  PrintStr(" angle_factor=");
  PrintFloat(g_pvBoiler.GetTriacAngleFactor());

  PrintStr(" out_perc=");
  PrintInt32(g_pvBoiler.GetOutputPercentage());

  PrintStr(" p_budget=");
  PrintInt32(g_pvBoiler.GetPowerBudget());
  PrintStr("W");

  PrintStr(" p_percentage=");
  PrintInt32(g_pvBoiler.GetPowerPercentage());
  PrintStr("%");

  PrintStrN("");

  return pack_result_code(ERR_CODE_OK);
}


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

#ifdef SSR_STYLE_MODE
    if (g_iOutputPercentage == 0)
    {
      digitalWrite(TRIAC_OUTPUT, LOW); // Always off
    }
    else
    {
      g_iSSRPeriodCounter++;
      if ((g_iSSRPeriodCounter * 100) / m_iSsrPeriodCount <= g_iOutputPercentage)
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

      if (g_iSSRPeriodCounter >= m_iSsrPeriodCount)
      {
        g_iSSRPeriodCounter = 0;
      }
    }
#else
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
#endif
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
  PrintStrN("-------new message from broker-----");
  PrintStr("topic: ");
  PrintStrN(topic);
  PrintStr("data: ");
  for (unsigned int i = 0; i < length; i++)
  {
    PrintChar((char) payload[i]);
  }
  PrintStrN("");

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
        g_pvBoiler.SetOnOff((iVal == 1 || length == 0) ? true : false);
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
  else if (!g_pvBoiler.GetLogicMode() && STRIEQUALS(topic, MQTT_NAME "/" MQTT_SET_POWER_BUDGET "/set"))
  {
    if (bValidInt)
    {
      g_pvBoiler.SetPowerBudget(iVal);
    }
    else
    {
      CMqttUtil::PrintDataError();
    }
  }
  else if (g_pvBoiler.GetLogicMode() && STRIEQUALS(topic, MQTT_NAME "/" MQTT_SET_POWER_PERCENTAGE "/set"))
  {
    if (bValidInt && iVal >=0 && iVal <= 100)
    {
      g_pvBoiler.SetPowerPercentage(iVal);
    }
    else
    {
      CMqttUtil::PrintDataError();
    }
  }

  // Got a message so network is still ok:
  g_pvBoiler.TriggerWatchdog();
}


bool MQTTReconnect()
{
  if (IPAddress(g_network.GetServerIp()) == IPAddress(0, 0, 0, 0))
  {
    return false;
  }

  if (!g_MQTTUtil.Reconnect())
  {
    return false;
  }

  // Publish MQTT config for eg. HA discovery and subscribe to control topics
  g_MQTTUtil.PublishSwitchConfig(MQTT_CONTROLLER_ON_OFF);

  if (g_pvBoiler.GetLogicMode())
  {
    g_MQTTUtil.PublishNumberConfig(MQTT_SET_POWER_PERCENTAGE, "0", "100", "1");
  }
  else
  {
    g_MQTTUtil.PublishNumberConfig(MQTT_SET_POWER_BUDGET, "-10000.0", "10000.0", "0.1");
  }

  g_MQTTUtil.PublishSensorConfig(MQTT_OUTPUT_POWER, "W", "power");

  g_MQTTUtil.PublishSensorConfig(MQTT_OUTPUT_PERCENTAGE, "%", "power_factor");

  // Publish our f/w version
  g_MQTTClient.publish(MQTT_NAME "/" MQTT_FW_VERSION, MY_VERSION, true);

  return true;
}


void LoadSettings()
{
  g_network.LoadSettings();

  uint16_t iVal16 = 0;
  EEPROM.get(EEPROM_BP_RATING, iVal16);
  if (iVal16 > BOILER_POWER_RATING_MAX)
  {
    iVal16 = BOILER_POWER_RATING_DEFAULT;
  }
  g_pvBoiler.SetBoilerPowerRating(iVal16);

  EEPROM.get(EEPROM_PB_MARGIN, iVal16);
  if (iVal16 > POWER_BUDGET_MARGIN_MAX)
  {
    iVal16 = POWER_BUDGET_MARGIN_DEFAULT;
  }
  g_pvBoiler.SetPowerBudgetMargin(iVal16);

  uint8_t iVal8 = 0;
  EEPROM.get(EEPROM_CTRL_MODE, iVal8);
  g_pvBoiler.SetLogicMode(iVal8 != 0); // Percentage(true) or budget(false) ?

  EEPROM.get(EEPROM_DIM_STYLE, iVal8);
  g_pvBoiler.SetDimStyle((iVal8 != 0) ? CPVBoiler::DIM_STYLE_SSR : CPVBoiler::DIM_STYLE_PHASE_CUT);

  EEPROM.get(EEPROM_SSR_PERIOD, iVal8);
  if (iVal8 > SSR_PERIOD_COUNT_MAX)
  {
    iVal8 = SSR_PERIOD_COUNT_DEFAULT;
  }
  g_pvBoiler.SetSsrPeriodCount(iVal8);

  float fGain;
  EEPROM.get(EEPROM_ERROR_GAIN, fGain);
  if (fGain > ERROR_GAIN_MAX || fGain < ERROR_GAIN_MIN)
  {
    fGain = ERROR_GAIN_DEFAULT;
  }
  g_pvBoiler.SetErrorGain(fGain);
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

  LoadSettings();

  delay(10);

  g_network.InitWifi(false);

  if (IPAddress(g_network.GetServerIp()) != IPAddress(0, 0, 0, 0))
  {
    g_MQTTClient.setServer(g_network.GetServerIp(), MQTT_PORT);
    g_MQTTClient.setBufferSize(MQTT_MAX_SIZE);
    g_MQTTClient.setCallback(MQTTCallback);
  }

  // Allow the hardware to sort itself out
  delay(1500);

  MQTTReconnect();

    // Have the terminal start with a newline
  PrintStrN("");

  // Print out version info
  PrintStrN(VER_STR);
}


#if 0
  //FIXME
    case CMD_RESPONSE_SERVER:
    {
      g_heliumPurityMeter.SetResponseServerOK(true);
    }
    break;
#endif



void pollSerial(void)
{
  static uint8_t charCount = 0;
  static char strCommand[CMD_BUF_SIZE] = { 0 };
  static char strOldCommand[CMD_BUF_SIZE] = { 0 };

  if (Serial.available())
  {
    const char c = Serial.read();
    if (c != 0)
    {
      if (c == '!') // Repeat the previous command but don't execute it yet
      {
        if (charCount == 0 && *strOldCommand)
        {
          strcpy(strCommand, strOldCommand);
          charCount = strlen(strOldCommand);

          if (g_commandHandler.GetLocalEchoEnabled())
            Serial.print(strCommand);
        }
      }
      else if (c == CH_CR || c == CH_LF)       // if you've gotten to the end of the line, process it
      {
        // Linefeed for local echo
        if (g_commandHandler.GetLocalEchoEnabled())
          Serial.println("");

        // Don't check empty commands
        if (charCount > 0)
        {
          strCommand[charCount] = '\0';

          // Store in old buffer
          strcpy(strOldCommand, strCommand);

          // Reset counter for next round
          charCount = 0;

          // Parse uart command
          g_commandHandler.ProcessCommand(strCommand);
        }
      }
      else if (c == CH_DELETE || c == CH_BACKSPACE) //backspace OR delete (sometimes mixed up by terminal programs)
      {
        if (charCount > 0)
        {
          if (g_commandHandler.GetLocalEchoEnabled())
          {
            // Backspace
            Serial.write(CH_BACKSPACE);
            // Blank character
            Serial.write(' ');
            // And backspace again since the blank jumps forward
            Serial.write(CH_BACKSPACE);
          }
          charCount--;
        }
      }
      else if (c >= ' ' && c <= '~') // Limit allowed characters
      {
        // Don't overflow + skip leading spaces:
        if (charCount < (CMD_BUF_SIZE - 1) && !(charCount == 0 && c == ' '))
        {
          strCommand[charCount++] = c;

          if (g_commandHandler.GetLocalEchoEnabled())
            Serial.write(c);
        }
      }
    }
  }
}


void pollEthernet(void)
{
  static uint8_t charCount = 0;
  static char strCommand[CMD_BUF_SIZE] = { 0 };
  static char strOldCommand[CMD_BUF_SIZE] = { 0 };

  if (!g_network.GetSocketServerClient() || !g_network.GetSocketServerClient().connected())
  {
    g_network.GetSocketServerClient() = g_network.GetSocketServer().accept();
#ifdef WIFI_DEBUG
    if (g_network.GetSocketServerClient())
    {
      PrintStr("Accepting connection from: ");
      PrintStrN(g_network.GetSocketServerClient().remoteIP().toString().c_str());
    }
#endif
  }

  if (g_network.GetSocketServerClient() && g_network.GetSocketServerClient().connected())
  {
    if (g_network.GetSocketServerClient().available())
    {
      const char c = g_network.GetSocketServerClient().read();
      if (c != 0)
      {
        if (c == '!') // Repeat the previous command but don't execute it yet
        {
          if (charCount == 0 && *strOldCommand)
          {
            strcpy(strCommand, strOldCommand);
            charCount = strlen(strOldCommand);

#if 0
            if (commandHandler.GetLocalEchoEnabled())
              g_network.GetSocketServerClient().print(strCommand);
#endif
          }
        }
        else if (c == CH_CR || c == CH_LF)       // if you've gotten to the end of the line, process it
        {
#if 0
          // Linefeed
          g_network.GetSocketServerClient().println("");
#endif

          // Don't check empty commands
          if (charCount > 0)
          {
            strCommand[charCount] = '\0';

            // Store in old buffer
            strcpy(strOldCommand, strCommand);

            // Reset counter for next round
            charCount = 0;

            // Parse client command
            g_commandHandler.ProcessCommand(strCommand, &g_network.GetSocketServerClient());
          }
        }
        else if (c == CH_DELETE || c == CH_BACKSPACE) //backspace OR delete (sometimes mixed up by terminal programs)
        {
          if (charCount > 0)
          {
#if 0
            if (g_commandHandler.GetLocalEchoEnabled())
            {
              // Backspace
              g_socketServerClient.write(CH_BACKSPACE);
              // Blank character
              g_socketServerClient.write(' ');
              // And backspace again since the blank jumps forward
              g_socketServerClient.write(CH_BACKSPACE);
            }
#endif
            charCount--;
          }
        }
        else if (c >= ' ' && c <= '~') // Limit allowed characters
        {
          // Don't overflow + skip leading spaces:
          if (charCount < (CMD_BUF_SIZE - 1) && !(charCount == 0 && c == ' '))
          {
            strCommand[charCount++] = c;
#if 0
            g_socketServerClient.write(c);
#endif
          }
        }
      }
    }
  }
}


bool CheckNetwork()
{
  static elapsedMillis MQTTReconnectTimer = 0;
  static elapsedMillis WifiReconnectTimer = 0;
  static elapsedMillis ledTimer = 0;
  static bool bWifiConnected = false;

  if (WiFi.status() == WL_CONNECTED)
  {
    if (!bWifiConnected)
    {
#ifdef WIFI_DEBUG
      PrintStrN("");
      PrintStrN("WiFi connected");
      PrintStr("IP address: ");
      PrintStrN(WiFi.localIP().toString().c_str());
#endif
      bWifiConnected = true;
      WifiReconnectTimer = 0;

      MQTTReconnect();
      MQTTReconnectTimer = 0;
    }

    // Check for MQTT disconnects
    if (!g_MQTTClient.connected() && MQTTReconnectTimer > 5000)
    {
      MQTTReconnect();
      MQTTReconnectTimer = 0;
    }
  }
  else
  {
    bWifiConnected = false;
#ifdef STATUS_LED
    digitalWrite(STATUS_LED, LOW); // Always on: failure
#endif

    if (WifiReconnectTimer > WIFI_CONNECT_TIMEOUT)
    {
#ifdef WIFI_DEBUG
      Serial.print(millis());
      Serial.println(" - (Re)connecting to WiFi...");
#endif
      WiFi.disconnect();
      WiFi.reconnect();
      WifiReconnectTimer = 0;
    }
  }
  
  if (!bWifiConnected || !g_MQTTClient.connected())
  {
#ifdef STATUS_LED
    digitalWrite(STATUS_LED, LOW); // Always on: failure
#endif
  }
  else
  {
    g_MQTTClient.loop();

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
  }

  return bWifiConnected;
}


void loop()
{
  if (CheckNetwork())
  {
    // Handle OTA-updates
    ArduinoOTA.handle();

    // Poll ethernet for commands
    pollEthernet();
  }

  // Poll serial for commands
  pollSerial();

  // FIXME: Remove this from loop()?
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
