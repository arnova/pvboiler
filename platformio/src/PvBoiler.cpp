#include "PvBoiler.h"
#include "util.h"

#include <EEPROM.h>

// Out-of-class definition required pre-C++17 (ESP32 core builds with gnu++11), redundant but harmless with C++17
constexpr const char* CPvBoiler::m_strModeNames[];

CPvBoiler::CPvBoiler(CNetwork& network) : m_network(network), m_oneWire(ONE_WIRE), m_tempSensors(&m_oneWire)
{
  m_tempSensors.begin();
  m_tempSensors.setResolution(9);
  m_tempSensors.setWaitForConversion(false); // Do not block but poll
  m_tempSensors.requestTemperatures(); // Request first temperature

  m_boilerTemperatureAverage.SetAvgCount(30);
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

        // Over temperature protection. Use averaged value so a single bad reading can't latch it
        if (fTemperatureAveraged >= TEMPERATURE_OVERHEATING_MAX && !m_bBoilerOverHeated)
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
    const bool bLegionellaDisinfectionRequired = m_legionella.DisinfectionRequired();
    if (m_bLegionellaDisinfectionRequired != bLegionellaDisinfectionRequired)
    {
      m_bLegionellaDisinfectionRequired = bLegionellaDisinfectionRequired;
      m_bPublishLegionellaDisinfectionRequired = true;
    }

    const uint32_t iDisinfectingElapsedMinutes = m_legionella.GetDisinfectingElapsedMinutes();
    if (m_iLegionellaDisinfectingElapsedMinutes != iDisinfectingElapsedMinutes)
    {
      m_iLegionellaDisinfectingElapsedMinutes = iDisinfectingElapsedMinutes;
      m_bPublishLegionellaDisinfectingElapsedMinutes = true;
    }

    // Publish new setter value only when changed else we'll loop on setting it over and over again via broker
    const uint32_t iLegionellaDisinfectionAgeHours = m_legionella.GetDisinfectionAgeHours();
    if (m_iLegionellaDisinfectionAgeHours != iLegionellaDisinfectionAgeHours)
    {
      m_iLegionellaDisinfectionAgeHours = iLegionellaDisinfectionAgeHours;
      m_bPublishLegionellaDisinfectionAgeHoursSetter = true;
    }

    // Publish new setter value only when changed else we'll loop on setting it over and over again via broker
    const uint32_t iLegionellaInDangerZoneHours = m_legionella.GetInDangerZoneHours();
    if (m_iLegionellaInDangerZoneHours != iLegionellaInDangerZoneHours)
    {
      m_iLegionellaInDangerZoneHours = iLegionellaInDangerZoneHours;
      m_bPublishLegionellaInDangerZoneHoursSetter = true;
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
  m_iPowerDelivered = 0;
  m_iPowerReturned = 0;
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

  // Re-detect temperature sensor(s) (eg. when connected after boot)
  m_tempSensors.begin();
  m_tempSensors.requestTemperatures();

  m_fBoilerTemperature = -1.0f;
  m_bPublishBoilerTemperature = true;
  m_boilerTemperatureAverage.Reset();
  m_iBoilerTemperatureRetryCount = 0;
  m_legionella.UpdateTemperature(m_fBoilerTemperature);
  m_bBoilerOverHeated = false;
  m_bPublishBoilerOverHeated = true;

  m_bPublishThermostatSetpoint = true; // Value itself is (re)loaded by LoadSettings()
  m_bThermostatAllowsHeating = true;
  m_bPublishThermostatAllowsHeating = true;

  m_iLegionellaDisinfectionAgeHours = UINT32_MAX;
  m_bPublishLegionellaDisinfectionAgeHours = true;
  m_bPublishLegionellaDisinfectionAgeHoursSetter = false;

  m_iLegionellaInDangerZoneHours = UINT32_MAX;
  m_bPublishLegionellaInDangerZoneHours = true;
  m_bPublishLegionellaInDangerZoneHoursSetter = false;

  m_iLegionellaDisinfectingElapsedMinutes = 0;
  m_bPublishLegionellaDisinfectingElapsedMinutes = true;

  m_bLegionellaDisinfectionRequired = false;
  m_bPublishLegionellaDisinfectionRequired = true;

  LoadSettings();
}


void CPvBoiler::SetLegionellaDisinfectionAgeHours(const uint32_t iVal)
{
  // May be set from both the mqtt broker (retained message) or
  // from ourselves when the value changes in the CLegionella-class
  // We know that when value differs from what we last published it was not set by ourselves
  // and it's the retained message from the broker (or set by the user).
  // NOTE: Do not compare with the current value as our own echo may arrive after it changed
  // (eg. directly after (re)connect), which would reset the restored value
  if (iVal != m_iLegionellaDisinfectionAgeHoursSetter)
  {
    // Be pessemistic(+1) about the retained value from the broker due to rounding (down) errors
    m_legionella.SetDisinfectionAgeHours(iVal + 1);
  }

  m_bPublishLegionellaDisinfectionAgeHours = true;
}


void CPvBoiler::SetLegionellaInDangerZoneHours(const uint32_t iVal)
{
  // May be set from both the mqtt broker (retained message) or
  // from ourselves when the value changes in the CLegionella-class
  // We know that when value differs from what we last published it was not set by ourselves
  // and it's the retained message from the broker (or set by the user).
  // NOTE: Do not compare with the current value as our own echo may arrive after it changed
  // (eg. directly after (re)connect), which would reset the restored value
  if (iVal != m_iLegionellaInDangerZoneHoursSetter)
  {
    // Be pessemistic(+1) about the retained value from the broker due to rounding (down) errors
    m_legionella.SetInDangerZoneHours(iVal + 1);
  }

  m_bPublishLegionellaInDangerZoneHours = true;
}


bool CPvBoiler::MqttPublishValues()
{
  if (!m_network.IsMqttConnected())
    return false;

  char strBuf[24]; // Enough room for signed/unsigned 32 bit number or our floats with 4 digit precision

  // Publish our f/w version. Note: Static so only published on request
  if (m_bMqttRepublish)
  {
    m_network.GetMqttClient().PublishMessage(MQTT_FW_VERSION, MY_VERSION);
  }

  if (m_bPublishSettings || m_bMqttRepublish)
  {
    m_bPublishSettings = false;

    snprintf(strBuf, sizeof(strBuf), "%u", m_iBoilerPowerRating);
    m_network.GetMqttClient().PublishMessage(MQTT_BOILER_POWER_RATING, strBuf);

    m_network.GetMqttClient().PublishMessage(MQTT_SET_MODE, m_strModeNames[m_mode]);

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

    m_network.GetMqttClient().PublishMessage(MQTT_DIM_STYLE, (m_dimStyle == DIM_STYLE_SSR) ? "SSR" : "Phase-angle");

    snprintf(strBuf, sizeof(strBuf), "%u", m_iSsrPeriodCount);
    m_network.GetMqttClient().PublishMessage(MQTT_SSR_PERIOD_COUNT, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iMqttUpdateInterval);
    m_network.GetMqttClient().PublishMessage(MQTT_UPDATE_INTERVAL, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iNetWatchDogTimeout);
    m_network.GetMqttClient().PublishMessage(MQTT_NET_WD_TIMEOUT, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iNetWatchDogRecovery);
    m_network.GetMqttClient().PublishMessage(MQTT_NET_WD_RECOVERY, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iThermostatHysteresis);
    m_network.GetMqttClient().PublishMessage(MQTT_THERMOSTAT_HYSTERESIS, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iDangerZoneLowTemperature);
    m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DANGER_ZONE_LOW_TEMP, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iDangerZoneHighTemperature);
    m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DANGER_ZONE_HIGH_TEMP, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iDisinfectionIntervalDays);
    m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DISINFECTION_INTERVAL, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iDisinfectionDurationMinutes);
    m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DISINFECTION_DURATION, strBuf);

    snprintf(strBuf, sizeof(strBuf), "%u", m_iDisinfectionTemperature);
    m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DISINFECTION_TEMPERATURE, strBuf);
  }

  // Publish these MQTT values (if any) when timer expires (and connected)
  if (m_mqttPublishTimer > m_iMqttUpdateInterval * 1000 || m_bMqttRepublish)
  {
    m_mqttPublishTimer = 0;

    if (m_bPublishPowerBudget || m_bMqttRepublish)
    {
      m_bPublishPowerBudget = false;

      snprintf(strBuf, sizeof(strBuf), "%d", m_iPowerBudget);
      m_network.GetMqttClient().PublishMessage(MQTT_SET_POWER_BUDGET, strBuf);
    }

    if (m_bPublishPowerPercentage || m_bMqttRepublish)
    {
      m_bPublishPowerPercentage = false;

      snprintf(strBuf, sizeof(strBuf), "%u", m_iPowerPercentage);
      m_network.GetMqttClient().PublishMessage(MQTT_SET_POWER_PERCENTAGE, strBuf);
    }

    if (m_bPublishThermostatSetpoint || m_bMqttRepublish)
    {
      m_bPublishThermostatSetpoint = false;

      snprintf(strBuf, sizeof(strBuf), "%u", m_iThermostatSetpoint);
      m_network.GetMqttClient().PublishMessage(MQTT_SET_THERMOSTAT_SETPOINT, strBuf);
    }

    if (m_bPublishOutputPercentage || m_bMqttRepublish)
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

    if (m_bPublishBoilerTemperature || m_bMqttRepublish)
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

    if (m_bPublishBoilerOverHeated || m_bMqttRepublish)
    {
      m_bPublishBoilerOverHeated = false;
      m_network.GetMqttClient().PublishMessage(MQTT_BOILER_OVERHEATED, m_bBoilerOverHeated ? "1" : "0");
    }

    if (m_bPublishThermostatAllowsHeating || m_bMqttRepublish)
    {
      m_bPublishThermostatAllowsHeating = false;
      m_network.GetMqttClient().PublishMessage(MQTT_THERMOSTAT_ALLOWS_HEATING, m_bThermostatAllowsHeating ? "1" : "0");
    }

    if (m_bPublishDisinfectionEnabled || m_bMqttRepublish)
    {
      m_bPublishDisinfectionEnabled = false;
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DISINFECTION_ENABLED, m_bDisinfectionEnabled ? "1" : "0");
    }

    if (m_bPublishLegionellaDisinfectionRequired || m_bMqttRepublish)
    {
      m_bPublishLegionellaDisinfectionRequired = false;
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DISINFECTION_REQUIRED, m_bLegionellaDisinfectionRequired ? "1" : "0");
    }

    if (m_bPublishLegionellaDisinfectingElapsedMinutes || m_bMqttRepublish)
    {
      m_bPublishLegionellaDisinfectingElapsedMinutes = false;
      snprintf(strBuf, sizeof(strBuf), "%u", m_iLegionellaDisinfectingElapsedMinutes);
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DISINFECTING_ELAPSED, strBuf);
    }

    // Hold off publishing setters directly after subscribing so the (retained) broker values are
    // received first. Else we'd overwrite them with our own (reset) values
    const bool bSetterHoldOff = (m_mqttSetterHoldOffTimer < MQTT_SETTLE_TIME_MS);

    // Publish setter? Note: Only when changed else we'll loop on setting it over and over again via broker
    if (m_bPublishLegionellaDisinfectionAgeHoursSetter && !bSetterHoldOff)
    {
      m_bPublishLegionellaDisinfectionAgeHoursSetter = false;
      m_iLegionellaDisinfectionAgeHoursSetter = m_iLegionellaDisinfectionAgeHours;
      snprintf(strBuf, sizeof(strBuf), "%u", m_iLegionellaDisinfectionAgeHours);
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DISINFECTION_AGE "/set", strBuf, true); // Retain: restored from broker after reboot
    }

    // Publish state?
    if (m_bPublishLegionellaDisinfectionAgeHours || m_bMqttRepublish)
    {
      m_bPublishLegionellaDisinfectionAgeHours = false;
      snprintf(strBuf, sizeof(strBuf), "%u", m_iLegionellaDisinfectionAgeHours);
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_DISINFECTION_AGE, strBuf);
    }

    // Publish setter? Note: Only when changed else we'll loop on setting it over and over again via broker
    if (m_bPublishLegionellaInDangerZoneHoursSetter && !bSetterHoldOff)
    {
      m_bPublishLegionellaInDangerZoneHoursSetter = false;
      m_iLegionellaInDangerZoneHoursSetter = m_iLegionellaInDangerZoneHours;
      snprintf(strBuf, sizeof(strBuf), "%u", m_iLegionellaInDangerZoneHours);
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_IN_DANGER_ZONE "/set", strBuf, true); // Retain: restored from broker after reboot
    }

    // Publish state?
    if (m_bPublishLegionellaInDangerZoneHours || m_bMqttRepublish)
    {
      m_bPublishLegionellaInDangerZoneHours = false;
      snprintf(strBuf, sizeof(strBuf), "%u", m_iLegionellaInDangerZoneHours);
      m_network.GetMqttClient().PublishMessage(MQTT_LEGIONELLA_IN_DANGER_ZONE, strBuf);
    }

    if (m_bPublishPowerGoodFlag || m_bMqttRepublish)
    {
      // Note: Clear before GetPowerGoodFlag() since it may set it again when the flag is restored
      m_bPublishPowerGoodFlag = false;
      m_network.GetMqttClient().PublishMessage(MQTT_MAINS_ERROR, GetPowerGoodFlag() ? "0" : "1");
    }

    if (m_bPublishMainsValues || m_bMqttRepublish)
    {
      m_bPublishMainsValues = false;

      if (m_iPeriodTime != 0 && m_iPeriodTime != NET_PERIOD_INVALID)
      {
        // NOTE: Actual period is *2 since what we detect is rectified 50 Hz
        snprintf(strBuf, sizeof(strBuf), "%u", m_iPeriodTime * 2);
        m_network.GetMqttClient().PublishMessage(MQTT_MAINS_PERIOD, strBuf);

        snprintf(strBuf, sizeof(strBuf), "%.2f", (500.0f * 1000.0f) / m_iPeriodTime);
        m_network.GetMqttClient().PublishMessage(MQTT_MAINS_FREQUENCY, strBuf);
      }
      else
      {
        m_network.GetMqttClient().PublishMessage(MQTT_MAINS_PERIOD, "None"); // Home Assistant: state "unknown"
        m_network.GetMqttClient().PublishMessage(MQTT_MAINS_FREQUENCY, "None");
      }

      if (m_iZeroCrossWindow != 0 && m_iZeroCrossWindow != ZERO_CROSS_WINDOW_INVALID)
      {
        snprintf(strBuf, sizeof(strBuf), "%u", m_iZeroCrossWindow);
        m_network.GetMqttClient().PublishMessage(MQTT_MAINS_ZERO_CROSS_WINDOW, strBuf);
      }
      else
      {
        m_network.GetMqttClient().PublishMessage(MQTT_MAINS_ZERO_CROSS_WINDOW, "None"); // Home Assistant: state "unknown"
      }
    }

    // Publish uptime. Note: Always published since it changes every interval
    const CUptime::uptime_t upTime = GetUpTime();
    snprintf(strBuf, sizeof(strBuf), "%ud %02u:%02u:%02u", upTime.iDays, upTime.iHours, upTime.iMinutes, upTime.iSeconds);
    m_network.GetMqttClient().PublishMessage(MQTT_UP_TIME, strBuf);
  }

  m_bMqttRepublish = false;

  return true;
}


