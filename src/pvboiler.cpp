#include "pvboiler.h"
#include "util.h"

#define CONTROL_LOOP_TIME_MS                    200   // ms

const float& CPVBoiler::GetTriacAngleFactor() const
{
  return triac_percentage_factor[m_iOutputPercentage];
}


bool CPVBoiler::MQTTPublishValues()
{
  if (m_bUpdateCtrlEnable)
  {
    m_bUpdateCtrlEnable = false;
    m_pMQTTClient->publish(MQTT_NAME "/" MQTT_CONTROLLER_ON_OFF, m_bCtrlEnable ? "1" : "0", true);
  }

  if (!m_bPowerPercControl && m_bUpdatePowerBudget)
  {
    m_bUpdatePowerBudget = false;

    char strTemp[6];

    snprintf(strTemp, 6, "%i", m_iPowerBudget);
    m_pMQTTClient->publish(MQTT_NAME "/" MQTT_SET_POWER_BUDGET, strTemp, true);
  }

  if (m_bPowerPercControl && m_bUpdatePowerPercentage)
  {
    m_bUpdatePowerPercentage = false;

    char strTemp[6];

    snprintf(strTemp, 6, "%i", m_iPowerPercentage);
    m_pMQTTClient->publish(MQTT_NAME "/" MQTT_SET_POWER_PERCENTAGE, strTemp, true);
  }

  if (m_bUpdateOutputPercentage)
  {
    m_bUpdateOutputPercentage = false;

    char strTemp[7];

    snprintf(strTemp, 7, "%i", m_iOutputPercentage);
    m_pMQTTClient->publish(MQTT_NAME "/" MQTT_OUTPUT_PERCENTAGE, strTemp, true);

    snprintf(strTemp, 7, "%i", (m_iBoilerPowerRating * m_iOutputPercentage) / 100);
    m_pMQTTClient->publish(MQTT_NAME "/" MQTT_OUTPUT_POWER, strTemp, true);
  }

  return true;
}


void CPVBoiler::Update()
{
  if (m_bPowerPercControl)
  {
    if (m_iPowerPercentage > m_iOutputPercentage)
    {
      m_iOutputPercentage++;
      m_bUpdateOutputPercentage = true;
    }
    else if (m_iPowerPercentage < m_iOutputPercentage)
    {
      m_iOutputPercentage--;
      m_bUpdateOutputPercentage = true;
    }
  }
  else if (m_iWatchdogRecoveryCounter > 0 || !m_bCtrlEnable)
  {
    if (!m_bCtrlEnable)
    {
      m_iWatchdogRecoveryCounter = 0; // When off: quick recovery
    }

    if (m_iOutputPercentage > 0)
    {
      m_iOutputPercentage--; // Device off or watch-dog triggered: output to 0%
      m_bUpdateOutputPercentage = true;
    }
  }
  else
  {
    const float fStepPercentage = (m_fErrorGain * 100.0f * m_iPowerBudget) / m_iBoilerPowerRating;
    int32_t iOutputPercentage = m_iOutputPercentage;
    if (m_iPowerBudget > m_iPowerBudgetMargin || m_iPowerBudget < -m_iPowerBudgetMargin)
      iOutputPercentage += fStepPercentage;

    if (iOutputPercentage > 100)
      iOutputPercentage = 100;
    else if (iOutputPercentage < 0)
      iOutputPercentage = 0;

    if (m_iOutputPercentage != iOutputPercentage)
    {
      m_iOutputPercentage = iOutputPercentage;
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


void CPVBoiler::loop()
{
  // Run timed control loop
  if (m_loopTimer > CONTROL_LOOP_TIME_MS)
  {
    CheckWatchDog();
    Update();

    m_loopTimer = 0;
  }

  // Publish new MQTT values (if any) when timer expires (and connected)
  if (m_MQTTTimer > MQTT_UPDATE_TIME * 1000 && m_pMQTTClient->connected())
  {
    MQTTPublishValues();

    m_MQTTTimer = 0;
  }
}