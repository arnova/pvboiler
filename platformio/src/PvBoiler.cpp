#include "PvBoiler.h"
#include "util.h"

#include <EEPROM.h>

// Make sure the thermostat never drops to/below the disinfect temperature during disinfection (this would reset the disinfect run time)
static_assert(TEMPERATURE_DISINFECT_SETPOINT - TEMPERATURE_OVERRIDE_HYSTERESIS > DISINFECT_TEMPERATURE, "TEMPERATURE_DISINFECT_SETPOINT too low");

CPvBoiler::CPvBoiler(CNetwork& network) : m_network(network), m_oneWire(ONE_WIRE), m_tempSensors(&m_oneWire)
{
  m_tempSensors.begin();
  m_tempSensors.setResolution(9);
  m_tempSensors.setWaitForConversion(false); // Do not block but poll
  m_tempSensors.requestTemperatures(); // Request first temperature

  m_boilerTemperatureAverage.SetAvgCount(100);
}


void CPvBoiler::Loop()
{
  // Update uptime
  m_upTime.Update();

  // Run timed control loop
  if (m_loopTimer > CONTROL_LOOP_TIME_MS)
  {
    CheckNetworkWatchDog();

    if (m_tempSensors.getDeviceCount() > 0)
    {
      const float fTemperature = m_tempSensors.getTempCByIndex(0); // first sensor on the bus

      // Note: 85.0 is the startup value so we can't see the difference between restarts and an actual value of 85.0c
      // therefore in case of 85.0 we need TEMPERATURE_MAX_RETRIES before the value is actually used
      if (fTemperature != DEVICE_DISCONNECTED_C && fTemperature != 85.0f)
      {
        m_iBoilerTemperatureRetryCount = 0;

        const float fTemperatureAveraged = m_boilerTemperatureAverage.UpdateValue(fTemperature);
        if (m_fBoilerTemperature != fTemperatureAveraged)
        {
          m_bPublishBoilerTemperature = true;
          m_fBoilerTemperature = fTemperatureAveraged;
        }

        // Over temperature protection. Use raw value as the averaged value lags behind
        if (fTemperature >= TEMPERATURE_OVERHEATING_MAX && !m_bBoilerOverHeated)
        {
          m_bBoilerOverHeated = true;
          m_bPublishBoilerOverHeated = true;
        }
      }
      else if (m_iBoilerTemperatureRetryCount < TEMPERATURE_MAX_RETRIES &&
               ++m_iBoilerTemperatureRetryCount >= TEMPERATURE_MAX_RETRIES)
      {
        // Overwrite averaged value (once) when out of retries:
        m_bPublishBoilerTemperature = true;
        m_fBoilerTemperature = fTemperature;
        m_boilerTemperatureAverage.Reset();
      }

      // Always update so legionella logic never runs on a stale temperature (invalid values are ignored by it)
      m_legionella.UpdateTemperature(m_fBoilerTemperature);

      m_legionella.Loop();

      // Request new value from temperature probe
      m_tempSensors.requestTemperatures();
    }
    else
    {
      if (m_fBoilerTemperature != -1.0f)
      {
        m_bPublishBoilerTemperature = true;
        m_fBoilerTemperature = -1.0f;
      }
    }

    // Update legionella values
    const bool bLegionellaMustDisinfect = m_legionella.MustDisinfect();
    if (m_bLegionellaMustDisinfect != bLegionellaMustDisinfect)
    {
      m_bLegionellaMustDisinfect = bLegionellaMustDisinfect;
      m_bPublishLegionellaMustDisinfect = true;
    }

    const uint32_t iDisinfectRunTimeSeconds = m_legionella.GetDisinfectRunTimeSeconds();
    if (m_iLegionellaDisinfectRunTimeSeconds != iDisinfectRunTimeSeconds)
    {
      m_iLegionellaDisinfectRunTimeSeconds = iDisinfectRunTimeSeconds;
      m_bPublishLegionellaDisinfectRunTimeSeconds = true;
    }

    // Publish new setter value only when changed else we'll loop on setting it over and over again via broker
    const uint32_t iLegionellaHoursSinceDisinfection = m_legionella.GetHoursPassedSinceLastDisinfect();
    if (m_iLegionellaHoursSinceDisinfection != iLegionellaHoursSinceDisinfection)
    {
      m_iLegionellaHoursSinceDisinfection = iLegionellaHoursSinceDisinfection;
      m_bPublishLegionellaHoursSinceDisinfectionSetter = true;
    }

    // Publish new setter value only when changed else we'll loop on setting it over and over again via broker
    const uint32_t iLegionellaDangerZoneHours = m_legionella.GetHoursInDangerZone();
    if (m_iLegionellaDangerZoneHours != iLegionellaDangerZoneHours)
    {
      m_iLegionellaDangerZoneHours = iLegionellaDangerZoneHours;
      m_bPublishLegionellaDangerZoneHoursSetter = true;
    }

    MqttPublishValues();

    Update();

    m_loopTimer = 0;
  }
}