void CPvBoiler::MqttUpdateDsmrSubscription()
{
  // Get current grid power from DSMR-reader (used as power budget). Note: Only subscribe when used,
  // else its messages would keep feeding the network watchdog
  if (m_bDsmrPowerBudget)
  {
    m_network.GetMqttClient().subscribe(MQTT_DSMR_ELECTRICITY_DELIVERED, 1);
    m_network.GetMqttClient().subscribe(MQTT_DSMR_ELECTRICITY_RETURNED, 1);
  }
  else
  {
    m_network.GetMqttClient().unsubscribe(MQTT_DSMR_ELECTRICITY_DELIVERED);
    m_network.GetMqttClient().unsubscribe(MQTT_DSMR_ELECTRICITY_RETURNED);
  }
}


void CPvBoiler::MqttPublishDiscovery()
{
  // (Re)subscribing makes the broker resend retained setter values: hold off publishing our own
  m_mqttSetterHoldOffTimer = 0;

  // Publish HA discovery config and subscribe to control (setter) topics
  m_network.GetMqttClient().PublishBinarySensorConfig(MQTT_MAINS_ERROR, true);

  // Note: Don't retain power budget: a stale (retained) budget would be applied after (re)connect and also feed the network watchdog
  m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_BUDGET, 1.0f, -100000.0f, 100000.0f, "W", "power", true, false);
  m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_POWER_PERCENTAGE, 1.0f, 0.0f, 100.0f, "%", "", false);
  m_network.GetMqttClient().PublishNumberConfig(MQTT_SET_THERMOSTAT_SETPOINT, 1.0f, THERMOSTAT_SETPOINT_MIN, THERMOSTAT_SETPOINT_MAX, "°C", "temperature", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_ERROR_GAIN_POS, "", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_ERROR_GAIN_NEG, "", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_STEP_CLAMP_POS, "%", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_STEP_CLAMP_NEG, "%", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_DEAD_ZONE, "W", "power", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_BUDGET_MARGIN, "W", "power", "", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_POWER, "W", "power");
  m_network.GetMqttClient().PublishSensorConfig(MQTT_OUTPUT_PERCENTAGE, "%", "");

  m_network.GetMqttClient().PublishSelectConfig(MQTT_SET_MODE, m_strModeNames, MODE_COUNT);

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

  m_network.GetMqttClient().PublishSensorConfig(MQTT_BOILER_TEMPERATURE, "°C", "temperature", "");
  m_network.GetMqttClient().PublishBinarySensorConfig(MQTT_BOILER_OVERHEATED, true);
  m_network.GetMqttClient().PublishBinarySensorConfig(MQTT_THERMOSTAT_ALLOWS_HEATING, true);
  m_network.GetMqttClient().PublishBinarySensorConfig(MQTT_LEGIONELLA_DISINFECTION_ENABLED, true);
  m_network.GetMqttClient().PublishBinarySensorConfig(MQTT_LEGIONELLA_DISINFECTION_REQUIRED);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_LEGIONELLA_DISINFECTING_ELAPSED, "min", "duration", "", true);

  m_network.GetMqttClient().PublishNumberConfig(MQTT_LEGIONELLA_DISINFECTION_AGE, 1.0f, 0.0f, LEGIONELLA_HOURS_MAX, "h");
  m_network.GetMqttClient().PublishNumberConfig(MQTT_LEGIONELLA_IN_DANGER_ZONE, 1.0f, 0.0f, LEGIONELLA_HOURS_MAX, "h");

  m_network.GetMqttClient().PublishSensorConfig(MQTT_THERMOSTAT_HYSTERESIS, "°C", "temperature", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_LEGIONELLA_DANGER_ZONE_LOW_TEMP, "°C", "temperature", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_LEGIONELLA_DANGER_ZONE_HIGH_TEMP, "°C", "temperature", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_LEGIONELLA_DISINFECTION_INTERVAL, "d", "duration", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_LEGIONELLA_DISINFECTION_DURATION, "min", "duration", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_LEGIONELLA_DISINFECTION_TEMPERATURE, "°C", "temperature", "", true);

  m_network.GetMqttClient().PublishSensorConfig(MQTT_UPDATE_INTERVAL, "s", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_NET_WD_TIMEOUT, "s", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_NET_WD_RECOVERY, "s", "", "", true);
  m_network.GetMqttClient().PublishSensorConfig(MQTT_UP_TIME, "", "", "", true);
}


