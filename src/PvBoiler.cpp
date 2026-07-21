#include "PvBoiler.h"
#include "util.h"

#include <EEPROM.h>

#define CONTROL_LOOP_TIME_MS                    200   // ms

CPvBoiler::CPvBoiler(CNetwork& network) : m_network(network)
{
}


void CPvBoiler::Loop()
{
  // Run timed control loop
  if (m_loopTimer > CONTROL_LOOP_TIME_MS)
  {
    CheckWatchDog();
    Update();

    m_loopTimer = 0;
  }

  // Publish new MQTT values (if any) when timer expires (and connected)
  if (m_MQTTTimer > MQTT_UPDATE_TIME * 1000 && m_network.GetMqttClient().connected())
  {
    MqttPublishValues();

    m_MQTTTimer = 0;
  }
}


void CPvBoiler::Reset()
{
  m_iWatchdogCounter = 0;
  m_iWatchdogRecoveryCounter = 0;

  m_bCtrlEnable = true;
  m_bPublishCtrlEnable = true;

  m_iPowerBudget = 0;
  m_bPublishPowerBudget = true;

  m_iPowerPercentage = 0;
  m_bPublishPowerPercentage = true;

  m_iCurrentPercentage = 0;
  m_bPublishOutputPercentage = true;

  LoadSettings();
}


bool CPvBoiler::MqttPublishValues()
{
  if (m_bPublishCtrlEnable)
  {
    m_bPublishCtrlEnable = false;
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
    m_network.GetMqttClient().PublishMessage(MQTT_OUTPUT_PERCENTAGE, String(m_iCurrentPercentage));
    m_network.GetMqttClient().PublishMessage(MQTT_OUTPUT_POWER, String(GetCurrentPower()));
    m_network.GetMqttClient().PublishMessage(MQTT_PHASE_ANGLE, String((GetTriacPhaseAngle() / 1000.0f), 3));

    if (m_dimStyle == DIM_STYLE_PHASE_ANGLE)
    {
      m_network.GetMqttClient().PublishMessage(MQTT_PHASE_ANGLE_FACTOR, String(GetTriacAngleFactor(), 4));
    }
  }

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
      m_network.GetMqttClient().PublishMessage(MQTT_SSR_PERIOD, String(m_iSsrPeriodCount));
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

  if (m_logicMode == CPvBoiler::LOGIC_MODE_BUDGET)
  {
    m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_BUDGET, "1", "-100000", "100000");
    m_network.GetMqttClient().PublishSensorConfig(MQTT_ERROR_GAIN);
    m_network.GetMqttClient().PublishSensorConfig(MQTT_ERROR_CLAMP);
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
    m_network.GetMqttClient().PublishSensorConfig(MQTT_SSR_PERIOD);
  }
  else
  {
    m_network.GetMqttClient().UnpublishSensorConfig(MQTT_SSR_PERIOD);
  }

  // Diagnostic
  m_network.GetMqttClient().PublishSensorConfig(MQTT_WIFI_SSID, "", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_IP_ADDRESS, "", "", "", true);
//  m_network.GetMqttClient().PublishSensorConfig(MQTT_IP_NETMASK, "", "", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_PHASE_ANGLE, "ms", "duration", "measurement", true);
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


float CPvBoiler::UpdateTriacPhaseAngle(const uint32_t iPeriodTime)
{
  if (m_dimStyle == CPvBoiler::DIM_STYLE_SSR)
  {
    m_fTriacAngleFactor = 0.0f;
    m_fTriacPhaseAngle = ZERO_CROSS_EDGE_MIN_US;
  }
  else if (m_dimStyle == CPvBoiler::DIM_STYLE_PHASE_ANGLE)
  {
    // Update triac angle factor
    m_fTriacAngleFactor = triac_percentage_factor[m_iCurrentPercentage];

    const float fDelay = max((m_fTriacAngleFactor * iPeriodTime), ZERO_CROSS_EDGE_MIN_US); // Make sure we trigger not too close to zero cross

    // NOTE: Only turn on triac when NOT near 0% to prevent excessive EMI due to misfiring
    if (fDelay + ZERO_CROSS_EDGE_MIN_US + GATE_PULSE_WIDTH <= iPeriodTime)
    {
      m_fTriacPhaseAngle = fDelay;
    }
    else
    {
      m_fTriacPhaseAngle = 0; // Do nothing
    }
  }
  else // DIM_STYLE_NONE
  {
    m_fTriacAngleFactor = 0.0f;
    m_fTriacPhaseAngle = 0; // Do nothing
  }

  return m_fTriacPhaseAngle;
}


void CPvBoiler::Update()
{
  if (m_logicMode == LOGIC_MODE_PERCENT)
  {
    if (m_iPowerPercentage > m_iCurrentPercentage)
    {
      m_iCurrentPercentage++;
      m_bPublishOutputPercentage = true;
    }
    else if (m_iPowerPercentage < m_iCurrentPercentage)
    {
      m_iCurrentPercentage--;
      m_bPublishOutputPercentage = true;
    }
  }
  else if (m_iWatchdogRecoveryCounter > 0 || !m_bCtrlEnable)
  {
    if (!m_bCtrlEnable)
    {
      m_iWatchdogRecoveryCounter = 0; // When off: quick recovery
    }

    if (m_iCurrentPercentage > 0)
    {
      m_iCurrentPercentage--; // Device off or watch-dog triggered: output to 0%
      m_bPublishOutputPercentage = true;
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

    int32_t iOutputPercentage = m_iCurrentPercentage;
    if (m_iPowerBudget > m_iPowerBudgetMargin || m_iPowerBudget < -m_iPowerBudgetMargin)
      iOutputPercentage += fErrorStep;

    // Clamp percentage to 0-100
    if (iOutputPercentage > 100)
      iOutputPercentage = 100;
    else if (iOutputPercentage < 0)
      iOutputPercentage = 0;

    if (iOutputPercentage != m_iCurrentPercentage)
    {
      m_iCurrentPercentage = iOutputPercentage;
      m_bPublishOutputPercentage = true;
    }
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