void CPvBoiler::Reset()
{
  m_iNetworkWatchdogCounter = 0;
  m_iNetworkWatchdogRecoveryCounter = 0;

  m_iPowerBudget = 0;
  m_bPublishPowerBudget = true;

  m_iPowerPercentage = 0;
  m_bPublishPowerPercentage = true;

  m_fCurrentPercentage = 0.0f;
  m_bPublishOutputPercentage = true;

  m_bPublishSettings = true;

  m_bPowerGood = true;
  m_bPowerGoodFlag = true;
  m_bPublishPowerGoodFlag = true;

  m_fTriacAngleFactor = 0.0f;
  m_iTriacPhaseAngle = 0;
  m_iPeriodTime = NET_PERIOD_INVALID;
  m_iZeroCrossWindow = ZERO_CROSS_WINDOW_INVALID;
  m_bPublishMainsValues = true;

  m_fBoilerTemperature = -1.0f;
  m_bPublishBoilerTemperature = true;
  m_boilerTemperatureAverage.Reset();
  m_iBoilerTemperatureRetryCount = 0;
  m_legionella.UpdateTemperature(m_fBoilerTemperature);
  m_bBoilerOverHeated = false;
  m_bPublishBoilerOverHeated = true;

  m_iTemperatureOverride = TEMPERATURE_OVERRIDE_MAX;
  m_bPublishTemperatureOverride = true;
  m_bThermostatAllowsHeating = true;

  m_iLegionellaHoursSinceDisinfection = UINT32_MAX;
  m_bPublishLegionellaHoursSinceDisinfection = true;
  m_bPublishLegionellaHoursSinceDisinfectionSetter = false;

  m_iLegionellaDangerZoneHours = UINT32_MAX;
  m_bPublishLegionellaDangerZoneHours = true;
  m_bPublishLegionellaDangerZoneHoursSetter = false;

  m_iLegionellaDisinfectRunTimeSeconds = 0;
  m_bPublishLegionellaDisinfectRunTimeSeconds = true;

  m_bLegionellaMustDisinfect = false;
  m_bPublishLegionellaMustDisinfect = true;

  LoadSettings();
}


void CPvBoiler::SetLegionellaHoursSinceDisinfection(const uint32_t iVal)
{
  // May be set from both the mqtt broker (retained message) or
  // from ourselves when the value changes in the CLegionella-class
  // We know that when value differs it was not set by ourselves and
  // it's the retained message from the broker
  if (iVal != m_iLegionellaHoursSinceDisinfection)
  {
    // Be pessemistic(+1) about the retained value from the broker due to rounding (down) errors
    m_legionella.SetHoursPassedSinceLastDisinfect(iVal + 1);
  }

  m_bPublishLegionellaHoursSinceDisinfection = true;
}


void CPvBoiler::SetLegionellaDangerZoneHours(const uint32_t iVal)
{
  // May be set from both the mqtt broker (retained message) or
  // from ourselves when the value changes in the CLegionella-class
  // We know that when value differs it was not set by ourselves and
  // it's the retained message from the broker
  if (iVal != m_iLegionellaDangerZoneHours)
  {
    // Be pessemistic(+1) about the retained value from the broker due to rounding (down) errors
    m_legionella.SetHoursInDangerZone(iVal + 1);
  }

  m_bPublishLegionellaDangerZoneHours = true;
}


