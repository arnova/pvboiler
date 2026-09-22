#include "Terminal.h"
#include "util.h"
#include "App.h"

CApp::CApp() : m_pvBoiler(m_network), m_commandHandler(m_pvBoiler, m_network), m_terminal(m_network),
               m_period(NET_PERIOD_MIN_US, NET_PERIOD_MAX_US, MAX_CONSECUTIVE_OUTLIERS),
               m_zeroCrossWindow(ZERO_CROSS_WINDOW_MIN_US, ZERO_CROSS_WINDOW_MAX_US, MAX_CONSECUTIVE_OUTLIERS)
{
  m_iLastPeriodStartTime = m_iLastEventTime = micros();
}


void CApp::Init()
{
  m_display.Init();
  m_display.WriteDisplayStr(DEVICE_NAME);
  m_display.WriteDisplayStr("v" MY_VERSION, 1, false);
  m_display.WriteDisplayStr("(C) Arnova", 2, false);

  m_network.Init();

  m_pvBoiler.Reset();
}


void IRAM_ATTR CApp::ZeroCrossHandler()
{
  const uint32_t iNow = micros();
  const uint32_t iPulseWidth = iNow - m_iLastEventTime;

  if (iPulseWidth < ZERO_CROSS_WINDOW_MIN_US || m_bGateBlanking)
  {
    return; // Filter noise
  }

  m_iLastEventTime = iNow;

  // Short pulse: zero cross window measurement
  if (iPulseWidth <= ZERO_CROSS_WINDOW_MAX_US)
  {
    const CTrackedValue::result_t result = m_zeroCrossWindow.Update(iPulseWidth);
    if (result == CTrackedValue::RESULT_OUTLIER || result == CTrackedValue::RESULT_RESET)
    {
      m_iLastEventTime = iNow - iPulseWidth; // Ignore this edge completely (restore previous event time)
    }

    return;
  }

  // Long pulse: period measurement
  const uint32_t iNewPeriodTime = iNow - m_iLastPeriodStartTime;

  if (iNewPeriodTime < NET_PERIOD_MIN_US)
  {
    // Implausibly short period, e.g. the end edge of a zero-cross pulse that was
    // serviced late (WiFi latency) and therefore looks like a long pulse.
    // Ignore this edge completely: keep the period baseline intact.
    m_iLastEventTime = iNow - iPulseWidth; // Restore previous event time
    return;
  }

  if (iNewPeriodTime >= NET_PERIOD_INVALID)
  {
    // Gap: far too long to be a real period (dropout, missed pulses, boot)
    m_period.Reset(iNewPeriodTime);
    m_iLastPeriodStartTime = iNow;
    return;
  }

  switch (m_period.Update(iNewPeriodTime))
  {
    case CTrackedValue::RESULT_ACCEPTED:
    {
      m_iLastPeriodStartTime = iNow;
    }
    break;

    case CTrackedValue::RESULT_OUTLIER:
    {
      // Flywheel: advance by the number of whole periods passed (handles missed edges)
      const uint32_t iPeriod = m_period.Get();
      m_iLastPeriodStartTime += ((iNow - m_iLastPeriodStartTime + iPeriod / 2) / iPeriod) * iPeriod;
    }
    break;

    default: // No valid average: resync and do not fire triac
    {
      m_iLastPeriodStartTime = iNow;
    }
    return;
  }

  digitalWrite(TRIAC_OUTPUT, LOW); // Off

  if (m_dimStyle == CPvBoiler::DIM_STYLE_PHASE_ANGLE)
  {
    // NOTE: m_iTriacDelayUs is 0 when for whatever reason triac should not be turned on
    if (m_iTriacDelayUs != 0)
    {
      ScheduleTriac(iNow);
    }
  }
  else if (m_dimStyle == CPvBoiler::DIM_STYLE_SSR && m_iCurrentPercentage != 0)
  {
    m_iSSRPeriodCounter++;
    if ((m_iSSRPeriodCounter * 100) / m_iSSRPeriodCount <= m_iCurrentPercentage)
    {
      ScheduleTriac(iNow);
    }

    if (m_iSSRPeriodCounter >= m_iSSRPeriodCount)
    {
      m_iSSRPeriodCounter = 0;
    }
  }
}


void IRAM_ATTR CApp::ScheduleTriac(const uint32_t iNow)
{
  const int32_t iRemainingDelay = static_cast<int32_t>(m_iLastPeriodStartTime + m_iTriacDelayUs - iNow);

  if (iRemainingDelay <= 0)
  {
    // Conservative: skip firing this half-cycle rather than
    // firing at an unintended (near-zero) delay
    m_bTriacOn = false;
    return;
  }

  m_bTriacOn = true;
#ifdef ESP8266
  timer1_write(iRemainingDelay * ESP8266_TICKS_PER_US);
#else
  timerWrite(m_hTriacTimer, 0);
  timerAlarmWrite(m_hTriacTimer, iRemainingDelay, false); // one-shot, tick = 1µs
  timerAlarmEnable(m_hTriacTimer);
#endif
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
    timerAlarmWrite(m_hTriacTimer, GATE_PULSE_WIDTH, false);
    timerAlarmEnable(m_hTriacTimer);
#endif
  }
  else
  {
    digitalWrite(TRIAC_OUTPUT, LOW); // Off
    m_bGateBlanking = false;
  }
}


