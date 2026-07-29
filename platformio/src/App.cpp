#include "TermPrint.h"
#include "util.h"
#include "App.h"

CApp::CApp() : m_pvBoiler(m_network), m_commandHandler(m_pvBoiler, m_network)
{
  m_iLastZeroCrossTime = m_iLastEventTime = micros();
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
  if (iPulseWidth < ZERO_CROSS_EDGE_MIN_US || m_bGateBlanking)
  {
    return; // Filter noise
  }

  // Update last even time
  m_iLastEventTime = iNow;

  // Pulse is longer than ~65 ms?
  if (iPulseWidth > 65535)
  {
    m_iPeriodTime = 65535; // Cap to max uint16 value
    return;
  }

  if (iPulseWidth >= ZERO_CROSS_EDGE_MAX_US) // Long pulse?
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
      if (m_iTriacDelayUs != 0)
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
    m_bGateBlanking = true;
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
    m_bGateBlanking = false;
  }
}


void CApp::TerminalHandler(void)
{
  char rxChar = 0x00;

  if (TERM_SERIAL.available())
  {
    rxChar = TERM_SERIAL.read();
  }
  else if (m_network.IsConnected())
  {
    WiFiClient& socketServerClient = m_network.GetSocketServerClient();
    if (socketServerClient && socketServerClient.connected())
    {
      if (socketServerClient.available()) // Data available?
      {
        rxChar = socketServerClient.read(); // Read data from socket
      }
    }
  }

  // No data?
  if (rxChar == 0x00)
    return;

  if (m_pActiveTermRxData->state == RX_STATE_READY)
    return; // All buffers full

  if (m_pActiveTermRxData->state == RX_STATE_DONE)
  {
    m_pActiveTermRxData->buf_count = 0;
    m_pActiveTermRxData->state = RX_STATE_FILLING;
  }

  if (rxChar == '!') // Repeat the previous command but don't execute it yet
  {
    if (m_pActiveTermRxData->buf_count == 0)
    {
      // Swap pointers
      volatile rx_data_t* p_temp = m_pActiveTermRxData;
      m_pActiveTermRxData = m_pInactiveTermRxData;
      m_pInactiveTermRxData = p_temp;
      m_pActiveTermRxData->state = RX_STATE_FILLING;
      m_pInactiveTermRxData->state = RX_STATE_DONE;

      if (m_commandHandler.GetLocalEchoEnabled())
      {
        TERM_SERIAL.print(const_cast<char*>(m_pActiveTermRxData->buf));
      }
    }
  }
  else if (rxChar == CH_DELETE || rxChar == CH_BACKSPACE) // Backspace OR delete (sometimes mixed up by terminal programs)
  {
    if (m_pActiveTermRxData->buf_count != 0)
    {
      if (m_commandHandler.GetLocalEchoEnabled())
      {
        // NOTE: Only for serial connection

        // Backspace
        TERM_SERIAL.write(CH_BACKSPACE);
        // Blank character
        TERM_SERIAL.write(' ');
        // And backspace again since the blank jumps forward
        TERM_SERIAL.write(CH_BACKSPACE);
      }

      m_pActiveTermRxData->buf_count--;
    }
  }
  else if (rxChar >= ' ' && rxChar <= '~') // Limit allowed characters
  {
    // Don't overflow + skip leading spaces:
    if (m_pActiveTermRxData->buf_count < (CMD_BUF_SIZE - 1) && !(m_pActiveTermRxData->buf_count == 0 && rxChar == ' '))
    {
      m_pActiveTermRxData->buf[m_pActiveTermRxData->buf_count++] = rxChar;

      if (m_commandHandler.GetLocalEchoEnabled() && m_pActiveTermRxData->buf[0] != '[' /* Don't display special sequences that start with [ */)
      {
        TERM_SERIAL.write(rxChar);
      }
    }
  }

  if (rxChar == CH_CR || rxChar == CH_LF) // Continue until the command is "entered"
  {
    // Linefeed for local echo
    if (m_commandHandler.GetLocalEchoEnabled())
    {
      TERM_SERIAL.println("");
    }

    // Only check non-empty commands
    if (m_pActiveTermRxData->buf_count != 0)
    {
      m_pActiveTermRxData->state = RX_STATE_READY;
      m_pActiveTermRxData->buf[m_pActiveTermRxData->buf_count] = '\0';

      if (m_pInactiveTermRxData->state != RX_STATE_READY)
      {
        // Swap pointers
        volatile rx_data_t* p_temp = m_pActiveTermRxData;
        m_pActiveTermRxData = m_pInactiveTermRxData;
        m_pInactiveTermRxData = p_temp;
        m_pActiveTermRxData->state = RX_STATE_DONE;
      }
    }
  }

  // Handling of special commands
  if (m_pActiveTermRxData->buf_count == 4 && memcmp(const_cast<char*>(m_pActiveTermRxData->buf), "[13~", 4) == 0)  // F3, repeat command without <enter>
  {
    if (m_pInactiveTermRxData->state == RX_STATE_DONE && m_pInactiveTermRxData->buf_count != 0)
    {
      if (m_commandHandler.GetLocalEchoEnabled())
      {
        TERM_SERIAL.println(const_cast<char*>(m_pInactiveTermRxData->buf));
      }

      m_pInactiveTermRxData->state = RX_STATE_READY;
    }

    m_pActiveTermRxData->buf_count = 0;
    m_pActiveTermRxData->state = RX_STATE_DONE;
  }
  else if (m_pActiveTermRxData->buf[0] == '[' && m_pActiveTermRxData->buf_count > 4)
  {
    m_pActiveTermRxData->buf_count = 0;
    m_pActiveTermRxData->state = RX_STATE_DONE;
  }
}


