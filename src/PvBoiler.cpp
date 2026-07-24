#include "PvBoiler.h"
#include "util.h"

#include <EEPROM.h>

#define CONTROL_LOOP_TIME_MS                    200   // ms

CPvBoiler::CPvBoiler(CNetwork& network) : m_network(network)
{
}


void CPvBoiler::Loop()
{
  // Update uptime
  m_upTime.Update();

  // Run timed control loop
  if (m_loopTimer > CONTROL_LOOP_TIME_MS)
  {
    CheckWatchDog();
    Update();

    m_loopTimer = 0;
  }

  // Publish new MQTT values (if any) when timer expires (and connected)
  if (m_mqttPublishTimer > MQTT_UPDATE_TIME * 1000 && m_network.GetMqttClient().connected())
  {
    MqttPublishValues();

    m_mqttPublishTimer = 0;
  }
}


void CPvBoiler::Reset()
{
  m_iWatchdogCounter = 0;
  m_iWatchdogRecoveryCounter = 0;

  m_bCtrlEnable = true;
  m_bPublishCtrlOnOff = true;

  m_iPowerBudget = 0;
  m_bPublishPowerBudget = true;

  m_iPowerPercentage = 0;
  m_bPublishPowerPercentage = true;

  m_fCurrentPercentage = 0.0f;
  m_bPublishOutputPercentage = true;

  m_bError = false;

  m_fTriacAngleFactor = 0.0f;
  m_iTriacPhaseAngle = 0;
  m_iPeriodTime = 0;
  m_iZeroCrossWindow = ZERO_CROSS_WINDOW_DEFAULT;

  LoadSettings();
}


bool CPvBoiler::MqttPublishValues()
{
  if (m_bPublishCtrlOnOff)
  {
    m_bPublishCtrlOnOff = false;
    m_network.GetMqttClient().PublishMessage(MQTT_CONTROLLER_ON_OFF, m_bCtrlEnable ? "1" : "0");
  }

  if (m_bPublishPowerBudget)
  {
    m_bPublishPowerBudget = false;

    if (m_logicMode == LOGIC_MODE_BUDGET)
      m_network.GetMqttClient().PublishMessage(MQTT_SET_POWER_BUDGET, String(m_iPowerBudget));
  }

  if (m_bPublishPowerPercentage)
  {
    m_bPublishPowerPercentage = false;

    if (m_logicMode == LOGIC_MODE_PERCENT)
      m_network.GetMqttClient().PublishMessage(MQTT_SET_POWER_PERCENTAGE, String(m_iPowerPercentage));
  }

  if (m_bPublishOutputPercentage)
  {
    m_bPublishOutputPercentage = false;
    m_network.GetMqttClient().PublishMessage(MQTT_OUTPUT_PERCENTAGE, String(m_fCurrentPercentage));
    m_network.GetMqttClient().PublishMessage(MQTT_OUTPUT_POWER, String(GetCurrentPower()));
    m_network.GetMqttClient().PublishMessage(MQTT_PHASE_ANGLE, String(GetTriacPhaseAngle()));

    if (m_dimStyle == DIM_STYLE_PHASE_ANGLE)
    {
      m_network.GetMqttClient().PublishMessage(MQTT_PHASE_ANGLE_FACTOR, String(GetTriacAngleFactor(), 4));
    }
  }

  // FIXME: These are always updated
  m_network.GetMqttClient().PublishMessage(MQTT_TRIAC_ERROR, m_bError ? "1" : "0");
  m_network.GetMqttClient().PublishMessage(MQTT_NET_PERIOD, String(m_iPeriodTime));
  m_network.GetMqttClient().PublishMessage(MQTT_ZERO_CROSS_WINDOW, String(m_iZeroCrossWindow));

  // Publish uptime
  const CUptime::uptime_t upTime = GetUpTime();
  char strTemp[24];
  snprintf(strTemp, sizeof(strTemp), "%uy %ud %02u:%02u:%02u", upTime.iYears, upTime.iDays, upTime.iHours, upTime.iMinutes, upTime.iSeconds);
  m_network.GetMqttClient().PublishMessage(MQTT_UP_TIME, strTemp);

  if (m_bPublishSettings)
  {
    m_bPublishSettings = false;

    m_network.GetMqttClient().PublishMessage(MQTT_BOILER_POWER, String(m_iBoilerPowerRating));

    if (m_logicMode == LOGIC_MODE_BUDGET)
    {
      m_network.GetMqttClient().PublishMessage(MQTT_LOGIC_MODE, "Budget");
      m_network.GetMqttClient().PublishMessage(MQTT_BUDGET_MARGIN, String(m_iPowerBudgetMargin));
      m_network.GetMqttClient().PublishMessage(MQTT_ERROR_GAIN, String(m_fErrorGain, 3));
      m_network.GetMqttClient().PublishMessage(MQTT_ERROR_CLAMP, String(m_iErrorClamp));
    }
    else // Percentage
    {
      m_network.GetMqttClient().PublishMessage(MQTT_LOGIC_MODE, "Percentage");
    }

    if (m_dimStyle == DIM_STYLE_SSR)
    {
      m_network.GetMqttClient().PublishMessage(MQTT_DIM_STYLE, "SSR");
      m_network.GetMqttClient().PublishMessage(MQTT_SSR_PERIOD_COUNT, String(m_iSsrPeriodCount));
    }
    else
    {
      m_network.GetMqttClient().PublishMessage(MQTT_DIM_STYLE, "Phase-angle");
    }
  }

  return true;
}


