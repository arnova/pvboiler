#include "PvBoiler.h"
#include "util.h"

#include <EEPROM.h>

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
    CheckNetworkWatchDog();
    Update();

    m_loopTimer = 0;
  }

  // Publish new MQTT values (if any) when timer expires (and connected)
  if (m_mqttPublishTimer > MQTT_UPDATE_TIME * 1000 && m_network.IsMqttConnected())
  {
    MqttPublishValues();

    m_mqttPublishTimer = 0;
  }
}


void CPvBoiler::Reset()
{
  m_iNetworkWatchdogCounter = 0;
  m_iNetworkWatchdogRecoveryCounter = 0;

  m_bCtrlEnable = true;
  m_bPublishCtrlOnOff = true;

  m_iPowerBudget = 0;
  m_bPublishPowerBudget = true;

  m_iPowerPercentage = 0;
  m_bPublishPowerPercentage = true;

  m_bPowerBoost = false;
  m_bPublishPowerBoost = true;

  m_fCurrentPercentage = 0.0f;
  m_bPublishOutputPercentage = true;

  m_bPublishSettings = true;
  m_iErrorCount = 0;

  m_fTriacAngleFactor = 0.0f;
  m_iTriacPhaseAngle = 0;
  m_iPeriodTime = 65535;
  m_iZeroCrossWindow = ZERO_CROSS_WINDOW_DEFAULT;

  LoadSettings();
}