bool CApp::CommandHandler()
{
  char* strTermCommand = m_terminal.GetCommand();
  if (strTermCommand == nullptr)
    return false;

//  noInterrupts(); // Enter critical section

  // Copy command since it *may* be modified by ProcessCommand
  char strCommand[CMD_BUF_SIZE];
  strcpy(strCommand, strTermCommand);

//  interrupts(); // Leave critical section

  // Parse command and get result
  const result_code_t resultCode = m_commandHandler.ProcessCommand(strCommand);

  // Finally output result-code string (OK or ERROR:)
  if (resultCode.code != ERR_CODE_OK_NULL)
  {
    char strResult[RESULT_BUF_SIZE];
    get_error_string(resultCode, strResult, false);
    CTerminal::print(strResult);
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
    char strLine[22]; // Maximum amount of characters on a single line with this font is 21

    switch (m_displayCount)
    {
      case 0:
      {
        m_display.WriteDisplayStr(DEVICE_NAME, 0, true);
        m_display.WriteDisplayStr("v" MY_VERSION, 1, false);
        m_display.WriteDisplayStr("(C) Arnova", 2, false);
      } 
      break;

      case 1:
      {
        m_display.WriteDisplayStr(m_network.GetHostName(), 0, true);

        snprintf(strLine, sizeof(strLine), "%u.%u.%u.%u", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
        m_display.WriteDisplayStr(strLine, 1, false);

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
          m_display.WriteDisplayStr(m_network.GetWifiSsid(), 2, false);
        }
      }
      break;

      case 2:
      {
        if (!m_pvBoiler.GetPowerGood())
        {
          m_display.WriteDisplayStr("Power error", 0, true);
        }
        else
        {
          snprintf(strLine, sizeof(strLine), "%uW", m_pvBoiler.GetCurrentPower());
          m_display.WriteDisplayStr(strLine, 0, true);
        }

        const uint8_t iPercent = m_pvBoiler.GetCurrentPercentage();

        switch (m_pvBoiler.GetMode())
        {
          case CPvBoiler::MODE_BOOST:
          {
            strcpy(strLine, "Boost - 100%");
          }
          break;

          case CPvBoiler::MODE_BUDGET:
          {
            snprintf(strLine, sizeof(strLine), "Budget - %u%%", iPercent);
          }
          break;

          case CPvBoiler::MODE_PERCENT:
          {
            snprintf(strLine, sizeof(strLine), "%u%%", iPercent);
          }
          break;

          case CPvBoiler::MODE_OFF:
          {
            strcpy(strLine, "Off - 0%");
          }
          break;
        }

        m_display.WriteDisplayStr(strLine, 1, false);

        // Chars are not monospace so need to compensate for smaller spaces with the logic below
        strcpy(strLine, "[");
        for (uint8_t iCount = 0; iCount < 100;)
        {
          if (iCount < iPercent)
          {
            strcat(strLine, "=");
            iCount += 10;
          }
          else
          {
            strcat(strLine, " ");
            iCount += 5; // Spaces are smaller
          }
        }
        strcat(strLine, "]");
        m_display.WriteDisplayStr(strLine, 2, false);
      }
      break;

      case 3:
      {
        const float fTemperature = m_pvBoiler.GetBoilerTemperature();
        if (fTemperature < 0.0f)
        {
          m_display.WriteDisplayStr("Unknown temperature", 1, true);
        }
        else
        {
          snprintf(strLine, sizeof(strLine), "%.1fC", fTemperature);
          m_display.WriteDisplayStr(strLine, 1, true);
        }

        // Chars are not monospace so need to compensate for smaller spaces with the logic below
        strcpy(strLine, "[");
        for (uint8_t iCount = 0; iCount < 100;)
        {
          if (iCount < fTemperature) // Range 0C - 100C
          {
            strcat(strLine, "=");
            iCount += 10;
          }
          else
          {
            strcat(strLine, " ");
            iCount += 5; // Spaces are smaller
          }
        }
        strcat(strLine, "]");
        m_display.WriteDisplayStr(strLine, 2, false);
      }
      break;

      case 4:
      {
        m_display.WriteDisplayStr("", 0, true); // Empty screen to preven burnin
      }
      break;
    }

    if (++m_displayCount > 4)
    {
      m_displayCount = 0;
    }
  }
}


// Update values between CPvBoiler and triac ISR
void CApp::UpdateValues()
{
  noInterrupts(); // Enter critical section

  // Get current period & zero cross window from ISR
  const bool bPeriodValid = m_period.IsValid();
  const uint32_t iPeriodTime = m_period.Get();
  const bool bZeroCrossWindowValid = m_zeroCrossWindow.IsValid();
  const uint32_t iZeroCrossWindow = m_zeroCrossWindow.Get();

  interrupts(); // Leave critical section

  m_pvBoiler.SetPowerGood(bPeriodValid && bZeroCrossWindowValid);

  // Get updated values for ISR
  const uint8_t iCurrentPercentage = m_pvBoiler.GetCurrentPercentage();
  const uint8_t iSSRPeriodCount = m_pvBoiler.GetSsrPeriodCount();
  const CPvBoiler::dim_style_t dimStyle = m_pvBoiler.GetDimStyle();

  const uint16_t iTriacDelayUs = m_pvBoiler.CalculateTriacPhaseDelay(iPeriodTime, iZeroCrossWindow);

  noInterrupts(); // Enter critical section

  m_iCurrentPercentage = iCurrentPercentage;
  m_iSSRPeriodCount = iSSRPeriodCount;
  m_dimStyle = dimStyle;

  // Only update value when no errors (else fallback to previous value)
  if (m_pvBoiler.GetPowerGood())
  {
    m_iTriacDelayUs = iTriacDelayUs;
  }

  interrupts(); // Leave critical section
}


void CApp::Loop()
{
  // 1ms loop delay
  delay(1);

  HandleNetwork();

  // Poll serial & ethernet for commands
  m_terminal.Process();

  CommandHandler();

  m_pvBoiler.Loop();

  UpdateValues();

  HandleDisplay();
}