void CPvBoiler::MqttPublishConfig()
{
  // Publish MQTT config for eg. HA discovery and subscribe to control topics
  m_network.GetMqttClient().PublishSwitchConfig(MQTT_CONTROLLER_ON_OFF);

  m_network.GetMqttClient().PublishBinarySensorConfig(MQTT_TRIAC_ERROR);

  if (m_logicMode == CPvBoiler::LOGIC_MODE_BUDGET)
  {
    m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_BUDGET, "1", "-100000", "100000");
    m_network.GetMqttClient().PublishSensorConfig(MQTT_ERROR_GAIN);
    m_network.GetMqttClient().PublishSensorConfig(MQTT_ERROR_CLAMP, "%", "power_factor");
    m_network.GetMqttClient().PublishSensorConfig(MQTT_BUDGET_MARGIN, "W", "power");

    m_network.GetMqttClient().UnpublishNumberConfig(MQTT_SET_POWER_PERCENTAGE);
  }
  else // Percentage
  {
    m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_PERCENTAGE, "1", "0", "100", false);

    m_network.GetMqttClient().UnpublishNumberConfig(MQTT_SET_POWER_BUDGET);
    m_network.GetMqttClient().UnpublishNumberConfig(MQTT_ERROR_GAIN);
    m_network.GetMqttClient().UnpublishNumberConfig(MQTT_ERROR_CLAMP);
    m_network.GetMqttClient().UnpublishNumberConfig(MQTT_BUDGET_MARGIN);
  }

  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_POWER, "W", "power");
  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_PERCENTAGE, "%", "power_factor");

  m_network.GetMqttClient().PublishSensorConfig(MQTT_LOGIC_MODE);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_BOILER_POWER, "W", "power");

  m_network.GetMqttClient().PublishSensorConfig(MQTT_DIM_STYLE);

  if (m_dimStyle == DIM_STYLE_SSR)
  {
    m_network.GetMqttClient().PublishSensorConfig(MQTT_SSR_PERIOD_COUNT);
  }
  else
  {
    m_network.GetMqttClient().UnpublishSensorConfig(MQTT_SSR_PERIOD_COUNT);
  }

  // Diagnostic
  m_network.GetMqttClient().PublishSensorConfig(MQTT_WIFI_SSID, "", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_IP_ADDRESS, "", "", "", true);
//  m_network.GetMqttClient().PublishSensorConfig(MQTT_IP_NETMASK, "", "", true);

//  m_network.GetMqttClient().PublishSensorConfig(MQTT_PHASE_ANGLE, "ms", "duration", "measurement", true);  
  m_network.GetMqttClient().PublishSensorConfig(MQTT_PHASE_ANGLE, "us", "", "", true);

//  m_network.GetMqttClient().PublishSensorConfig(MQTT_NET_PERIOD, "ms", "duration", "measurement", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_NET_PERIOD, "us", "", "", true);

//  m_network.GetMqttClient().PublishSensorConfig(MQTT_ZERO_CROSS_WINDOW, "ms", "duration", "measurement", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_ZERO_CROSS_WINDOW, "us", "", "", true);

  if (m_dimStyle == DIM_STYLE_PHASE_ANGLE)
  {
    m_network.GetMqttClient().PublishSensorConfig(MQTT_PHASE_ANGLE_FACTOR, "", "", "", true);
  }
  else
  {
    m_network.GetMqttClient().UnpublishSensorConfig(MQTT_PHASE_ANGLE_FACTOR);
  }

  // Publish our f/w version
  m_network.GetMqttClient().PublishMessage(MQTT_FW_VERSION, MY_VERSION, true);
}