bool CPvBoiler::MqttPublishValues(const bool bForce /* = false */)
{
  char strBuf[24]; // Enough room for signed/unsigned 32 bit number or our floats with 4 digit precision

  if (m_bPublishCtrlOnOff || bForce)
  {
    m_bPublishCtrlOnOff = false;
    m_network.GetMqttClient().PublishMessage(MQTT_CONTROLLER_ON_OFF, m_bCtrlEnable ? "1" : "0");
  }

  if (m_bPublishPowerBudget || bForce)
  {
    m_bPublishPowerBudget = false;

    if (m_logicMode == LOGIC_MODE_BUDGET)
    {
      snprintf(strBuf, sizeof(strBuf), "%d", m_iPowerBudget);
      m_network.GetMqttClient().PublishMessage(MQTT_SET_POWER_BUDGET, strBuf);
    }
  }

  if (m_bPublishPowerPercentage || bForce)
  {
    m_bPublishPowerPercentage = false;

    if (m_logicMode == LOGIC_MODE_PERCENT)
    {
      snprintf(strBuf, sizeof(strBuf), "%u", m_iPowerPercentage);
      m_network.GetMqttClient().PublishMessage(MQTT_SET_POWER_PERCENTAGE, strBuf);
    }
  }

  if (m_bPublishPowerBoost || bForce)
  {
    m_bPublishPowerBoost = false;
    m_network.GetMqttClient().PublishMessage(MQTT_POWER_BOOST_ON_OFF, m_bPowerBoost ? "1" : "0");
  }

  if (m_bPublishOutputPercentage || bForce)
  {
    m_bPublishOutputPercentage = false;

    snprintf(strBuf, sizeof(strBuf), "%.2f", m_fCurrentPercentage);
    m_network.GetMqttClient().PublishMessage(MQTT_OUTPUT_PERCENTAGE, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", GetCurrentPower());
    m_network.GetMqttClient().PublishMessage(MQTT_OUTPUT_POWER, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", GetTriacPhaseAngle());
    m_network.GetMqttClient().PublishMessage(MQTT_PHASE_ANGLE, strBuf);

    if (m_dimStyle == DIM_STYLE_PHASE_ANGLE)
    {
      snprintf(strBuf, sizeof(strBuf), "%.4f", GetTriacAngleFactor());
      m_network.GetMqttClient().PublishMessage(MQTT_PHASE_ANGLE_FACTOR, strBuf);
    }
  }

  // FIXME: These are always updated
  m_network.GetMqttClient().PublishMessage(MQTT_POWER_ERROR, GetError() ? "1" : "0");

  // NOTE: Actual period is *2 since what we detect is rectified 50 Hz
  snprintf(strBuf, sizeof(strBuf), "%u", m_iPeriodTime * 2);
  m_network.GetMqttClient().PublishMessage(MQTT_NET_PERIOD, strBuf);

  snprintf(strBuf, sizeof(strBuf), "%.2f", (500.0f * 1000.0f) / m_iPeriodTime);
  m_network.GetMqttClient().PublishMessage(MQTT_NET_FREQUENCY, strBuf);

  snprintf(strBuf, sizeof(strBuf), "%u", m_iZeroCrossWindow);
  m_network.GetMqttClient().PublishMessage(MQTT_ZERO_CROSS_WINDOW, strBuf);

  // Publish uptime
  const CUptime::uptime_t upTime = GetUpTime();
  snprintf(strBuf, sizeof(strBuf), "%uy %ud %02u:%02u:%02u", upTime.iYears, upTime.iDays, upTime.iHours, upTime.iMinutes, upTime.iSeconds);
  m_network.GetMqttClient().PublishMessage(MQTT_UP_TIME, strBuf);

  if (m_bPublishSettings || bForce)
  {
    m_bPublishSettings = false;

    snprintf(strBuf, sizeof(strBuf), "%u", m_iBoilerPowerRating);
    m_network.GetMqttClient().PublishMessage(MQTT_BOILER_POWER_RATING, strBuf);

    if (m_logicMode == LOGIC_MODE_BUDGET)
    {
      m_network.GetMqttClient().PublishMessage(MQTT_SET_LOGIC_MODE, "Budget");

      snprintf(strBuf, sizeof(strBuf), "%u", m_iDeadZone);
      m_network.GetMqttClient().PublishMessage(MQTT_DEAD_ZONE, strBuf);

      snprintf(strBuf, sizeof(strBuf), "%.3f", m_fErrorGain);
      m_network.GetMqttClient().PublishMessage(MQTT_ERROR_GAIN, strBuf);

      snprintf(strBuf, sizeof(strBuf), "%.2f", m_fStepClamp);
      m_network.GetMqttClient().PublishMessage(MQTT_STEP_CLAMP, strBuf);
    }
    else // Percentage
    {
      m_network.GetMqttClient().PublishMessage(MQTT_SET_LOGIC_MODE, "Percentage");
    }

    if (m_dimStyle == DIM_STYLE_SSR)
    {
      m_network.GetMqttClient().PublishMessage(MQTT_DIM_STYLE, "SSR");

      snprintf(strBuf, sizeof(strBuf), "%u", m_iSsrPeriodCount);
      m_network.GetMqttClient().PublishMessage(MQTT_SSR_PERIOD_COUNT, strBuf);
    }
    else
    {
      m_network.GetMqttClient().PublishMessage(MQTT_DIM_STYLE, "Phase-angle");
    }

    snprintf(strBuf, sizeof(strBuf), "%u", m_iNetWatchDogTimeout);
    m_network.GetMqttClient().PublishMessage(MQTT_NET_WD_TIMEOUT, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iNetWatchDogRecovery);
    m_network.GetMqttClient().PublishMessage(MQTT_NET_WD_RECOVERY, strBuf);
  }

  return true;
}


void CPvBoiler::MqttPublishConfig()
{
  // Publish MQTT config for eg. HA discovery and subscribe to control topics
  m_network.GetMqttClient().PublishSwitchConfig(MQTT_CONTROLLER_ON_OFF);
  m_network.GetMqttClient().PublishSwitchConfig(MQTT_POWER_BOOST_ON_OFF);

  m_network.GetMqttClient().PublishBinarySensorConfig(MQTT_POWER_ERROR, true);

  if (m_logicMode == CPvBoiler::LOGIC_MODE_BUDGET)
  {
    m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_BUDGET, "1", "-100000", "100000");
    m_network.GetMqttClient().PublishSensorConfig(MQTT_ERROR_GAIN, "", "", "", true);
    m_network.GetMqttClient().PublishSensorConfig(MQTT_STEP_CLAMP, "%", "", "", true);
    m_network.GetMqttClient().PublishSensorConfig(MQTT_DEAD_ZONE, "W", "power", "", true);

    m_network.GetMqttClient().UnpublishNumberConfig(MQTT_SET_POWER_PERCENTAGE);
  }
  else // Percentage
  {
    m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_PERCENTAGE, "1", "0", "100", false);

    m_network.GetMqttClient().UnpublishNumberConfig(MQTT_SET_POWER_BUDGET);
    m_network.GetMqttClient().UnpublishNumberConfig(MQTT_ERROR_GAIN);
    m_network.GetMqttClient().UnpublishNumberConfig(MQTT_STEP_CLAMP);
    m_network.GetMqttClient().UnpublishNumberConfig(MQTT_DEAD_ZONE);
  }

  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_POWER, "W", "power");
  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_PERCENTAGE, "%", "");

  static const char* strSelectValues[] = { "Percentage", "Budget" };
  m_network.GetMqttClient().PublishSelectConfig(MQTT_SET_LOGIC_MODE, strSelectValues, 2);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_BOILER_POWER_RATING, "W", "power", "", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_DIM_STYLE, "", "", "", true);

  if (m_dimStyle == DIM_STYLE_SSR)
  {
    m_network.GetMqttClient().PublishSensorConfig(MQTT_SSR_PERIOD_COUNT, "", "", "", true);
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

  m_network.GetMqttClient().PublishSensorConfig(MQTT_NET_FREQUENCY, "Hz", "", "", true);

//  m_network.GetMqttClient().PublishSensorConfig(MQTT_ZERO_CROSS_WINDOW, "ms", "duration", "measurement", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_ZERO_CROSS_WINDOW, "us", "", "", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_NET_WD_TIMEOUT, "s", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_NET_WD_RECOVERY, "s", "", "", true);
  
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
  uint8_t iVal8 = 0;
  uint16_t iVal16 = 0;
  float fVal = 0.0f;

  EEPROM.get(EEPROM_BP_RATING, iVal16);
  if (iVal16 < 100 || iVal16 > BOILER_POWER_RATING_MAX)
  {
    iVal16 = BOILER_POWER_RATING_DEFAULT;
  }
  m_iBoilerPowerRating = iVal16;

  EEPROM.get(EEPROM_DEAD_ZONE, iVal8);
  if (iVal8 < DEAD_ZONE_MIN || iVal8 > DEAD_ZONE_MAX)
  {
    iVal8 = DEAD_ZONE_DEFAULT;
  }
  m_iDeadZone = iVal8;

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

  EEPROM.get(EEPROM_ERROR_GAIN, fVal);
  if (fVal < ERROR_GAIN_MIN || fVal > ERROR_GAIN_MAX || isnan(fVal))
  {
    fVal = ERROR_GAIN_DEFAULT;
  }
  m_fErrorGain = fVal;

  EEPROM.get(EEPROM_STEP_CLAMP, fVal);
  if (fVal < STEP_CLAMP_MIN || fVal > STEP_CLAMP_MAX || isnan(fVal))
  {
    fVal = STEP_CLAMP_DEFAULT;
  }
  m_fStepClamp = fVal;

  EEPROM.get(EEPROM_NET_WD_TIMEOUT, iVal16);
  if (iVal16 > NETWORK_WATCHDOG_TIMEOUT_MAX)
  {
    iVal16 = NETWORK_WATCHDOG_TIMEOUT_DEFAULT;
  }
  m_iNetWatchDogTimeout = iVal16;

  EEPROM.get(EEPROM_NET_WD_RECOVER, iVal16);
  if (iVal16 > NETWORK_WATCHDOG_RECOVERY_MAX)
  {
    iVal16 = NETWORK_WATCHDOG_RECOVERY_DEFAULT;
  }
  m_iNetWatchDogRecovery = iVal16;

  m_bPublishSettings = true;
}


