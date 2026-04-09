#include "pvboiler.h"
#include "util.h"

CPVBoiler::CPVBoiler(PubSubClient& MQTTClient)
{
  m_pMQTTClient = &MQTTClient;
}


float CPVBoiler::GetTriacAngleFactor()
{
  return triac_percentage_factor[m_iOutputPercentage];
}


bool CPVBoiler::MQTTPublishValues()
{
  if (m_bUpdateCtrlEnable)
  {
    m_bUpdateCtrlEnable = false;
    m_pMQTTClient->publish(MQTT_PVBOILER_NAME "/" MQTT_CONTROLLER_ON_OFF, m_bCtrlEnable ? "1" : "0", true);
  }

#ifdef MQTT_SET_POWER_BUDGET
  if (m_bUpdatePowerBudget)
  {
    m_bUpdatePowerBudget = false;

    char strTemp[6];

    snprintf(strTemp, 6, "%i", m_iPowerBudget);
    m_pMQTTClient->publish(MQTT_PVBOILER_NAME "/" MQTT_SET_POWER_BUDGET, strTemp, true);
  }
#endif

#ifdef MQTT_SET_TARGET_PERCENTAGE
  if (m_bUpdatePowerPercentage)
  {
    m_bUpdatePowerPercentage = false;

    char strTemp[6];

    snprintf(strTemp, 6, "%i", m_iPowerPercentage);
    m_pMQTTClient->publish(MQTT_PVBOILER_CTRL_PREFIX MQTT_SET_TARGET_PERCENTAGE, strTemp, true);
  }
#endif

  if (m_bUpdateOutputPercentage)
  {
    m_bUpdateOutputPercentage = false;

    char strTemp[6];

    snprintf(strTemp, 6, "%i", m_iOutputPercentage);
    m_pMQTTClient->publish(MQTT_PVBOILER_NAME "/" MQTT_OUTPUT_PERCENTAGE, strTemp, true);

    snprintf(strTemp, 6, "%i", (BOILER_POWER * m_iOutputPercentage) / 100);
    m_pMQTTClient->publish(MQTT_PVBOILER_NAME "/" MQTT_OUTPUT_POWER, strTemp, true);
  }

  return true;
}


void CPVBoiler::Update()
{
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

  if (m_iWatchdogRecoveryCounter > 0 || !m_bCtrlEnable)
  {
    if (m_iOutputPercentage != 0)
    {
      m_iOutputPercentage = 0; // Device off or watch-dog triggered: output to 0%
      m_bUpdateOutputPercentage = true;
    }
  }
  else
  {
#ifdef PERCENTAGE_CONTROL
    if (m_iOutputPercentage < m_iPowerPercentage)
    {
      m_iOutputPercentage++;
      m_bUpdateOutputPercentage = true;
    }
    else if (m_iOutputPercentage > m_iPowerPercentage)
    {
      m_iOutputPercentage--;
      m_bUpdateOutputPercentage = true;
    }
#else
    // Simple algoritm where we, for now, simply check whether there's any power budget left or not
    if (m_iPowerBudget > POWER_BUDGET_MIN)
    {
      if (m_iOutputPercentage < 100)
      {
        m_iOutputPercentage++;
        m_bUpdateOutputPercentage = true;
      }
    }
    else if (m_iOutputPercentage > 0)
    {
      m_iOutputPercentage--;
      m_bUpdateOutputPercentage = true;
    }
#endif
  }
}


void CPVBoiler::loop()
{
  // Run timed control loop
  if (m_loopTimer > CONTROL_LOOP_TIME_MS)
  {
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