void CPvBoiler::LoadSettings()
{
  uint16_t iVal16 = 0;
  EEPROM.get(EEPROM_BP_RATING, iVal16);
  if (iVal16 < 100 || iVal16 > BOILER_POWER_RATING_MAX)
  {
    iVal16 = BOILER_POWER_RATING_DEFAULT;
  }
  m_iBoilerPowerRating = iVal16;

  EEPROM.get(EEPROM_PB_MARGIN, iVal16);
  if (iVal16 > POWER_BUDGET_MARGIN_MAX)
  {
    iVal16 = POWER_BUDGET_MARGIN_DEFAULT;
  }
  m_iPowerBudgetMargin = iVal16;

  uint8_t iVal8 = 0;
  EEPROM.get(EEPROM_CTRL_MODE, iVal8);
  m_logicMode = (iVal8 == 0x01) ? CPvBoiler::LOGIC_MODE_PERCENT : CPvBoiler::LOGIC_MODE_BUDGET;

  EEPROM.get(EEPROM_DIM_STYLE, iVal8);
  m_dimStyle = (iVal8 == 0x01) ? CPvBoiler::DIM_STYLE_SSR : CPvBoiler::DIM_STYLE_PHASE_ANGLE;

  EEPROM.get(EEPROM_SSR_PERIOD, iVal8);
  if (iVal8 < 2 || iVal8 > SSR_PERIOD_COUNT_MAX)
  {
    iVal8 = SSR_PERIOD_COUNT_DEFAULT;
  }
  m_iSsrPeriodCount = iVal8;

  float fErrorGain;
  EEPROM.get(EEPROM_ERROR_GAIN, fErrorGain);
  if (fErrorGain < ERROR_GAIN_MIN || fErrorGain > ERROR_GAIN_MAX || isnan(fErrorGain))
  {
    fErrorGain = ERROR_GAIN_DEFAULT;
  }
  m_fErrorGain = fErrorGain;

  uint8_t iErrorClamp = 0;
  EEPROM.get(EEPROM_ERROR_CLAMP, iErrorClamp);
  if (iErrorClamp < ERROR_CLAMP_MIN || iErrorClamp > ERROR_CLAMP_MAX)
  {
    iErrorClamp = ERROR_CLAMP_DEFAULT;
  }
  m_iErrorClamp = iErrorClamp;

  m_bPublishSettings = true;
}


void CPvBoiler::SetBoilerPowerRating(const uint16_t iPower)
{
  EEPROM.put(EEPROM_BP_RATING, iPower);
  EEPROM.commit();

  m_iBoilerPowerRating = iPower;

  m_bPublishSettings = true;
}


void CPvBoiler::SetPowerBudgetMargin(const uint16_t iMargin)
{
  EEPROM.put(EEPROM_PB_MARGIN, iMargin);
  EEPROM.commit();

  m_iPowerBudgetMargin = iMargin;

  m_bPublishSettings = true;
}


void CPvBoiler::SetLogicMode(const CPvBoiler::logic_mode_t logicMode)
{
  EEPROM.put(EEPROM_CTRL_MODE, (logicMode == CPvBoiler::LOGIC_MODE_PERCENT) ? 0x01 : 0x00);
  EEPROM.commit();

  m_logicMode = logicMode;

  m_bPublishSettings = true;
}


void CPvBoiler::SetDimStyle(const CPvBoiler::dim_style_t dimStyle)
{
  EEPROM.put(EEPROM_DIM_STYLE, (dimStyle == CPvBoiler::DIM_STYLE_SSR) ? 0x01 : 0x00);
  EEPROM.commit();

  m_dimStyle = dimStyle;

  m_bPublishSettings = true;
}


void CPvBoiler::SetSsrPeriodCount(const uint8_t iCount)
{
  EEPROM.put(EEPROM_SSR_PERIOD, iCount);
  EEPROM.commit();

  m_iSsrPeriodCount = iCount;

  m_bPublishSettings = true;
}


void CPvBoiler::SetErrorGain(const float fGain)
{
  EEPROM.put(EEPROM_ERROR_GAIN, fGain);
  EEPROM.commit();

  m_fErrorGain = fGain;

  m_bPublishSettings = true;
}


void CPvBoiler::SetErrorClamp(const uint8_t iClamp)
{
  EEPROM.put(EEPROM_ERROR_CLAMP, iClamp);
  EEPROM.commit();

  m_iErrorClamp = iClamp;

  m_bPublishSettings = true;
}