bool CPvBoiler::MqttPublishValues(const bool bForce /* = false */)
{
  if (!m_network.IsMqttConnected())
    return false;

  char strBuf[24]; // Enough room for signed/unsigned 32 bit number or our floats with 4 digit precision

  if (m_bPublishSettings || bForce)
  {
    m_bPublishSettings = false;

    snprintf(strBuf, sizeof(strBuf), "%u", m_iBoilerPowerRating);
    m_network.GetMqttClient().PublishMessage(MQTT_BOILER_POWER_RATING, strBuf);

    switch(m_mode)
    {
      case MODE_BUDGET:
      {
        m_network.GetMqttClient().PublishMessage(MQTT_SET_MODE, "Budget");

        snprintf(strBuf, sizeof(strBuf), "%u", m_iDeadZone);
        m_network.GetMqttClient().PublishMessage(MQTT_DEAD_ZONE, strBuf);

        snprintf(strBuf, sizeof(strBuf), "%u", m_iBudgetMargin);
        m_network.GetMqttClient().PublishMessage(MQTT_BUDGET_MARGIN, strBuf);

        snprintf(strBuf, sizeof(strBuf), "%.3f", m_fErrorGainPos);
        m_network.GetMqttClient().PublishMessage(MQTT_ERROR_GAIN_POS, strBuf);

        snprintf(strBuf, sizeof(strBuf), "%.3f", m_fErrorGainNeg);
        m_network.GetMqttClient().PublishMessage(MQTT_ERROR_GAIN_NEG, strBuf);

        snprintf(strBuf, sizeof(strBuf), "%.2f", m_fStepClampPos);
        m_network.GetMqttClient().PublishMessage(MQTT_STEP_CLAMP_POS, strBuf);

        snprintf(strBuf, sizeof(strBuf), "%.2f", m_fStepClampNeg);
        m_network.GetMqttClient().PublishMessage(MQTT_STEP_CLAMP_NEG, strBuf);
      }
      break;

      case MODE_PERCENT:
      {
        m_network.GetMqttClient().PublishMessage(MQTT_SET_MODE, "Percentage");
      }
      break;

      case MODE_OFF:
      {
        m_network.GetMqttClient().PublishMessage(MQTT_SET_MODE, "Off");
      }
      break;

      case MODE_ON:
      {
        m_network.GetMqttClient().PublishMessage(MQTT_SET_MODE, "On");
      }
      break;

      case MODE_BOOST:
      {
        m_network.GetMqttClient().PublishMessage(MQTT_SET_MODE, "Boost");
      }
      break;
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

    snprintf(strBuf, sizeof(strBuf), "%u", m_iMqttUpdateInterval);
    m_network.GetMqttClient().PublishMessage(MQTT_UPDATE_INTERVAL, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iNetWatchDogTimeout);
    m_network.GetMqttClient().PublishMessage(MQTT_NET_WD_TIMEOUT, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iNetWatchDogRecovery);
    m_network.GetMqttClient().PublishMessage(MQTT_NET_WD_RECOVERY, strBuf);
  }

  // Publish these MQTT values (if any) when timer expires (and connected)
  if (m_mqttPublishTimer > m_iMqttUpdateInterval * 1000)
  {
    m_mqttPublishTimer = 0;

    if (m_bPublishPowerBudget || bForce)
    {
      m_bPublishPowerBudget = false;

      snprintf(strBuf, sizeof(strBuf), "%d", m_iPowerBudget);
      m_network.GetMqttClient().PublishMessage(MQTT_SET_POWER_BUDGET, strBuf);
    }

    if (m_bPublishPowerPercentage || bForce)
    {
      m_bPublishPowerPercentage = false;

      snprintf(strBuf, sizeof(strBuf), "%u", m_iPowerPercentage);
      m_network.GetMqttClient().PublishMessage(MQTT_SET_POWER_PERCENTAGE, strBuf);
    }

    if (m_bPublishTemperatureOverride || bForce)
    {
      m_bPublishTemperatureOverride = false;

      snprintf(strBuf, sizeof(strBuf), "%u", m_iTemperatureOverride);
      m_network.GetMqttClient().PublishMessage(MQTT_SET_TEMPERATURE_OVERRIDE, strBuf);
    }

    if (m_bPublishOutputPercentage || bForce)
    {
      m_bPublishOutputPercentage = false;

      snprintf(strBuf, sizeof(strBuf), "%.2f", m_fCurrentPercentage);
      m_network.GetMqttClient().PublishMessage(MQTT_OUTPUT_PERCENTAGE, strBuf);

      snprintf(strBuf, sizeof(strBuf), "%u", GetCurrentPower());
      m_network.GetMqttClient().PublishMessage(MQTT_OUTPUT_POWER, strBuf);

      snprintf(strBuf, sizeof(strBuf), "%u", GetTriacPhaseAngle());
      m_network.GetMqttClient().PublishMessage(MQTT_TRIAC_PHASE_ANGLE, strBuf);

      if (m_dimStyle == DIM_STYLE_PHASE_ANGLE)
      {
        snprintf(strBuf, sizeof(strBuf), "%.4f", GetTriacAngleFactor());
        m_network.GetMqttClient().PublishMessage(MQTT_TRIAC_ANGLE_FACTOR, strBuf);
      }
    }

    if (m_bPublishBoilerTemperature || bForce)
    {
      m_bPublishBoilerTemperature = false;
      if (m_fBoilerTemperature > 0.0f)
      {
        snprintf(strBuf, sizeof(strBuf), "%.1f", m_fBoilerTemperature);
        m_network.GetMqttClient().PublishMessage(MQTT_BOILER_TEMPERATURE, strBuf);
      }
      else
      {
        m_network.GetMqttClient().PublishMessage(MQTT_BOILER_TEMPERATURE, "None"); // Home Assistant: state "unknown"
      }
    }

    if (m_bPublishBoilerOverHeated || bForce)
    {
      m_bPublishBoilerOverHeated = false;
      m_network.GetMqttClient().PublishMessage(MQTT_BOILER_OVERHEATED, m_bBoilerOverHeated ? "1" : "0");
    }

    if (m_bPublishLegionellaMustDisinfect || bForce)
    {
      m_bPublishLegionellaMustDisinfect = false;
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_MUST_DISINFECT, m_bLegionellaMustDisinfect ? "1" : "0");
    }

    if (m_bPublishLegionellaDisinfectRunTimeSeconds || bForce)
    {
      m_bPublishLegionellaDisinfectRunTimeSeconds = false;
      snprintf(strBuf, sizeof(strBuf), "%u", m_iLegionellaDisinfectRunTimeSeconds);
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DISINFECT_RUN_SECONDS, strBuf);
    }

    // Publish setter? Note: Only when changed else we'll loop on setting it over and over again via broker
    if (m_bPublishLegionellaHoursSinceDisinfectionSetter)
    {
      m_bPublishLegionellaHoursSinceDisinfectionSetter = false;
      snprintf(strBuf, sizeof(strBuf), "%u", m_iLegionellaHoursSinceDisinfection);
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_HOURS_SINCE_DISINFECTION "/set", strBuf);
    }

    // Publish state?
    if (m_bPublishLegionellaHoursSinceDisinfection || bForce)
    {
      m_bPublishLegionellaHoursSinceDisinfection = false;
      snprintf(strBuf, sizeof(strBuf), "%u", m_iLegionellaHoursSinceDisinfection);
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_HOURS_SINCE_DISINFECTION, strBuf);
    }

    // Publish setter? Note: Only when changed else we'll loop on setting it over and over again via broker
    if (m_bPublishLegionellaDangerZoneHoursSetter)
    {
      m_bPublishLegionellaDangerZoneHoursSetter = false;
      snprintf(strBuf, sizeof(strBuf), "%u", m_iLegionellaDangerZoneHours);
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DANGER_ZONE_HOURS "/set", strBuf);
    }

    // Publish state?
    if (m_bPublishLegionellaDangerZoneHours || bForce)
    {
      m_bPublishLegionellaDangerZoneHours = false;
      snprintf(strBuf, sizeof(strBuf), "%u", m_iLegionellaDangerZoneHours);
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DANGER_ZONE_HOURS, strBuf);
    }

    if (m_bPublishPowerGoodFlag || bForce)
    {
      // Note: Clear before GetPowerGoodFlag() since it may set it again when the flag is restored
      m_bPublishPowerGoodFlag = false;
      m_network.GetMqttClient().PublishMessage(MQTT_MAINS_ERROR, GetPowerGoodFlag() ? "0" : "1");
    }

    if (m_bPublishMainsValues || bForce)
    {
      m_bPublishMainsValues = false;

      // NOTE: Actual period is *2 since what we detect is rectified 50 Hz
      snprintf(strBuf, sizeof(strBuf), "%u", (m_iPeriodTime == NET_PERIOD_INVALID) ? 0 : m_iPeriodTime * 2);
      m_network.GetMqttClient().PublishMessage(MQTT_MAINS_PERIOD, strBuf);

      snprintf(strBuf, sizeof(strBuf), "%.2f", (500.0f * 1000.0f) / m_iPeriodTime);
      m_network.GetMqttClient().PublishMessage(MQTT_MAINS_FREQUENCY, strBuf);

      snprintf(strBuf, sizeof(strBuf), "%u", (m_iZeroCrossWindow == ZERO_CROSS_WINDOW_INVALID) ? 0 : m_iZeroCrossWindow);
      m_network.GetMqttClient().PublishMessage(MQTT_MAINS_ZERO_CROSS_WINDOW, strBuf);
    }

    // Publish uptime. Note: Always published since it changes every interval
    const CUptime::uptime_t upTime = GetUpTime();
    snprintf(strBuf, sizeof(strBuf), "%ud %02u:%02u:%02u", upTime.iDays, upTime.iHours, upTime.iMinutes, upTime.iSeconds);
    m_network.GetMqttClient().PublishMessage(MQTT_UP_TIME, strBuf);
  }

  return true;
}