void CPvBoiler::MqttOnConnect()
{
  MqttPublishDiscovery();
  MqttUpdateDsmrSubscription();
}


void CPvBoiler::LoadSettings()
{
  uint8_t iVal8 = 0;
  uint16_t iVal16 = 0;
  float fVal = 0.0f;

  EEPROM.get(EEPROM_BP_RATING, iVal16);
  if (iVal16 < BOILER_POWER_RATING_MIN || iVal16 > BOILER_POWER_RATING_MAX)
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

  // Note: MODE_BOOST is never stored
  EEPROM.get(EEPROM_CTRL_MODE, iVal8);
  m_mode = (iVal8 < MODE_COUNT && iVal8 != CPvBoiler::MODE_BOOST) ? static_cast<CPvBoiler::mode_t>(iVal8) : CPvBoiler::MODE_BUDGET;

  EEPROM.get(EEPROM_DIM_STYLE, iVal8);
  m_dimStyle = (iVal8 == 0x01) ? CPvBoiler::DIM_STYLE_SSR : CPvBoiler::DIM_STYLE_PHASE_ANGLE;

  EEPROM.get(EEPROM_SSR_PERIOD, iVal8);
  if (iVal8 < SSR_PERIOD_COUNT_MIN || iVal8 > SSR_PERIOD_COUNT_MAX)
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

  EEPROM.get(EEPROM_THERMOSTAT_SP, iVal8);
  if (iVal8 < THERMOSTAT_SETPOINT_MIN || iVal8 > THERMOSTAT_SETPOINT_MAX)
  {
    iVal8 = THERMOSTAT_SETPOINT_DEFAULT;
  }
  m_iThermostatSetpoint = iVal8;
  m_bPublishThermostatSetpoint = true;

  // Note: Anything other than 0x00 (eg. erased EEPROM) means enabled
  EEPROM.get(EEPROM_DISINFECTION_EN, iVal8);
  m_bDisinfectionEnabled = (iVal8 != 0x00);
  m_bPublishDisinfectionEnabled = true;

  // Note: Anything other than 0x01 (eg. erased EEPROM) means direct
  EEPROM.get(EEPROM_BUDGET_SRC, iVal8);
  m_bDsmrPowerBudget = (iVal8 == 0x01);

  EEPROM.get(EEPROM_THERMOSTAT_HYST, iVal8);
  if (iVal8 < THERMOSTAT_HYSTERESIS_MIN || iVal8 > THERMOSTAT_HYSTERESIS_MAX)
  {
    iVal8 = THERMOSTAT_HYSTERESIS_DEFAULT;
  }
  m_iThermostatHysteresis = iVal8;

  uint8_t iLowTemp = 0;
  uint8_t iHighTemp = 0;
  EEPROM.get(EEPROM_DZ_LOW_TEMP, iLowTemp);
  EEPROM.get(EEPROM_DZ_HIGH_TEMP, iHighTemp);
  if (iLowTemp < DANGER_ZONE_TEMPERATURE_MIN || iHighTemp > DANGER_ZONE_TEMPERATURE_MAX || iLowTemp >= iHighTemp)
  {
    iLowTemp = DANGER_ZONE_LOW_TEMPERATURE_DEFAULT;
    iHighTemp = DANGER_ZONE_HIGH_TEMPERATURE_DEFAULT;
  }
  m_iDangerZoneLowTemperature = iLowTemp;
  m_legionella.SetDangerZoneLowTemperature(iLowTemp);
  m_iDangerZoneHighTemperature = iHighTemp;
  m_legionella.SetDangerZoneHighTemperature(iHighTemp);

  EEPROM.get(EEPROM_DISINFECTION_INTERVAL, iVal8);
  if (iVal8 < DISINFECTION_INTERVAL_DAYS_MIN || iVal8 > DISINFECTION_INTERVAL_DAYS_MAX)
  {
    iVal8 = DISINFECTION_INTERVAL_DAYS_DEFAULT;
  }
  m_iDisinfectionIntervalDays = iVal8;
  m_legionella.SetDisinfectionIntervalDays(iVal8);

  EEPROM.get(EEPROM_DISINFECTION_DURATION, iVal8);
  if (iVal8 < DISINFECTION_DURATION_MINUTES_MIN || iVal8 > DISINFECTION_DURATION_MINUTES_MAX)
  {
    iVal8 = DISINFECTION_DURATION_MINUTES_DEFAULT;
  }
  m_iDisinfectionDurationMinutes = iVal8;
  m_legionella.SetDisinfectionDurationMinutes(iVal8);

  EEPROM.get(EEPROM_DISINFECTION_TEMP, iVal8);
  if (iVal8 < DISINFECTION_TEMPERATURE_MIN || iVal8 > DISINFECTION_TEMPERATURE_MAX)
  {
    iVal8 = DISINFECTION_TEMPERATURE_DEFAULT;
  }
  m_iDisinfectionTemperature = iVal8;
  m_legionella.SetDisinfectionTemperature(iVal8);

  m_bPublishSettings = true;
}


