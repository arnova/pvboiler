#include <ArduinoOTA.h>

#include "TermPrint.h"
#include "util.h"
#include "App.h"

CApp::CApp() : m_network(), m_pvBoiler(m_network), m_commandHandler(m_pvBoiler, m_network)
{
  m_display.Init();
}


void CApp::Init()
{
  m_network.LoadSettings();
  m_pvBoiler.LoadSettings();

  delay(10);

  m_network.InitWifi(false);

  if (IPAddress(m_network.GetServerIp()) != IPAddress(0, 0, 0, 0))
  {
    m_network.GetMqttClient().setServer(m_network.GetServerIp(), MQTT_PORT);
    m_network.GetMqttClient().setBufferSize(MQTT_MAX_SIZE);
  }
}


void CApp::Loop()
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

  m_pvBoiler.Loop();
}


bool CApp::MQTTReconnect()
{
  if (IPAddress(m_network.GetServerIp()) == IPAddress(0, 0, 0, 0))
  {
    return false;
  }

  if (!m_network.GetMqttClient().Reconnect())
  {
    return false;
  }

  // Publish MQTT config for eg. HA discovery and subscribe to control topics
  m_network.GetMqttClient().PublishSwitchConfig(MQTT_CONTROLLER_ON_OFF);

  if (m_pvBoiler.GetLogicMode() == CPVBoiler::LOGIC_MODE_PERCENTAGE)
  {
    m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_PERCENTAGE, "0", "100", "1");
  }
  else
  {
    m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_BUDGET, "-10000.0", "10000.0", "0.1");
  }

  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_POWER, "W", "power");

  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_PERCENTAGE, "%", "power_factor");

  // Publish our f/w version
  m_network.GetMqttClient().publish(MQTT_NAME "/" MQTT_FW_VERSION, MY_VERSION, true);

  return true;
}


void CApp::pollSerial(void)
{
  static uint8_t charCount = 0;
  static char strCommand[CMD_BUF_SIZE] = { 0 };
  static char strOldCommand[CMD_BUF_SIZE] = { 0 };

  if (TERM_SERIAL.available())
  {
    const char c = TERM_SERIAL.read();
    if (c != 0)
    {
      if (c == '!') // Repeat the previous command but don't execute it yet
      {
        if (charCount == 0 && *strOldCommand)
        {
          strcpy(strCommand, strOldCommand);
          charCount = strlen(strOldCommand);

          if (m_commandHandler.GetLocalEchoEnabled())
            TERM_SERIAL.print(strCommand);
        }
      }
      else if (c == CH_CR || c == CH_LF)       // if you've gotten to the end of the line, process it
      {
        // Linefeed for local echo
        if (m_commandHandler.GetLocalEchoEnabled())
          TERM_SERIAL.println("");

        // Don't check empty commands
        if (charCount > 0)
        {
          strCommand[charCount] = '\0';

          // Store in old buffer
          strcpy(strOldCommand, strCommand);

          // Reset counter for next round
          charCount = 0;

          // Parse uart command
          m_commandHandler.ProcessCommand(strCommand);
        }
      }
      else if (c == CH_DELETE || c == CH_BACKSPACE) //backspace OR delete (sometimes mixed up by terminal programs)
      {
        if (charCount > 0)
        {
          if (m_commandHandler.GetLocalEchoEnabled())
          {
            // Backspace
            TERM_SERIAL.write(CH_BACKSPACE);
            // Blank character
            TERM_SERIAL.write(' ');
            // And backspace again since the blank jumps forward
            TERM_SERIAL.write(CH_BACKSPACE);
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

          if (m_commandHandler.GetLocalEchoEnabled())
            TERM_SERIAL.write(c);
        }
      }
    }
  }
}


void CApp::pollEthernet(void)
{
  static uint8_t charCount = 0;
  static char strCommand[CMD_BUF_SIZE] = { 0 };
  static char strOldCommand[CMD_BUF_SIZE] = { 0 };

  WiFiClient& socketServerClient = m_network.GetSocketServerClient();
  if (socketServerClient && socketServerClient.connected())
  {
    if (socketServerClient.available()) // Data available?
    {
      const char c = socketServerClient.read(); // Read data from socket
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
              socketServerClient.print(strCommand);
#endif
          }
        }
        else if (c == CH_CR || c == CH_LF)       // if you've gotten to the end of the line, process it
        {
#if 0
          // Linefeed
          socketServerClient.println("");
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
            CTermPrint::SetSocketClient(socketServerClient);
            m_commandHandler.ProcessCommand(strCommand);
          }
        }
        else if (c == CH_DELETE || c == CH_BACKSPACE) //backspace OR delete (sometimes mixed up by terminal programs)
        {
          if (charCount > 0)
          {
#if 0
            if (m_commandHandler.GetLocalEchoEnabled())
            {
              // Backspace
              socketServerClient.write(CH_BACKSPACE);
              // Blank character
              socketServerClient.write(' ');
              // And backspace again since the blank jumps forward
              socketServerClient.write(CH_BACKSPACE);
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
            socketServerClient.write(c);
#endif
          }
        }
      }
    }
  }
}


bool CApp::CheckNetwork()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!m_bWifiConnected)
    {
#ifdef WIFI_DEBUG
      TERM_SERIAL.println("");
      TERM_SERIAL.print(PSTR("WiFi connected with IP address: "));
      TERM_SERIAL.println(WiFi.localIP().toString().c_str());
#endif
      m_bWifiConnected = true;
      m_wifiReconnectTimer = 0;

      MQTTReconnect();
      m_mqttReconnectTimer = 0;
    }

    // Check for MQTT disconnects
    if (!m_network.GetMqttClient().connected() && m_mqttReconnectTimer > 5000)
    {
      MQTTReconnect();
      m_mqttReconnectTimer = 0;
    }
  }
  else
  {
    m_bWifiConnected = false;
#ifdef STATUS_LED
    digitalWrite(STATUS_LED, LOW); // Always on: failure
#endif

    if (m_wifiReconnectTimer > WIFI_CONNECT_TIMEOUT)
    {
      m_network.InitWifi(true);
      m_wifiReconnectTimer = 0;
    }
  }
  
  if (!m_bWifiConnected || !m_network.GetMqttClient().connected())
  {
#ifdef STATUS_LED
    digitalWrite(STATUS_LED, LOW); // Always on: failure
#endif
  }
  else
  {
    m_network.GetMqttClient().loop();

    // Indicate we're running:
#ifdef STATUS_LED
    if (m_ledTimer > 2000)
    {
      digitalWrite(STATUS_LED, HIGH); // Off
      m_ledTimer = 0;
    }
    else if (m_ledTimer > 1000)
    {
      digitalWrite(STATUS_LED, LOW); // On
    }
#endif
  }

  return m_bWifiConnected;
}