void CPvBoiler::MqttPublishConfig()
{
  // Publish MQTT config for eg. HA discovery and subscribe to control topics
  m_network.GetMqttClient().PublishBinarySensorConfig(MQTT_MAINS_ERROR, true);

  m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_BUDGET, 1.0f, -100000.0f, 100000.0f, "W", "power");
  m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_PERCENTAGE, 1.0f, 0.0f, 100.0f, "%", "", false);
  m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_TEMPERATURE_OVERRIDE, 1.0f, TEMPERATURE_OVERRIDE_MIN, TEMPERATURE_OVERRIDE_MAX, "°C", "temperature", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_ERROR_GAIN_POS, "", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_ERROR_GAIN_NEG, "", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_STEP_CLAMP_POS, "%", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_STEP_CLAMP_NEG, "%", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_DEAD_ZONE, "W", "power", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_BUDGET_MARGIN, "W", "power", "", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_POWER, "W", "power");
  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_PERCENTAGE, "%", "");

  static const char* strSelectValues[] = { "Percentage", "Budget", "Off", "On", "Boost" };
  m_network.GetMqttClient().PublishSelectConfig(MQTT_SET_MODE, strSelectValues, 5);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_BOILER_POWER_RATING, "W", "power", "", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_DIM_STYLE, "", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_SSR_PERIOD_COUNT, "", "", "", true);

  // Diagnostic
  m_network.GetMqttClient().PublishSensorConfig(MQTT_WIFI_SSID, "", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_IP_ADDRESS, "", "", "", true);
