#include "Terminal.h"
#include "util.h"
#include "App.h"

#ifdef ESP8266
// Timer1 at DIV1 (80 MHz clock) -> 80 ticks per µs on esp8266
// Maximum ~104 ms at this prescaler; no need for DIV256 in our range.
#define ESP8266_TICKS_PER_US                    80
#else // ESP32
#include "driver/mcpwm.h"
#include "driver/periph_ctrl.h"
#include "hal/mcpwm_ll.h"

// MCPWM (unit 0) does all triac timing in hardware, so WiFi interrupt latency can't affect it:
// - Capture channel 0 timestamps the zero cross edges
// - Capture channel 1 is software triggered to sample "now" on the capture timer
// - Timer 0 runs one-shot and drives generator A (triac gate) through compare A (pulse start) and B (pulse end)
#define MCPWM_CAP_ZERO_CROSS                    0
#define MCPWM_CAP_NOW                           1
#define MCPWM_GATE_TIMER                        0
#define MCPWM_GATE_OPERATOR                     0
#define MCPWM_GATE_GENERATOR                    0
#define MCPWM_CMP_GATE_START                    0
#define MCPWM_CMP_GATE_END                      1

// Capture timer runs at the APB clock (80 MHz)
#define MCPWM_CAP_TICKS_PER_US                  80

// Gate timer: 160 MHz / 16 / 10 = 1 MHz -> 1 µs per tick. Its period is 16 bit, so maximum delay is ~65 ms
#define MCPWM_GROUP_PRESCALE                    16
#define MCPWM_TIMER_PRESCALE                    10
#define MCPWM_COMPARE_NEVER                     0xFFFF // Beyond any timer peak we use, so never matches

static_assert(NET_PERIOD_MAX_US + ZERO_CROSS_WINDOW_MAX_US + GATE_PULSE_WIDTH + 2 < MCPWM_COMPARE_NEVER, "Triac delay doesn't fit MCPWM timer");
#endif


// Microseconds clock that's safe to use in an IRAM ISR (on ESP32 micros() isn't, unless CONFIG_ARDUINO_ISR_IRAM is set)
static inline uint32_t IRAM_ATTR GetMicros()
{
#ifdef ESP8266
  return micros();
#else
  return static_cast<uint32_t>(esp_timer_get_time());
#endif
}

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


// Called with the time of the zero cross edge. Note: On ESP32 this is a hardware timestamp, so it's not affected by ISR latency
void IRAM_ATTR CApp::ZeroCrossHandler(const uint32_t iNow)
{
  const uint32_t iPulseWidth = iNow - m_iLastEventTime;

  if (iPulseWidth < ZERO_CROSS_WINDOW_MIN_US || IsGateBlanking(iNow))
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

  // Real period edge seen: cancel any (pending) backup firing, this edge takes over
  m_bTriacBackupPending = false;
  DisarmTriacTimer();

  if (iNewPeriodTime >= NET_PERIOD_INVALID)
  {
    // Gap: far too long to be a real period (dropout, missed pulses, boot)
    m_period.Reset(iNewPeriodTime);
    m_iLastPeriodStartTime = iNow;
    return;
  }

  uint32_t iPeriodsPassed = 1;
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
      iPeriodsPassed = (iNow - m_iLastPeriodStartTime + iPeriod / 2) / iPeriod;
      m_iLastPeriodStartTime += iPeriodsPassed * iPeriod;
    }
    break;

    default: // No valid average: resync and do not fire triac
    {
      m_iLastPeriodStartTime = iNow;
    }
    return;
  }

#ifdef ESP8266
  digitalWrite(TRIAC_OUTPUT, LOW); // Off. Note: On ESP32 DisarmTriacTimer() already did this