bool CPvBoiler::StoreSetting(const int iAddr, uint8_t& iSetting, const uint8_t iVal)
{
  if (iVal == iSetting)
    return false;

  EEPROM.put(iAddr, iVal);
  EEPROM.commit();

  iSetting = iVal;

  return true;
}


bool CPvBoiler::StoreSetting(const int iAddr, uint16_t& iSetting, const uint16_t iVal)
{
  if (iVal == iSetting)
    return false;

  EEPROM.put(iAddr, iVal);
  EEPROM.commit();

  iSetting = iVal;

  return true;
}


bool CPvBoiler::StoreSetting(const int iAddr, float& fSetting, const float fVal)
{
  if (fVal == fSetting)
    return false;

  EEPROM.put(iAddr, fVal);
  EEPROM.commit();

  fSetting = fVal;

  return true;
}


bool CPvBoiler::StoreSetting(const int iAddr, bool& bSetting, const bool bVal)
{
  if (bVal == bSetting)
    return false;

  EEPROM.put(iAddr, static_cast<uint8_t>(bVal ? 0x01 : 0x00));
  EEPROM.commit();

  bSetting = bVal;

  return true;
}


void CPvBoiler::SetThermostatSetpoint(const uint8_t iTemperature)
{
  StoreSetting(EEPROM_THERMOSTAT_SP, m_iThermostatSetpoint, iTemperature);

  m_bPublishThermostatSetpoint = true;
}