//  m_network.GetMqttClient().PublishSensorConfig(MQTT_IP_NETMASK, "", "", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_TRIAC_PHASE_ANGLE, "us", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_TRIAC_ANGLE_FACTOR, "", "", "measurement", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_MAINS_PERIOD, "us", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_MAINS_FREQUENCY, "Hz", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_MAINS_ZERO_CROSS_WINDOW, "us", "", "", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_BOILER_TEMPERATURE, "°C", "temperature", "", true);
  m_network.GetMqttClient().PublishBinarySensorConfig(MQTT_BOILER_OVERHEATED, true);
  m_network.GetMqttClient().PublishBinarySensorConfig(MQTT_LEGIONELLA_MUST_DISINFECT, true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_LEGIONELLA_DISINFECT_RUN_SECONDS, "s", "", "", true);

  m_network.GetMqttClient().PublishNumberConfig(MQTT_LEGIONELLA_HOURS_SINCE_DISINFECTION, 1.0f, 0.0f, 10000000.0f);
  m_network.GetMqttClient().PublishNumberConfig(MQTT_LEGIONELLA_DANGER_ZONE_HOURS, 1.0f, 0.0f, 10000000.0f);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_UPDATE_INTERVAL, "s", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_NET_WD_TIMEOUT, "s", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_NET_WD_RECOVERY, "s", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_UP_TIME, "", "", "", true);

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

  EEPROM.get(EEPROM_BUDGET_MARGIN, iVal16);
  if (iVal16 < BUDGET_MARGIN_MIN || iVal16 > BUDGET_MARGIN_MAX)
  {
    iVal16 = BUDGET_MARGIN_DEFAULT;
  }
  m_iBudgetMargin = iVal16;

  EEPROM.get(EEPROM_CTRL_MODE, iVal8);
  m_mode = (iVal8 == 0x01) ? CPvBoiler::MODE_PERCENT : CPvBoiler::MODE_BUDGET;

  EEPROM.get(EEPROM_DIM_STYLE, iVal8);
  m_dimStyle = (iVal8 == 0x01) ? CPvBoiler::DIM_STYLE_SSR : CPvBoiler::DIM_STYLE_PHASE_ANGLE;

  EEPROM.get(EEPROM_SSR_PERIOD, iVal8);
  if (iVal8 < 2 || iVal8 > SSR_PERIOD_COUNT_MAX)
  {
    iVal8 = SSR_PERIOD_COUNT_DEFAULT;
  }
  m_iSsrPeriodCount = iVal8;

  EEPROM.get(EEPROM_ERROR_GAIN_POS, fVal);
  if (fVal < ERROR_GAIN_POS_MIN || fVal > ERROR_GAIN_POS_MAX || isnan(fVal))
  {
    fVal = ERROR_GAIN_POS_DEFAULT;
  }
  m_fErrorGainPos = fVal;

  EEPROM.get(EEPROM_ERROR_GAIN_NEG, fVal);
  if (fVal < ERROR_GAIN_NEG_MIN || fVal > ERROR_GAIN_NEG_MAX || isnan(fVal))
  {
    fVal = ERROR_GAIN_NEG_DEFAULT;
  }
  m_fErrorGainNeg = fVal;

  EEPROM.get(EEPROM_STEP_CLAMP_POS, fVal);
  if (fVal < STEP_CLAMP_POS_MIN || fVal > STEP_CLAMP_POS_MAX || isnan(fVal))
  {
    fVal = STEP_CLAMP_POS_DEFAULT;
  }
  m_fStepClampPos = fVal;

  EEPROM.get(EEPROM_STEP_CLAMP_NEG, fVal);
  if (fVal < STEP_CLAMP_NEG_MIN || fVal > STEP_CLAMP_NEG_MAX || isnan(fVal))
  {
    fVal = STEP_CLAMP_NEG_DEFAULT;
  }
  m_fStepClampNeg = fVal;

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

  EEPROM.get(EEPROM_MQTT_INTERVAL, m_iMqttUpdateInterval);
  if (m_iMqttUpdateInterval < MQTT_UPDATE_TIME_MIN || m_iMqttUpdateInterval > MQTT_UPDATE_TIME_MAX)
  {
    m_iMqttUpdateInterval = MQTT_UPDATE_TIME_DEFAULT;
  }

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