void CPvBoiler::SetBoilerPowerRating(const uint16_t iPower)
{
  if (iPower != m_iBoilerPowerRating)
  {
    EEPROM.put(EEPROM_BP_RATING, iPower);
    EEPROM.commit();

    m_iBoilerPowerRating = iPower;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDeadZone(const uint8_t iDeadZone)
{
  if (iDeadZone != m_iDeadZone)
  {
    EEPROM.put(EEPROM_DEAD_ZONE, iDeadZone);
    EEPROM.commit();

    m_iDeadZone = iDeadZone;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetLogicMode(const CPvBoiler::logic_mode_t logicMode)
{
  if (logicMode != m_logicMode)
  {
    EEPROM.put(EEPROM_CTRL_MODE, (logicMode == CPvBoiler::LOGIC_MODE_PERCENT) ? 0x01 : 0x00);
    EEPROM.commit();

    m_logicMode = logicMode;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDimStyle(const CPvBoiler::dim_style_t dimStyle)
{
  if (dimStyle != m_dimStyle)
  {
    EEPROM.put(EEPROM_DIM_STYLE, (dimStyle == CPvBoiler::DIM_STYLE_SSR) ? 0x01 : 0x00);
    EEPROM.commit();

    m_dimStyle = dimStyle;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetSsrPeriodCount(const uint8_t iCount)
{
  if (iCount != m_iSsrPeriodCount)
  {
    EEPROM.put(EEPROM_SSR_PERIOD, iCount);
    EEPROM.commit();

    m_iSsrPeriodCount = iCount;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetErrorGain(const float fGain)
{
  if (fGain != m_fErrorGain)
  {
    EEPROM.put(EEPROM_ERROR_GAIN, fGain);
    EEPROM.commit();

    m_fErrorGain = fGain;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetStepClamp(const float fClamp)
{
  if (fClamp != m_fStepClamp)
  {
    EEPROM.put(EEPROM_STEP_CLAMP, fClamp);
    EEPROM.commit();

    m_fStepClamp = fClamp;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetNetWatchDogTimeout(const uint16_t iTime)
{
  if (iTime != m_iNetWatchDogTimeout)
  {
    EEPROM.put(EEPROM_NET_WD_TIMEOUT, iTime);
    EEPROM.commit();

    m_iNetWatchDogTimeout = iTime;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetNetWatchDogRecovery(const uint16_t iTime)
{
  if (iTime != m_iNetWatchDogRecovery)
  {
    EEPROM.put(EEPROM_NET_WD_RECOVER, iTime);
    EEPROM.commit();

    m_iNetWatchDogRecovery = iTime;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::FactoryReset()
{
  SetBoilerPowerRating(BOILER_POWER_RATING_DEFAULT);
  SetDeadZone(DEAD_ZONE_DEFAULT);
  SetLogicMode(LOGIC_MODE_BUDGET);
  SetDimStyle(DIM_STYLE_PHASE_ANGLE);
  SetSsrPeriodCount(SSR_PERIOD_COUNT_DEFAULT);
  SetErrorGain(ERROR_GAIN_DEFAULT);
  SetStepClamp(STEP_CLAMP_DEFAULT);
  SetNetWatchDogTimeout(NETWORK_WATCHDOG_TIMEOUT_DEFAULT);
  SetNetWatchDogRecovery(NETWORK_WATCHDOG_RECOVERY_DEFAULT);

  // Reset controller
  Reset();
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
  if (iPeriodTime < NET_PERIOD_MIN_US || iPeriodTime > NET_PERIOD_MAX_US)
  {
    if (m_iErrorCount < 255)
    {
      m_iErrorCount++;
    }

    return 0;
  }
  else
  {
    if (m_iErrorCount > 0)
    {
      m_iErrorCount--;
    }
  }

  // With zero delay return zero so we know we should do "nothing"
  if (iDelay == 0)
  {
    return 0;
  }

  // Return phase angle including zero cross window compensation
  return iDelay + (iZeroCrossWindow / 2);
}


void CPvBoiler::Update()
{
  float fNewPercentage = m_fCurrentPercentage;

  if (m_iNetworkWatchdogRecoveryCounter > 0 || !m_bCtrlEnable || GetError())
  {
    if (!m_bCtrlEnable)
    {
      m_iNetworkWatchdogRecoveryCounter = 0; // When off: quick recovery
    }

    if (m_fCurrentPercentage > 0)
    {
      fNewPercentage -= m_fStepClamp; // Device off or watch-dog triggered: output to 0%
    }
  }
  else if (m_bPowerBoost)
  {
    fNewPercentage = 100.0f; // Immediately 100% power
  }
  else if (m_logicMode == LOGIC_MODE_PERCENT)
  {
    if (m_iPowerPercentage > m_fCurrentPercentage)
    {
      fNewPercentage += m_fStepClamp;

      if (fNewPercentage > m_iPowerPercentage)
      {
        fNewPercentage = m_iPowerPercentage;
      }
    }
    else if (m_iPowerPercentage < m_fCurrentPercentage)
    {
      fNewPercentage -= m_fStepClamp;

      if (fNewPercentage < m_iPowerPercentage)
      {
        fNewPercentage = m_iPowerPercentage;
      }
    }
  }
  else
  {
    float fErrorStep = (m_fErrorGain * 100.0f * m_iPowerBudget) / m_iBoilerPowerRating;

    // Clamp error (step) value
    if (fErrorStep > m_fStepClamp)
    {
      fErrorStep = m_fStepClamp;
    }
    else if (fErrorStep < -m_fStepClamp)
    {
      fErrorStep = -m_fStepClamp;
    }

    // Only change value when outside deadzone
    if (m_iPowerBudget > m_iDeadZone || m_iPowerBudget < -m_iDeadZone)
    {
      fNewPercentage += fErrorStep;
    }
  }

  // Clamp to 0-100%
  if (fNewPercentage > 100.0f)
  {
    fNewPercentage = 100.0f;
  }
  else if (fNewPercentage < 0.0f)
  {
    fNewPercentage = 0.0f;
  }

  if (fNewPercentage != m_fCurrentPercentage)
  {
    m_fCurrentPercentage = fNewPercentage;
    m_bPublishOutputPercentage = true;
  }
}


void CPvBoiler::CheckNetworkWatchDog()
{
  if (m_iNetWatchDogTimeout == 0)
    return;

  // Handle network watchdog
  if (m_iNetworkWatchdogCounter < (m_iNetWatchDogTimeout * 1000) / CONTROL_LOOP_TIME_MS)
  {
    m_iNetworkWatchdogCounter++;
    if (m_iNetworkWatchdogRecoveryCounter > 0)
    {
      m_iNetworkWatchdogRecoveryCounter--;
    }
  }
  else
  {
    m_iNetworkWatchdogRecoveryCounter = (m_iNetWatchDogRecovery * 1000) / CONTROL_LOOP_TIME_MS;
  }
}