#endif

  // SSR: Always switch full cycles (both half periods) to prevent a DC component. The decision is made on
  // the first half period of a cycle and held for the second one. Counter also advances for missed edges
  // so it stays in sync with the mains polarity
  m_iSSRPeriodCounter = (m_iSSRPeriodCounter + iPeriodsPassed) % (m_iSSRPeriodCount * 2);
  const bool bFirstHalf = (m_iSSRPeriodCounter & 1) == 0;
  if (bFirstHalf)
  {
    m_bSSRCycleOn = (m_iSSRPeriodCounter / 2) < m_iSSROnCycles;
  }
  else if (iPeriodsPassed > 1)
  {
    m_bSSRCycleOn = false; // First half period was missed: never complete a cycle we didn't start
  }

  const bool bPhaseAngle = (m_dimStyle == CPvBoiler::DIM_STYLE_PHASE_ANGLE);
  const bool bFire = bPhaseAngle || (m_dimStyle == CPvBoiler::DIM_STYLE_SSR && m_bSSRCycleOn);
  const bool bFireNext = bPhaseAngle || bFirstHalf; // SSR: Next cycle may be off

  // NOTE: m_iTriacDelayUs is 0 when for whatever reason triac should not be turned on
  if (bFire && m_iTriacDelayUs != 0)
  {
    const uint32_t iFireTime = m_iLastPeriodStartTime + m_iTriacDelayUs;

    // Also prepare a backup firing for the next half period (predicted with the flywheel period).
    // This prevents a DC component when the next zero cross is missed
    if (ScheduleTriac(iFireTime) && bFireNext)
    {
      m_iTriacBackupTime = iFireTime + m_period.Get();
      m_bTriacBackupPending = true;
    }
  }
}


bool IRAM_ATTR CApp::ScheduleTriac(const uint32_t iFireTime)
{
  const int32_t iRemainingDelay = static_cast<int32_t>(iFireTime - GetMicros());

  if (iRemainingDelay <= 0)
  {
    // Conservative: skip firing this half-cycle rather than
    // firing at an unintended (near-zero) delay
    return false;
  }

  m_bTriacOn = true;
  m_iGateTime = iFireTime;
  ArmTriacTimer(iRemainingDelay);

  return true;
}


void IRAM_ATTR CApp::ArmTriacTimer(const uint32_t iDelayUs)
{
#ifdef ESP8266
  timer1_write(iDelayUs * ESP8266_TICKS_PER_US);
#else // ESP32: The complete gate pulse is generated in hardware. Note: (Re)arming ends a pulse in progress (if any)
  mcpwm_dev_t* const mcpwm = &MCPWM0;

  mcpwm_ll_gen_trigger_noncontinue_force_action(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_GATE_GENERATOR); // Force low
  mcpwm_ll_operator_set_compare_value(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_CMP_GATE_START, iDelayUs);
  mcpwm_ll_operator_set_compare_value(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_CMP_GATE_END, iDelayUs + GATE_PULSE_WIDTH);
  mcpwm_ll_timer_set_peak(mcpwm, MCPWM_GATE_TIMER, iDelayUs + GATE_PULSE_WIDTH + 2, false);
  mcpwm_ll_timer_trigger_soft_sync(mcpwm, MCPWM_GATE_TIMER); // Restart counting at 0
  mcpwm_ll_intr_clear_timer_tep_status(mcpwm, 1 << MCPWM_GATE_TIMER); // Drop a pending end event of a previous pulse
  mcpwm_ll_timer_set_execute_command(mcpwm, MCPWM_GATE_TIMER, MCPWM_TIMER_START_STOP_AT_PEAK); // One-shot
#endif
}


// Cancel a scheduled triac firing
void IRAM_ATTR CApp::DisarmTriacTimer()
{
  m_bTriacOn = false; // ESP8266: TriacGateHandler() won't fire

#ifdef ESP32
  // Make the compares unreachable and end a pulse in progress (if any)
  mcpwm_dev_t* const mcpwm = &MCPWM0;
  mcpwm_ll_operator_set_compare_value(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_CMP_GATE_START, MCPWM_COMPARE_NEVER);
  mcpwm_ll_operator_set_compare_value(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_CMP_GATE_END, MCPWM_COMPARE_NEVER);
  mcpwm_ll_gen_trigger_noncontinue_force_action(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_GATE_GENERATOR); // Force low
#endif
}