void CPvBoiler::SetBudgetMargin(const uint16_t iMargin)
{
  if (iMargin != m_iBudgetMargin)
  {
    EEPROM.put(EEPROM_BUDGET_MARGIN, iMargin);
    EEPROM.commit();

    m_iBudgetMargin = iMargin;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetMode(const CPvBoiler::mode_t mode)
{
  if (mode != m_mode)
  {
    if (mode != CPvBoiler::MODE_BOOST)
    {
      EEPROM.put(EEPROM_CTRL_MODE, mode);
      EEPROM.commit();
    }

    m_mode = mode;

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


void CPvBoiler::SetErrorGainPos(const float fGain)
{
  if (fGain != m_fErrorGainPos)
  {
    EEPROM.put(EEPROM_ERROR_GAIN_POS, fGain);
    EEPROM.commit();

    m_fErrorGainPos = fGain;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetErrorGainNeg(const float fGain)
{
  if (fGain != m_fErrorGainNeg)
  {
    EEPROM.put(EEPROM_ERROR_GAIN_NEG, fGain);
    EEPROM.commit();

    m_fErrorGainNeg = fGain;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetStepClampPos(const float fClamp)
{
  if (fClamp != m_fStepClampPos)
  {
    EEPROM.put(EEPROM_STEP_CLAMP_POS, fClamp);
    EEPROM.commit();

    m_fStepClampPos = fClamp;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetStepClampNeg(const float fClamp)
{
  if (fClamp != m_fStepClampNeg)
  {
    EEPROM.put(EEPROM_STEP_CLAMP_NEG, fClamp);
    EEPROM.commit();

    m_fStepClampNeg = fClamp;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetMqttUpdateInterval(const uint8_t iInterval)
{
  if (iInterval != m_iMqttUpdateInterval)
  {
    EEPROM.put(EEPROM_MQTT_INTERVAL, iInterval);
    EEPROM.commit();

    m_iMqttUpdateInterval = iInterval;
  
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
  SetBudgetMargin(BUDGET_MARGIN_DEFAULT);
  SetMode(MODE_BUDGET);
  SetDimStyle(DIM_STYLE_PHASE_ANGLE);
  SetSsrPeriodCount(SSR_PERIOD_COUNT_DEFAULT);
  SetErrorGainPos(ERROR_GAIN_POS_DEFAULT);
  SetErrorGainNeg(ERROR_GAIN_NEG_DEFAULT);
  SetStepClampPos(STEP_CLAMP_POS_DEFAULT);
  SetStepClampNeg(STEP_CLAMP_NEG_DEFAULT);
  SetNetWatchDogTimeout(NETWORK_WATCHDOG_TIMEOUT_DEFAULT);
  SetNetWatchDogRecovery(NETWORK_WATCHDOG_RECOVERY_DEFAULT);

  // Reset controller
  Reset();
}


void CPvBoiler::SetPowerGood(const bool bPowerGood)
{
  m_bPowerGood = bPowerGood;

  // Latch power failure until reported
  if (!bPowerGood && m_bPowerGoodFlag)
  {
    m_bPowerGoodFlag = false;
    m_bPublishPowerGoodFlag = true;
  }
}


bool CPvBoiler::GetPowerGoodFlag()
{
  const bool bPowerGoodFlag = m_bPowerGoodFlag;

  // Reading the flag restores it to the current state
  if (m_bPowerGoodFlag != m_bPowerGood)
  {
    m_bPowerGoodFlag = m_bPowerGood;
    m_bPublishPowerGoodFlag = true;
  }

  return bPowerGoodFlag;
}


uint16_t CPvBoiler::CalculateTriacPhaseDelay(const uint16_t iPeriodTime, const uint16_t iZeroCrossWindow)
{
  if (m_iPeriodTime != iPeriodTime || m_iZeroCrossWindow != iZeroCrossWindow)
  {
    m_iPeriodTime = iPeriodTime;
    m_iZeroCrossWindow = iZeroCrossWindow;
    m_bPublishMainsValues = true;
  }

  if (iPeriodTime > NET_PERIOD_MAX_US || iZeroCrossWindow > ZERO_CROSS_WINDOW_MAX_US)
  {
    return 0; // Unable to calculate value
  }

  uint32_t iDelay = 0;
  if (m_dimStyle == CPvBoiler::DIM_STYLE_SSR)
  {
    m_fTriacAngleFactor = 0.0f;
    m_iTriacPhaseAngle = TRIAC_PHASE_ANGLE_MARGIN_US;
    iDelay = TRIAC_PHASE_ANGLE_MARGIN_US;
  }
  else if (m_dimStyle == CPvBoiler::DIM_STYLE_PHASE_ANGLE)
  {
    // Update triac angle factor
    const float fTriacAngleFactor = triac_percentage_factor[static_cast<uint8_t>(m_fCurrentPercentage)];
    if (fTriacAngleFactor != m_fTriacAngleFactor)
    {
      m_fTriacAngleFactor = fTriacAngleFactor;
      m_bPublishOutputPercentage = true; // Make sure (MQTT) published factor matches current percentage
    }

    // Make sure we trigger not too close to zero cross
    m_iTriacPhaseAngle = max(static_cast<uint32_t>(m_fTriacAngleFactor * iPeriodTime), static_cast<uint32_t>(TRIAC_PHASE_ANGLE_MARGIN_US));

    // NOTE: Only turn on triac when NOT near 0% to prevent excessive EMI due to misfiring (eg. caused by WiFi latency)
    if (m_iTriacPhaseAngle + TRIAC_PHASE_ANGLE_MARGIN_US + GATE_PULSE_WIDTH <= iPeriodTime)
    {
      iDelay = m_iTriacPhaseAngle;
    }

    // With zero delay return zero so we know we should do "nothing"
    if (iDelay == 0)
    {
      return 0;
    }
  }
  else // DIM_STYLE_NONE
  {
    m_fTriacAngleFactor = 0.0f;
    m_iTriacPhaseAngle = 0;
    return 0;
  }

  // Return phase angle including zero cross window compensation
  return iDelay + (iZeroCrossWindow / 2);
}


void CPvBoiler::Update()
{
  float fNewPercentage = m_fCurrentPercentage;

  if (m_bBoilerOverHeated)
  {
    fNewPercentage = 0.0f;
  }
  else if (m_bLegionellaMustDisinfect)
  {
    fNewPercentage = 100.0f;
  }
  else if (m_iNetworkWatchdogRecoveryCounter > 0 || m_mode == MODE_OFF)
  {
    if (m_mode == MODE_OFF)
    {
      m_iNetworkWatchdogRecoveryCounter = 0; // When off: quick recovery
    }

    if (m_fCurrentPercentage > 0)
    {
      fNewPercentage -= m_fStepClampNeg; // Device off or watch-dog triggered: output to 0%
    }
  }
  else if (m_mode == MODE_BOOST || m_mode == MODE_ON)
  {
    fNewPercentage = 100.0f; // Immediately 100% power
  }
  else if (m_mode == MODE_PERCENT)
  {
    fNewPercentage = m_iPowerPercentage;
  }
  else
  {
    // Calculate error step (percentage)
    float fErrorStep = (100.0f * (m_iPowerBudget - m_iBudgetMargin)) / m_iBoilerPowerRating;
    fErrorStep *= (fErrorStep > 0.0f) ? m_fErrorGainPos : m_fErrorGainNeg;

    // Clamp error (step) value
    if (fErrorStep > m_fStepClampPos)
    {
      fErrorStep = m_fStepClampPos;
    }
    else if (fErrorStep < -m_fStepClampNeg)
    {
      fErrorStep = -m_fStepClampNeg;
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

  // Software thermostat. When set to TEMPERATURE_OVERRIDE_MAX or when no temperature sensor
  // is installed it's disabled and the hardware thermostat takes over
  if (m_tempSensors.getDeviceCount() > 0 && m_mode != MODE_BOOST &&
      m_iTemperatureOverride >= TEMPERATURE_OVERRIDE_MIN && m_iTemperatureOverride < TEMPERATURE_OVERRIDE_MAX)
  {
    // During legionella disinfection raise the setpoint so the disinfect temperature can be reached and held
    uint8_t iSetpoint = m_iTemperatureOverride;
    if (m_bLegionellaMustDisinfect && iSetpoint < TEMPERATURE_DISINFECT_SETPOINT)
    {
      iSetpoint = TEMPERATURE_DISINFECT_SETPOINT;
    }

    if (m_fBoilerTemperature <= 0.0f)
    {
      m_bThermostatAllowsHeating = false; // Sensor installed but no valid reading: fail safe
    }
    else if (m_fBoilerTemperature >= iSetpoint)
    {
      m_bThermostatAllowsHeating = false;
    }
    else if (m_fBoilerTemperature <= iSetpoint - TEMPERATURE_OVERRIDE_HYSTERESIS)
    {
      m_bThermostatAllowsHeating = true;
    }
  }
  else
  {
    m_bThermostatAllowsHeating = true;
  }

  if (!m_bThermostatAllowsHeating)
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
