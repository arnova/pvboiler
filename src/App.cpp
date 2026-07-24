#include "TermPrint.h"
#include "util.h"
#include "App.h"

CApp::CApp() : m_pvBoiler(m_network), m_commandHandler(m_pvBoiler, m_network)
{
  m_iLastZeroCrossTime = m_iPeriodTime = m_iLastEventTime = micros();
}


void CApp::Init()
{
  m_display.Init();
  m_display.WriteDisplayStr(DEVICE_NAME);
  m_display.WriteDisplayStr("v" MY_VERSION, 1, false);
  m_display.WriteDisplayStr("C) Arnova", 2, false);

  m_network.Init();

  m_pvBoiler.Reset();
}


void IRAM_ATTR CApp::ZeroCrossHandler()
{
  const uint32_t iNow = micros();

  /*
   * Ignore pulse when it's less than ZERO_CROSS_EDGE_MIN_US, when it's more and less then ZERO_CROSS_EDGE_MAX_US
   * consider it the zero cross (short) pulse, of it's more consider it the (long) remainder of the period
   */
  const uint32_t iPulseWidth = iNow - m_iLastEventTime;
  bool bLongPulse = true;
  if (iPulseWidth < ZERO_CROSS_EDGE_MIN_US)
  {
    return; // Filter noise
  }
  else if (iPulseWidth < ZERO_CROSS_EDGE_MAX_US)
  {
    bLongPulse = false;
  }

  // Update last even time
  m_iLastEventTime = iNow;

  // Pulse is longer than ~65 ms?
  if (iPulseWidth > 65535)
  {
    m_iPeriodTime = 65535; // Cap to max uint16 value
    return;
  }

  if (bLongPulse)
  {
    m_iPeriodTime = iNow - m_iLastZeroCrossTime;

    m_iLastZeroCrossTime = iNow;

    if (m_dimStyle == CPvBoiler::DIM_STYLE_SSR)
    {
      if (m_iCurrentPercentage == 0)
      {
        digitalWrite(TRIAC_OUTPUT, LOW); // Always off
      }
      else
      {
        m_iSSRPeriodCounter++;
        if ((m_iSSRPeriodCounter * 100) / m_iSSRPeriodCount <= m_iCurrentPercentage)
        {
          m_bTriacOn = true;
#ifdef ESP8266
          timer1_write(m_iTriacDelayUs * ESP8266_TICKS_PER_US);
#else
          timerWrite(m_hTriacTimer, 0);
          timerAlarmWrite(m_hTriacTimer, m_iTriacDelayUs, false); // one-shot, GATE_PULSE_WIDTH in µs since tick = 1µs
          timerAlarmEnable(m_hTriacTimer);
#endif
        }
        else
        {
          digitalWrite(TRIAC_OUTPUT, LOW); // Off
        }

        if (m_iSSRPeriodCounter >= m_iSSRPeriodCount)
        {
          m_iSSRPeriodCounter = 0;
        }
      }
    }
    else if (m_dimStyle == CPvBoiler::DIM_STYLE_PHASE_ANGLE)
    {
      digitalWrite(TRIAC_OUTPUT, LOW); // Off

      // NOTE: m_iTriacDelayUs is 0 when for whatever reason triac should not be turned on
      if (m_iTriacDelayUs > 0)
      {
        m_bTriacOn = true;

#ifdef ESP8266
        timer1_write(m_iTriacDelayUs * ESP8266_TICKS_PER_US);
#else
        timerWrite(m_hTriacTimer, 0);
        timerAlarmWrite(m_hTriacTimer, m_iTriacDelayUs, false); // one-shot, GATE_PULSE_WIDTH in µs since tick = 1µs
        timerAlarmEnable(m_hTriacTimer);
#endif
      }
    }
  }
  else // Short pulse
  {
    // NOTE: The time between rising edge and falling edge is used
    m_iZeroCrossWindow = iPulseWidth;
  }
}