// Ignore zero cross edges during the gate pulse: Firing the triac may cause a glitch on the zero cross input
bool IRAM_ATTR CApp::IsGateBlanking(const uint32_t iNow) const
{
#ifdef ESP8266
  return m_bGateBlanking;
#else // ESP32: Gate pulse is generated in hardware so check its time window
  return m_bTriacOn && (iNow - m_iGateTime) <= GATE_PULSE_WIDTH;
#endif
}


void IRAM_ATTR CApp::TriacGateHandler()
{
#ifdef ESP8266
  if (m_bTriacOn)
  {
    m_bGateBlanking = true;
    digitalWrite(TRIAC_OUTPUT, HIGH); // On

    m_bTriacOn = false;
    ArmTriacTimer(GATE_PULSE_WIDTH);
    return;
  }

  digitalWrite(TRIAC_OUTPUT, LOW); // Off
  m_bGateBlanking = false;
#else // ESP32: Called at the end of the (hardware generated) gate pulse
  m_bTriacOn = false;
#endif

  // End of gate pulse: arm backup firing for the next half period. It's cancelled by its zero cross edge
  // (which then schedules the firing itself), so it only fires when that edge is missed
  if (m_bTriacBackupPending)
  {
    m_bTriacBackupPending = false;
    ScheduleTriac(m_iTriacBackupTime);
  }
}


#ifdef ESP32
void CApp::InitMcpwm(void (*isr)(void*))
{
  mcpwm_dev_t* const mcpwm = &MCPWM0;

  periph_module_enable(PERIPH_PWM0_MODULE);
  mcpwm_ll_intr_disable_all(mcpwm);
  mcpwm_ll_group_set_clock_prescale(mcpwm, MCPWM_GROUP_PRESCALE);

  // Gate timer: Counts up, one-shot (started by ArmTriacTimer()), software sync only to restart it at 0
  mcpwm_ll_timer_set_clock_prescale(mcpwm, MCPWM_GATE_TIMER, MCPWM_TIMER_PRESCALE);
  mcpwm_ll_timer_set_count_mode(mcpwm, MCPWM_GATE_TIMER, MCPWM_TIMER_COUNT_MODE_UP);
  mcpwm_ll_timer_update_period_at_once(mcpwm, MCPWM_GATE_TIMER);
  mcpwm_ll_timer_set_peak(mcpwm, MCPWM_GATE_TIMER, MCPWM_COMPARE_NEVER, false);
  mcpwm_ll_timer_set_soft_synchro(mcpwm, MCPWM_GATE_TIMER);
  mcpwm_ll_timer_set_sync_phase_value(mcpwm, MCPWM_GATE_TIMER, 0);
  mcpwm_ll_timer_set_sync_phase_direction(mcpwm, MCPWM_GATE_TIMER, MCPWM_TIMER_DIRECTION_UP);
  mcpwm_ll_timer_enable_sync_input(mcpwm, MCPWM_GATE_TIMER, true);

  // Gate generator: High from compare A until compare B. Also low at zero/peak, just to be safe
  mcpwm_ll_operator_select_timer(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_GATE_TIMER);
  mcpwm_ll_operator_update_compare_at_once(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_CMP_GATE_START);
  mcpwm_ll_operator_update_compare_at_once(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_CMP_GATE_END);
  mcpwm_ll_operator_set_compare_value(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_CMP_GATE_START, MCPWM_COMPARE_NEVER);
  mcpwm_ll_operator_set_compare_value(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_CMP_GATE_END, MCPWM_COMPARE_NEVER);
  mcpwm_ll_generator_set_action_on_compare_event(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_GATE_GENERATOR,
                                                 MCPWM_TIMER_DIRECTION_UP, MCPWM_CMP_GATE_START, MCPWM_GEN_ACTION_HIGH);
  mcpwm_ll_generator_set_action_on_compare_event(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_GATE_GENERATOR,
                                                 MCPWM_TIMER_DIRECTION_UP, MCPWM_CMP_GATE_END, MCPWM_GEN_ACTION_LOW);
  mcpwm_ll_generator_set_action_on_timer_event(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_GATE_GENERATOR,
                                               MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_ZERO, MCPWM_GEN_ACTION_LOW);
  mcpwm_ll_generator_set_action_on_timer_event(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_GATE_GENERATOR,
                                               MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_PEAK, MCPWM_GEN_ACTION_LOW);
  mcpwm_ll_gen_set_noncontinue_force_level(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_GATE_GENERATOR, 0); // Software force = low
  mcpwm_ll_gen_trigger_noncontinue_force_action(mcpwm, MCPWM_GATE_OPERATOR, MCPWM_GATE_GENERATOR);

  // Capture: Zero cross on both edges, "now" by software trigger only
  mcpwm_ll_capture_enable_timer(mcpwm, true);
  mcpwm_ll_capture_set_prescale(mcpwm, MCPWM_CAP_ZERO_CROSS, 1);
  mcpwm_ll_capture_enable_posedge(mcpwm, MCPWM_CAP_ZERO_CROSS, true);
  mcpwm_ll_capture_enable_negedge(mcpwm, MCPWM_CAP_ZERO_CROSS, true);
  mcpwm_ll_capture_enable_channel(mcpwm, MCPWM_CAP_ZERO_CROSS, true);
  mcpwm_ll_capture_enable_posedge(mcpwm, MCPWM_CAP_NOW, false);
  mcpwm_ll_capture_enable_negedge(mcpwm, MCPWM_CAP_NOW, false);
  mcpwm_ll_capture_enable_channel(mcpwm, MCPWM_CAP_NOW, true);

  // Route pins. Note: Zero cross pin keeps its pull-up (set with pinMode())
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, TRIAC_OUTPUT);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, ZERO_CROSS_INPUT);

  // IRAM interrupt, so it also runs while flash is busy (e.g. during WiFi/NVS writes)
  mcpwm_ll_intr_clear_status(mcpwm, UINT32_MAX);
  mcpwm_ll_intr_enable_timer_tep(mcpwm, MCPWM_GATE_TIMER, true);
  mcpwm_ll_intr_enable_capture(mcpwm, MCPWM_CAP_ZERO_CROSS, true);
  mcpwm_isr_register(MCPWM_UNIT_0, isr, nullptr, ESP_INTR_FLAG_IRAM, nullptr);
}


