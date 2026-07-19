#include "pvboiler.h"
#include "util.h"

#include <EEPROM.h>

#define CONTROL_LOOP_TIME_MS                    200   // ms

CPVBoiler::CPVBoiler(CNetwork& network) : m_network(network)
{
  Reset();
}


void CPVBoiler::Loop()
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


void CPVBoiler::Reset()
{
  m_iWatchdogCounter = 0;
  m_iWatchdogRecoveryCounter = 0;

  m_bCtrlEnable = true;
  m_bUpdateCtrlEnable = true;

  m_iPowerBudget = 0;
  m_bUpdatePowerBudget = true;

  m_iPowerPercentage = 0;
  m_bUpdatePowerPercentage = true;

  m_iCurrentPercentage = 0;
  m_bUpdateOutputPercentage = true;

  LoadSettings();
}


bool CPVBoiler::MqttPublishValues()
{
  if (m_bUpdateCtrlEnable)
  {
    m_bUpdateCtrlEnable = false;
    m_network.GetMqttClient().publish(MQTT_NAME "/" MQTT_CONTROLLER_ON_OFF, m_bCtrlEnable ? "1" : "0", true);
  }

  if (m_logicMode == LOGIC_MODE_BUDGET && m_bUpdatePowerBudget)
  {
    m_bUpdatePowerBudget = false;

    char strTemp[6];

    snprintf(strTemp, 6, "%i", m_iPowerBudget);
    m_network.GetMqttClient().publish(MQTT_NAME "/" MQTT_SET_POWER_BUDGET, strTemp, true);
  }

  if (m_logicMode == LOGIC_MODE_PERCENT && m_bUpdatePowerPercentage)
  {
    m_bUpdatePowerPercentage = false;

    char strTemp[6];

    snprintf(strTemp, 6, "%i", m_iPowerPercentage);
    m_network.GetMqttClient().publish(MQTT_NAME "/" MQTT_SET_POWER_PERCENTAGE, strTemp, true);
  }

  if (m_bUpdateOutputPercentage)
  {
    m_bUpdateOutputPercentage = false;

    char strTemp[7];

    snprintf(strTemp, 7, "%i", m_iCurrentPercentage);
    m_network.GetMqttClient().publish(MQTT_NAME "/" MQTT_OUTPUT_PERCENTAGE, strTemp, true);

    snprintf(strTemp, 7, "%i", GetCurrentPower());
    m_network.GetMqttClient().publish(MQTT_NAME "/" MQTT_OUTPUT_POWER, strTemp, true);
  }

  return true;
}


void CPVBoiler::MqttPublishConfig()
{
  // Publish MQTT config for eg. HA discovery and subscribe to control topics
  m_network.GetMqttClient().PublishSwitchConfig(MQTT_CONTROLLER_ON_OFF);

  if (m_logicMode == CPVBoiler::LOGIC_MODE_PERCENT)
  {
    m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_PERCENTAGE, "0", "100", "1");
  }
  else
  {
    m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_BUDGET, "", "", "0.1");
  }

  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_POWER, "W", "power");

  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_PERCENTAGE, "%", "power_factor");

  // Publish our f/w version
  m_network.GetMqttClient().publish(MQTT_NAME "/" MQTT_FW_VERSION, MY_VERSION, true);
}


void CPVBoiler::LoadSettings()
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
  m_logicMode = (iVal8 == 0x01) ? CPVBoiler::LOGIC_MODE_PERCENT : CPVBoiler::LOGIC_MODE_BUDGET;

  EEPROM.get(EEPROM_DIM_STYLE, iVal8);
  m_dimStyle = (iVal8 == 0x01) ? CPVBoiler::DIM_STYLE_SSR : CPVBoiler::DIM_STYLE_PHASE_ANGLE;

  EEPROM.get(EEPROM_SSR_PERIOD, iVal8);
  if (iVal8 < 2 || iVal8 > SSR_PERIOD_COUNT_MAX)
  {
    iVal8 = SSR_PERIOD_COUNT_DEFAULT;
  }
  m_iSsrPeriodCount = iVal8;

  float fGain;
  EEPROM.get(EEPROM_ERROR_GAIN, fGain);
  if (fGain < ERROR_GAIN_MIN || fGain > ERROR_GAIN_MAX || isnan(fGain))
  {
    fGain = ERROR_GAIN_DEFAULT;
  }
  m_fErrorGain = fGain;
}


void CPVBoiler::EepromCommit()
{
  noInterrupts(); // Enter critical section
  EEPROM.commit();
  interrupts(); // Leave critical section
}


void CPVBoiler::SetBoilerPowerRating(const uint16_t& iPower)
{
  EEPROM.put(EEPROM_BP_RATING, iPower);
  EepromCommit();

  m_iBoilerPowerRating = iPower;
}


void CPVBoiler::SetPowerBudgetMargin(const uint16_t& iMargin)
{
  EEPROM.put(EEPROM_PB_MARGIN, iMargin);
  EepromCommit();

  m_iPowerBudgetMargin = iMargin;
}


void CPVBoiler::SetLogicMode(const CPVBoiler::logic_mode_t& logicMode)
{
  EEPROM.put(EEPROM_CTRL_MODE, (logicMode == CPVBoiler::LOGIC_MODE_PERCENT) ? 0x01 : 0x00);
  EepromCommit();

  m_logicMode = logicMode;
}


void CPVBoiler::SetDimStyle(const CPVBoiler::dim_style_t& dimStyle)
{
  EEPROM.put(EEPROM_DIM_STYLE, (dimStyle == CPVBoiler::DIM_STYLE_SSR) ? 0x01 : 0x00);
  EepromCommit();

  m_dimStyle = dimStyle;
}


void CPVBoiler::SetSsrPeriodCount(const uint8_t& iCount)
{
  EEPROM.put(EEPROM_SSR_PERIOD, iCount);
  EepromCommit();

  m_iSsrPeriodCount = iCount;
}


void CPVBoiler::SetErrorGain(const float& fGain)
{
  EEPROM.put(EEPROM_ERROR_GAIN, fGain);
  EepromCommit();

  m_fErrorGain = fGain;
}


void CPVBoiler::Update()
{
  if (m_logicMode == LOGIC_MODE_PERCENT)
  {
    if (m_iPowerPercentage > m_iCurrentPercentage)
    {
      m_iCurrentPercentage++;
      m_bUpdateOutputPercentage = true;
    }
    else if (m_iPowerPercentage < m_iCurrentPercentage)
    {
      m_iCurrentPercentage--;
      m_bUpdateOutputPercentage = true;
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
      m_bUpdateOutputPercentage = true;
    }
  }
  else
  {
    const float fStepPercentage = (m_fErrorGain * 100.0f * m_iPowerBudget) / m_iBoilerPowerRating;
    int32_t iOutputPercentage = m_iCurrentPercentage;
    if (m_iPowerBudget > m_iPowerBudgetMargin || m_iPowerBudget < -m_iPowerBudgetMargin)
      iOutputPercentage += fStepPercentage;

    if (m_iCurrentPercentage > 100)
      iOutputPercentage = 100;
    else if (m_iCurrentPercentage < 0)
      iOutputPercentage = 0;

    if (iOutputPercentage != m_iCurrentPercentage)
    {
      m_iCurrentPercentage = iOutputPercentage;
      m_bUpdateOutputPercentage = true;
    }
  }
}


void CPVBoiler::CheckWatchDog()
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