uint16_t CPvBoiler::CalculateTriacPhaseDelay(const uint16_t iPeriodTime, const uint16_t iZeroCrossWindow)
{
  m_iPeriodTime = iPeriodTime;
  m_iZeroCrossWindow = iZeroCrossWindow;

  uint32_t iDelay = 0;
  if (m_dimStyle == CPvBoiler::DIM_STYLE_SSR)
  {
    m_fTriacAngleFactor = 0.0f;
    m_iTriacPhaseAngle = ZERO_CROSS_EDGE_MIN_US;
    iDelay = ZERO_CROSS_EDGE_MIN_US;
  }
  else if (m_dimStyle == CPvBoiler::DIM_STYLE_PHASE_ANGLE)
  {
    // Update triac angle factor
    m_fTriacAngleFactor = triac_percentage_factor[static_cast<uint8_t>(m_fCurrentPercentage)];

    // Make sure we trigger not too close to zero cross
    m_iTriacPhaseAngle = max(static_cast<uint32_t>(m_fTriacAngleFactor * iPeriodTime), static_cast<uint32_t>(ZERO_CROSS_EDGE_MIN_US));

    // NOTE: Only turn on triac when NOT near 0% to prevent excessive EMI due to misfiring
    if (m_iTriacPhaseAngle + ZERO_CROSS_EDGE_MIN_US + GATE_PULSE_WIDTH <= iPeriodTime)
    {
      iDelay = m_iTriacPhaseAngle;
    }
  }
  else // DIM_STYLE_NONE
  {
    m_fTriacAngleFactor = 0.0f;
    m_iTriacPhaseAngle = 0;
  }

  // Update error state. Note that invalid iZeroCrossWindow-value can never happen (handled in ISR)
  m_bError = (iPeriodTime == 0 || iPeriodTime > NET_PERIOD_MAX_US);

  // With errors or zero delay return zero so we know we should do "nothing"
  if (m_bError || iDelay == 0)
  {
    return 0;
  }

  // Return phase angle including zero cross window compensation
  return iDelay + (iZeroCrossWindow / 2);
}


void CPvBoiler::Update()
{
  float fNewPercentage = m_fCurrentPercentage;

  if (m_logicMode == LOGIC_MODE_PERCENT)
  {
    if (m_iPowerPercentage > m_fCurrentPercentage)
    {
      fNewPercentage++;
    }
    else if (m_iPowerPercentage < m_fCurrentPercentage)
    {
      fNewPercentage--;
    }
  }
  else if (m_iWatchdogRecoveryCounter > 0 || !m_bCtrlEnable)
  {
    if (!m_bCtrlEnable)
    {
      m_iWatchdogRecoveryCounter = 0; // When off: quick recovery
    }

    if (m_fCurrentPercentage > 0)
    {
      fNewPercentage--; // Device off or watch-dog triggered: output to 0%
    }
  }
  else
  {
    float fErrorStep = (m_fErrorGain * 100.0f * m_iPowerBudget) / m_iBoilerPowerRating;

    // Clamp error (step) value
    if (fErrorStep > m_iErrorClamp)
      fErrorStep = m_iErrorClamp;
    else if (fErrorStep < -m_iErrorClamp)
      fErrorStep = -m_iErrorClamp;

    if (m_iPowerBudget > m_iPowerBudgetMargin || m_iPowerBudget < -m_iPowerBudgetMargin)
      fNewPercentage += fErrorStep;
  }

  // Clamp to 0-100%
  if (fNewPercentage > 100.0f)
    fNewPercentage = 100.0f;
  else if (fNewPercentage < 0.0f)
    fNewPercentage = 0.0f;

  if (fNewPercentage != m_fCurrentPercentage)
  {
    m_fCurrentPercentage = fNewPercentage;
    m_bPublishOutputPercentage = true;
  }
}


void CPvBoiler::CheckWatchDog()
{
#ifdef WATCHDOG_TIMEOUT_TIME
  // Handle watchdog
  if (m_iWatchdogCounter < (WATCHDOG_TIMEOUT_TIME * 1000) / CONTROL_LOOP_TIME_MS)
  {
    m_iWatchdogCounter++;
    if (m_iWatchdogRecoveryCounter > 0)
    {
      m_iWatchdogRecoveryCounter--;
    }
  }
  else
  {
    m_iWatchdogRecoveryCounter = (WATCHDOG_RECOVERY_TIME * 1000) / CONTROL_LOOP_TIME_MS;
  }
#endif
}