bool CApp::CommandHandler()
{
  // NOTE: Inactive buffer always contains the "next" command to be processed
  if (m_pInactiveTermRxData->state != RX_STATE_READY)
    return false;

//  noInterrupts(); // Enter critical section

  char strCommand[CMD_BUF_SIZE];
  strcpy(strCommand, (char*) m_pInactiveTermRxData->buf);
  m_pInactiveTermRxData->state = RX_STATE_DONE;

  if (m_pActiveTermRxData->state == RX_STATE_READY)
  {
    // Swap pointers (so the command in the other buffer gets processed the next round
    volatile rx_data_t* p_temp = m_pActiveTermRxData;
    m_pActiveTermRxData = m_pInactiveTermRxData;
    m_pInactiveTermRxData = p_temp;
  }

//  interrupts(); // Leave critical section

  // Parse command and get result
  const result_code_t resultCode = m_commandHandler.ProcessCommand(strCommand);

  // Finally output result-code string (OK or ERROR:)
  if (resultCode.code != ERR_CODE_OK_NULL)
  {
    char strResult[RESULT_BUF_SIZE];
    get_error_string(resultCode, strResult, false);
    CTermPrint::print(strResult);
  }

  return true;
}


void CApp::HandleNetwork()
{
  m_network.Loop();

  if (m_network.HandleMqttClient())
  {
    // MQTT (re)connection: publish config & values
    m_pvBoiler.MqttPublishConfig();
    m_pvBoiler.MqttPublishValues(true);
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
    char strValue[22]; // Maximum amount of characters on a single line with this font is 21

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

                 snprintf(strValue, sizeof(strValue), "%u.%u.%u.%u", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
                 m_display.WriteDisplayStr(strValue, 1, false);

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
                 if (m_pvBoiler.GetError())
                 {
                   m_display.WriteDisplayStr("Power error", 0, true);
                 }
                 else
                 {
                   snprintf(strValue, sizeof(strValue), "%uW", m_pvBoiler.GetCurrentPower());
                   m_display.WriteDisplayStr(strValue, 0, true);
                 }

                 const uint8_t iPercent = m_pvBoiler.GetCurrentPercentage();

                 snprintf(strValue, sizeof(strValue), "%u%%", iPercent);
                 m_display.WriteDisplayStr(strValue, 1, false);

                 // Chars are not monospace so need to compensate for smaller spaces with the logic below
                 strcpy(strValue, "[");
                 for (uint8_t iCount = 0; iCount < 100;)
                 {
                   if (iCount < iPercent)
                   {
                     strcat(strValue, "=");
                     iCount += 10;
                   }
                   else
                   {
                     strcat(strValue, " ");
                     iCount += 5; // Spaces are smaller
                   }
                 }
                 strcat(strValue, "]");
                 m_display.WriteDisplayStr(strValue, 2, false);
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


// Update values between CPvBoiler and triac ISR
void CApp::UpdateValues()
{
  noInterrupts(); // Enter critical section

  const uint32_t iTimeSinceLastZeroCross = micros() - m_iLastZeroCrossTime;

  // Get current phase correction & zero cross time value from ISR
  uint16_t iZeroCrossWindow = m_iZeroCrossWindow;
  uint16_t iPeriodTime = m_iPeriodTime;

  interrupts(); // Leave critical section

  // Check for timeout: No zero cross within 1 second?
  if (iTimeSinceLastZeroCross > 1000 * 1000)
  {
    // Zero cross ISR timeout: reset values to default
    iZeroCrossWindow = ZERO_CROSS_WINDOW_DEFAULT;
    iPeriodTime = 65535;
  }

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

  // Poll serial & ethernet for commands
  TerminalHandler();

  CommandHandler();

  m_pvBoiler.Loop();

  UpdateValues();

  HandleDisplay();
}