void IRAM_ATTR CApp::TriacGateHandler()
{
  if (m_bTriacOn)
  {
    digitalWrite(TRIAC_OUTPUT, HIGH); // On

    m_bTriacOn = false;
#ifdef ESP8266
    timer1_write(GATE_PULSE_WIDTH * ESP8266_TICKS_PER_US);
#else // ESP32
    timerWrite(m_hTriacTimer, 0);
    timerAlarmWrite(m_hTriacTimer, GATE_PULSE_WIDTH, false); // one-shot, GATE_PULSE_WIDTH in µs since tick = 1µs
    timerAlarmEnable(m_hTriacTimer);
#endif
  }
  else
  {
    digitalWrite(TRIAC_OUTPUT, LOW); // Off
  }
}


void CApp::PollSerial(void)
{
  if (TERM_SERIAL.available())
  {
    const char c = TERM_SERIAL.read();
    if (c != 0)
    {
      if (c == '!') // Repeat the previous command but don't execute it yet
      {
        if (m_termCharCount == 0 && *m_strTermOldCommand)
        {
          strcpy(m_strTermCommand, m_strTermOldCommand);
          m_termCharCount = strlen(m_strTermOldCommand);

          if (m_commandHandler.GetLocalEchoEnabled())
            TERM_SERIAL.print(m_strTermCommand);
        }
      }
      else if (c == CH_CR || c == CH_LF)       // if you've gotten to the end of the line, process it
      {
        // Linefeed for local echo
        if (m_commandHandler.GetLocalEchoEnabled())
          TERM_SERIAL.println("");

        // Don't check empty commands
        if (m_termCharCount > 0)
        {
          m_strTermCommand[m_termCharCount] = '\0';

          // Store in old buffer
          strcpy(m_strTermOldCommand, m_strTermCommand);

          // Reset counter for next round
          m_termCharCount = 0;

          // Parse uart command
          m_commandHandler.ProcessCommand(m_strTermCommand);
        }
      }
      else if (c == CH_DELETE || c == CH_BACKSPACE) //backspace OR delete (sometimes mixed up by terminal programs)
      {
        if (m_termCharCount > 0)
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
          m_termCharCount--;
        }
      }
      else if (c >= ' ' && c <= '~') // Limit allowed characters
      {
        // Don't overflow + skip leading spaces:
        if (m_termCharCount < (CMD_BUF_SIZE - 1) && !(m_termCharCount == 0 && c == ' '))
        {
          m_strTermCommand[m_termCharCount++] = c;

          if (m_commandHandler.GetLocalEchoEnabled())
            TERM_SERIAL.write(c);
        }
      }
    }
  }
}


void CApp::PollEthernet(void)
{
  if (!m_network.IsConnected())
  {
    return;
  }

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
          if (m_termCharCount == 0 && *m_strTermOldCommand)
          {
            strcpy(m_strTermCommand, m_strTermOldCommand);
            m_termCharCount = strlen(m_strTermOldCommand);

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
          if (m_termCharCount > 0)
          {
            m_strTermCommand[m_termCharCount] = '\0';

            // Store in old buffer
            strcpy(m_strTermOldCommand, m_strTermCommand);

            // Reset counter for next round
            m_termCharCount = 0;

            // Parse client command
            CTermPrint::SetSocketClient(socketServerClient);
            m_commandHandler.ProcessCommand(m_strTermCommand);
          }
        }
        else if (c == CH_DELETE || c == CH_BACKSPACE) //backspace OR delete (sometimes mixed up by terminal programs)
        {
          if (m_termCharCount > 0)
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
            m_termCharCount--;
          }
        }
        else if (c >= ' ' && c <= '~') // Limit allowed characters
        {
          // Don't overflow + skip leading spaces:
          if (m_termCharCount < (CMD_BUF_SIZE - 1) && !(m_termCharCount == 0 && c == ' '))
          {
            m_strTermCommand[m_termCharCount++] = c;
#if 0
            socketServerClient.write(c);
#endif
          }
        }
      }
    }
  }
}