void IRAM_ATTR CApp::McpwmHandler()
{
  mcpwm_dev_t* const mcpwm = &MCPWM0;

  const uint32_t iStatus = mcpwm_ll_intr_get_status(mcpwm);
  mcpwm_ll_intr_clear_status(mcpwm, iStatus);

  // End of gate pulse first: It may arm the backup firing, which the zero cross edge then must be able to cancel
  if (iStatus & (1 << (6 + MCPWM_GATE_TIMER))) // TEP
  {
    TriacGateHandler();
  }

  if (iStatus & (1 << (27 + MCPWM_CAP_ZERO_CROSS)))
  {
    // Convert the edge's capture timestamp to our microseconds clock, using a "now" sample of both clocks
    const uint32_t iEdgeTicks = mcpwm_ll_capture_get_value(mcpwm, MCPWM_CAP_ZERO_CROSS);
    mcpwm_ll_trigger_soft_capture(mcpwm, MCPWM_CAP_NOW);
    const uint32_t iNow = GetMicros();
    const uint32_t iNowTicks = mcpwm_ll_capture_get_value(mcpwm, MCPWM_CAP_NOW);

    ZeroCrossHandler(iNow - (iNowTicks - iEdgeTicks) / MCPWM_CAP_TICKS_PER_US);
  }
}
#endif


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
    // MQTT (re)connection: subscribe and publish discovery config (values follow after the republish delay)
    m_pvBoiler.MqttOnConnect();
    MqttRequestRepublish();
  }

  // Delay republishing: HA subscribes to state topics of (newly) discovered entities only after processing
  // their config, so it would miss our (non-retained) values
  if (m_bMqttRepublish && m_mqttRepublishTimer > MQTT_SETTLE_TIME_MS)
  {
    m_bMqttRepublish = false;
    m_pvBoiler.MqttRequestRepublish();
    m_network.MqttRequestRepublish();
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


// Format value (0-100) as bar string. Note: strLine must fit "[" + 20 spaces + "]" + NUL
const char* CApp::FormatBar(char* strLine, const float fValue)
{
  // Chars are not monospace so need to compensate for smaller spaces with the logic below
  strcpy(strLine, "[");
  for (uint8_t iCount = 0; iCount < 100;)
  {
    if (iCount < fValue)
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

  return strLine;
}


void CApp::HandleDisplay()
{
  if (m_displayTimer > 5000)
  {
    m_displayTimer = 0;
    char strLine[24]; // Must fit the bar string: "[" + 20 spaces + "]" + NUL

    switch (m_displayCount)
    {
      case 0:
      {
        m_display.WriteDisplayStr(m_network.GetHostName(), 0, true);

        m_display.WriteDisplayStr(CNetwork::FormatIp(WiFi.localIP(), strLine, sizeof(strLine)), 1, false);

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

      case 1:
      {
        const uint8_t iPercent = lroundf(m_pvBoiler.GetCurrentPercentage());

        m_display.WriteDisplayStr(CPvBoiler::m_modeNames[m_pvBoiler.GetMode()].strDisplay, 0, true);

        if (!m_pvBoiler.GetPowerGood())
        {
          m_display.WriteDisplayStr("Power error", 1, false);
        }
        else
        {
          snprintf(strLine, sizeof(strLine), "%uW - %u%%", m_pvBoiler.GetCurrentPower(), iPercent);
          m_display.WriteDisplayStr(strLine, 1, false);
        }

        m_display.WriteDisplayStr(FormatBar(strLine, iPercent), 2, false);
      }
      break;

      case 2:
      {
        m_display.WriteDisplayStr(CPvBoiler::m_stateNames[m_pvBoiler.GetState()].strDisplay, 0, true);

        const float fTemperature = m_pvBoiler.GetBoilerTemperature();
        if (fTemperature < 0.0f)
        {
          m_display.WriteDisplayStr("Unknown temperature", 1, false);
        }
        else
        {
          snprintf(strLine, sizeof(strLine), "%.1fC", fTemperature);
          m_display.WriteDisplayStr(strLine, 1, false);
        }

        m_display.WriteDisplayStr(FormatBar(strLine, fTemperature), 2, false); // Range 0C - 100C
      }
      break;

      case 3:
      {
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

  // Get current period & zero cross window from ISR
  const bool bPeriodValid = m_period.IsValid();
  const uint32_t iPeriodTime = m_period.Get();
  const bool bZeroCrossWindowValid = m_zeroCrossWindow.IsValid();
  const uint32_t iZeroCrossWindow = m_zeroCrossWindow.Get();

  interrupts(); // Leave critical section

  m_pvBoiler.SetPowerGood(bPeriodValid && bZeroCrossWindowValid);

  // Get updated values for ISR
  const uint8_t iSSRPeriodCount = m_pvBoiler.GetSsrPeriodCount();
  const CPvBoiler::dim_style_t dimStyle = m_pvBoiler.GetDimStyle();

  // Number of full cycles to switch on per SSR period. Note: Percentage is clamped to 0-100
  const uint8_t iSSROnCycles = (dimStyle == CPvBoiler::DIM_STYLE_SSR) ? lroundf(m_pvBoiler.GetCurrentPercentage() * iSSRPeriodCount / 100.0f) : 0;

  const uint16_t iTriacDelayUs = m_pvBoiler.CalculateTriacPhaseDelay(iPeriodTime, iZeroCrossWindow);

  noInterrupts(); // Enter critical section

  m_iSSROnCycles = iSSROnCycles;
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