void CPvBoiler::SetDisinfectionEnabled(const bool bEnable)
{
  StoreSetting(EEPROM_DISINFECTION_EN, m_bDisinfectionEnabled, bEnable);

  m_bPublishDisinfectionEnabled = true;
}


void CPvBoiler::SetDsmrPowerBudget(const bool bEnable)
{
  if (StoreSetting(EEPROM_BUDGET_SRC, m_bDsmrPowerBudget, bEnable) && m_network.IsMqttConnected())
  {
    MqttUpdateDsmrSubscription();
  }
}


void CPvBoiler::SetThermostatHysteresis(const uint8_t iHysteresis)
{
  if (StoreSetting(EEPROM_THERMOSTAT_HYST, m_iThermostatHysteresis, iHysteresis))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDangerZoneLowTemperature(const uint8_t iTemperature)
{
  if (StoreSetting(EEPROM_DZ_LOW_TEMP, m_iDangerZoneLowTemperature, iTemperature))
  {
    m_legionella.SetDangerZoneLowTemperature(iTemperature);

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDangerZoneHighTemperature(const uint8_t iTemperature)
{
  if (StoreSetting(EEPROM_DZ_HIGH_TEMP, m_iDangerZoneHighTemperature, iTemperature))
  {
    m_legionella.SetDangerZoneHighTemperature(iTemperature);

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDisinfectionIntervalDays(const uint8_t iDays)
{
  if (StoreSetting(EEPROM_DISINFECTION_INTERVAL, m_iDisinfectionIntervalDays, iDays))
  {
    m_legionella.SetDisinfectionIntervalDays(iDays);

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDisinfectionDurationMinutes(const uint8_t iMinutes)
{
  if (StoreSetting(EEPROM_DISINFECTION_DURATION, m_iDisinfectionDurationMinutes, iMinutes))
  {
    m_legionella.SetDisinfectionDurationMinutes(iMinutes);

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDisinfectionTemperature(const uint8_t iTemperature)
{
  if (StoreSetting(EEPROM_DISINFECTION_TEMP, m_iDisinfectionTemperature, iTemperature))
  {
    m_legionella.SetDisinfectionTemperature(iTemperature);

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetBoilerPowerRating(const uint16_t iPower)
{
  if (StoreSetting(EEPROM_BP_RATING, m_iBoilerPowerRating, iPower))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDeadZone(const uint8_t iDeadZone)
{
  if (StoreSetting(EEPROM_DEAD_ZONE, m_iDeadZone, iDeadZone))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetBudgetMargin(const uint16_t iMargin)
{
  if (StoreSetting(EEPROM_BUDGET_MARGIN, m_iBudgetMargin, iMargin))
  {
    m_bPublishSettings = true;
  }
}


bool CPvBoiler::GetModeFromName(const char* strName, CPvBoiler::mode_t& mode)
{
  for (uint8_t it = 0; it < MODE_COUNT; it++)
  {
    if (strcasecmp(strName, m_strModeNames[it]) == 0)
    {
      mode = static_cast<CPvBoiler::mode_t>(it);
      return true;
    }
  }

  return false;
}


void CPvBoiler::SetMode(const CPvBoiler::mode_t mode)
{
  if (mode != m_mode)
  {
    if (mode != CPvBoiler::MODE_BOOST)
    {
      EEPROM.put(EEPROM_CTRL_MODE, static_cast<uint8_t>(mode));
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
    EEPROM.put(EEPROM_DIM_STYLE, static_cast<uint8_t>((dimStyle == CPvBoiler::DIM_STYLE_SSR) ? 0x01 : 0x00));
    EEPROM.commit();

    m_dimStyle = dimStyle;

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetSsrPeriodCount(const uint8_t iCount)
{
  if (StoreSetting(EEPROM_SSR_PERIOD, m_iSsrPeriodCount, iCount))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetErrorGainPos(const float fGain)
{
  if (StoreSetting(EEPROM_ERROR_GAIN_POS, m_fErrorGainPos, fGain))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetErrorGainNeg(const float fGain)
{
  if (StoreSetting(EEPROM_ERROR_GAIN_NEG, m_fErrorGainNeg, fGain))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetStepClampPos(const float fClamp)
{
  if (StoreSetting(EEPROM_STEP_CLAMP_POS, m_fStepClampPos, fClamp))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetStepClampNeg(const float fClamp)
{
  if (StoreSetting(EEPROM_STEP_CLAMP_NEG, m_fStepClampNeg, fClamp))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetMqttUpdateInterval(const uint8_t iInterval)
{
  if (StoreSetting(EEPROM_MQTT_INTERVAL, m_iMqttUpdateInterval, iInterval))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetNetWatchDogTimeout(const uint16_t iTime)
{
  if (StoreSetting(EEPROM_NET_WD_TIMEOUT, m_iNetWatchDogTimeout, iTime))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetNetWatchDogRecovery(const uint16_t iTime)
{
  if (StoreSetting(EEPROM_NET_WD_RECOVER, m_iNetWatchDogRecovery, iTime))
  {
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
  SetMqttUpdateInterval(MQTT_UPDATE_TIME_DEFAULT);
  SetThermostatSetpoint(THERMOSTAT_SETPOINT_DEFAULT);
  SetDisinfectionEnabled(true);
  SetDsmrPowerBudget(false);
  SetThermostatHysteresis(THERMOSTAT_HYSTERESIS_DEFAULT);
  SetDangerZoneLowTemperature(DANGER_ZONE_LOW_TEMPERATURE_DEFAULT);
  SetDangerZoneHighTemperature(DANGER_ZONE_HIGH_TEMPERATURE_DEFAULT);
  SetDisinfectionIntervalDays(DISINFECTION_INTERVAL_DAYS_DEFAULT);
  SetDisinfectionDurationMinutes(DISINFECTION_DURATION_MINUTES_DEFAULT);
  SetDisinfectionTemperature(DISINFECTION_TEMPERATURE_DEFAULT);

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
    const float fTriacAngleFactor = triac_percentage_factor[lroundf(m_fCurrentPercentage)]; // Note: Percentage is clamped to 0-100
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
  const bool bLegionellaDisinfect = m_bLegionellaDisinfectionRequired && m_bDisinfectionEnabled && m_mode != MODE_OFF;

  const bool bThermostatAllowedHeating = m_bThermostatAllowsHeating;

  // Software thermostat. When not in a thermostat mode or when no temperature sensor is installed
  // the hardware thermostat takes over
  if (IsThermostatMode() && m_tempSensors.getDeviceCount() > 0)
  {
    if (m_fBoilerTemperature <= 0.0f)
    {
      m_bThermostatAllowsHeating = false; // Sensor installed but no valid reading: fail safe
    }
    else if (bLegionellaDisinfect)
    {
      // Ignore setpoint during legionella disinfection: keep heating until the disinfect temperature has been held long enough.
      // Note: Cycling around a setpoint could drop the temperature to/below the disinfect temperature (this would reset the disinfecting counter)
      m_bThermostatAllowsHeating = true;
    }
    else if (m_fBoilerTemperature >= m_iThermostatSetpoint)
    {
      m_bThermostatAllowsHeating = false;
    }
    else if (m_fBoilerTemperature < m_iThermostatSetpoint - m_iThermostatHysteresis)
    {
      m_bThermostatAllowsHeating = true;
    }
  }
  else
  {
    // Without sensor never force heating in "setpoint, then budget" modes (only budget control remains)
    m_bThermostatAllowsHeating = (m_mode != MODE_SETPOINT_BUDGET && m_mode != MODE_PERCENT_SETPOINT_BUDGET);
  }

  if (m_bThermostatAllowsHeating != bThermostatAllowedHeating)
  {
    m_bPublishThermostatAllowsHeating = true;
  }

  float fNewPercentage = m_fCurrentPercentage;

  if (m_bBoilerOverHeated)
  {
    fNewPercentage = 0.0f;
  }
  else if (bLegionellaDisinfect)
  {
    fNewPercentage = (IsThermostatMode() && !m_bThermostatAllowsHeating) ? 0.0f : 100.0f; // Thermostat only blocks on invalid sensor reading
  }
  else if (m_mode == MODE_SETPOINT || (m_mode == MODE_SETPOINT_BUDGET && m_bThermostatAllowsHeating))
  {
    fNewPercentage = 100.0f; // Below setpoint: always heat, independent of (network) budget/watchdog
  }
  else if (m_mode == MODE_PERCENT_SETPOINT_BUDGET && m_bThermostatAllowsHeating)
  {
    fNewPercentage = m_iPowerPercentage; // Below setpoint: always heat at set percentage, independent of (network) budget/watchdog
  }
  else if (m_iNetworkWatchdogRecoveryCounter > 0 || m_mode == MODE_STANDBY || m_mode == MODE_OFF)
  {
    if (m_mode == MODE_STANDBY || m_mode == MODE_OFF)
    {
      m_iNetworkWatchdogRecoveryCounter = 0; // When off: quick recovery
    }

    if (m_fCurrentPercentage > 0)
    {
      fNewPercentage -= m_fStepClampNeg; // Device off or watch-dog triggered: output to 0%
    }
  }
  else if (m_mode == MODE_BOOST)
  {
    fNewPercentage = 100.0f; // Immediately 100% power
  }
  else if (m_mode == MODE_PERCENT || m_mode == MODE_PERCENT_SETPOINT)
  {
    fNewPercentage = m_iPowerPercentage;
  }
  else
  {
    // Calculate error step (percentage)
    const int32_t iPowerError = m_iPowerBudget - m_iBudgetMargin;
    float fErrorStep = (100.0f * iPowerError) / m_iBoilerPowerRating;
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
    if (iPowerError > m_iDeadZone || iPowerError < -m_iDeadZone)
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

  // Thermostat limits heating, except in "setpoint, then budget" modes where it only adds heating
  if (!m_bThermostatAllowsHeating && (m_mode == MODE_SETPOINT || m_mode == MODE_BUDGET_SETPOINT || m_mode == MODE_PERCENT_SETPOINT))
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