void CApp::HandleNetwork()
{
  m_network.Loop();

  if (m_network.HandleMqttClient())
  {
    // MQTT (re)connection: publish config & values
    m_pvBoiler.MqttPublishConfig();
    m_pvBoiler.MqttPublishValues();
    m_network.MqttPublishValues();
  }

  if (!m_network.IsConnected() || !m_network.IsMqttConnected())
  {
#ifdef STATUS_LED
    digitalWrite(STATUS_LED, LOW); // Always on: failure
#endif
  }
  else
  {
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
}


void CApp::HandleDisplay()
{
  if (m_displayTimer > 5000)
  {
    m_displayTimer = 0;
    String strValue;

    switch (m_displayCount)
    {
      case 0 : {
                 m_display.WriteDisplayStr(DEVICE_NAME, 0, true);
                 m_display.WriteDisplayStr("v" MY_VERSION, 1, false);
                 m_display.WriteDisplayStr("(C) Arnova", 2, false);
               } 
               break;

      case 1 : {
                 m_display.WriteDisplayStr(m_network.GetWifiSsid(), 0, true);

                 strValue = WiFi.localIP().toString();
                 m_display.WriteDisplayStr(strValue.c_str(), 1, false);

                 if (!m_network.IsConnected())
                 {
                   m_display.WriteDisplayStr("WiFi error", 2, false);
                 }
                 else if (!m_network.IsMqttConnected())
                 {
                   m_display.WriteDisplayStr("MQTT error", 2, false);
                 }
                 else
                 {
                   m_display.WriteDisplayStr("Connection OK", 2, false);
                 }
               }
               break;

      case 2 : {
                 strValue = String(m_pvBoiler.GetCurrentPower()) + "W";
                 m_display.WriteDisplayStr(strValue.c_str(), 0, true);

                 const uint8_t iPercent = m_pvBoiler.GetCurrentPercentage();
                 strValue = String(iPercent) + "%";
                 m_display.WriteDisplayStr(strValue.c_str(), 1, false);

                 strValue = "[";

                 // Chars are not monospace so need to compensate for smaller spaces with the logic below
                 for (uint8_t iCount = 0; iCount < 100;)
                 {
                   if (iCount < iPercent)
                   {
                     strValue += "=";
                     iCount += 10;
                   }
                   else
                   {
                     strValue += " ";
                     iCount += 5; // Spaces are smaller
                   }
                 }
                 strValue += "]";
                 m_display.WriteDisplayStr(strValue.c_str(), 2, false);
               }
               break;

      case 3 : {
                 m_display.WriteDisplayStr("", 0, true); // Empty screen to preven burnin
               }
               break;
    }

    if (++m_displayCount > 3)
    {
      m_displayCount = 0;
    }
  }
}


// Update values from PvBoiler for triac ISR
void CApp::UpdateValues()
{
  noInterrupts(); // Enter critical section

  // Get current phase correction & zero cross time value from ISR
  const uint16_t iZeroCrossWindow = m_iZeroCrossWindow;
  const uint16_t iPeriodTime = m_iPeriodTime;

  interrupts(); // Leave critical section

  // Get updated values for ISR
  const uint8_t iCurrentPercentage = m_pvBoiler.GetCurrentPercentage();
  const uint8_t iSSRPeriodCount = m_pvBoiler.GetSsrPeriodCount();
  const CPvBoiler::dim_style_t dimStyle = m_pvBoiler.GetDimStyle();

  const uint16_t iTriacDelayUs = m_pvBoiler.CalculateTriacPhaseDelay(iPeriodTime, iZeroCrossWindow);

  noInterrupts(); // Enter critical section

  m_iCurrentPercentage = iCurrentPercentage;
  m_iSSRPeriodCount = iSSRPeriodCount;
  m_dimStyle = dimStyle;
  m_iTriacDelayUs = iTriacDelayUs;

  interrupts(); // Leave critical section
}


void CApp::Loop()
{
  // 1ms loop delay
  delay(1);

  HandleNetwork();

  // Poll ethernet for commands
  PollEthernet();

  // Poll serial for commands
  PollSerial();

  m_pvBoiler.Loop();

  UpdateValues();

  HandleDisplay();
}
