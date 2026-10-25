#include "PvBoiler.h"
#include "util.h"
#include "EepromSettings.h"

#include <EEPROM.h>

// Out-of-class definition required pre-C++17 (ESP32 core builds with gnu++11), redundant but harmless with C++17
constexpr CPvBoiler::mode_name_t CPvBoiler::m_modeNames[];
constexpr CPvBoiler::state_name_t CPvBoiler::m_stateNames[];

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

  m_bPublishState = true;

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
  // See SetLegionellaDisinfectionAgeHours()
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

  CMqttClient& mqttClient = m_network.GetMqttClient();

  // Publish our f/w version. Note: Static so only published on request
  if (m_bMqttRepublish)
  {
    mqttClient.PublishMessage(MQTT_FW_VERSION, MY_VERSION);
  }

  if (m_bPublishSettings || m_bMqttRepublish)
  {
    m_bPublishSettings = false;

    mqttClient.PublishUInt(MQTT_BOILER_POWER_RATING, m_iBoilerPowerRating);
    mqttClient.PublishMessage(MQTT_SET_MODE, m_modeNames[m_mode].strMqtt);
    mqttClient.PublishUInt(MQTT_DEAD_ZONE, m_iDeadZone);
    mqttClient.PublishUInt(MQTT_BUDGET_MARGIN, m_iBudgetMargin);
    mqttClient.PublishFloat(MQTT_ERROR_GAIN_POS, m_fErrorGainPos, 3);
    mqttClient.PublishFloat(MQTT_ERROR_GAIN_NEG, m_fErrorGainNeg, 3);
    mqttClient.PublishFloat(MQTT_STEP_CLAMP_POS, m_fStepClampPos, 2);
    mqttClient.PublishFloat(MQTT_STEP_CLAMP_NEG, m_fStepClampNeg, 2);
    mqttClient.PublishMessage(MQTT_DIM_STYLE, (m_dimStyle == DIM_STYLE_SSR) ? "SSR" : "Phase-angle");
    mqttClient.PublishUInt(MQTT_SSR_PERIOD_COUNT, m_iSsrPeriodCount);
    mqttClient.PublishUInt(MQTT_UPDATE_INTERVAL, m_iMqttUpdateInterval);
    mqttClient.PublishUInt(MQTT_NET_WD_TIMEOUT, m_iNetWatchDogTimeout);
    mqttClient.PublishUInt(MQTT_NET_WD_RECOVERY, m_iNetWatchDogRecovery);
    mqttClient.PublishUInt(MQTT_THERMOSTAT_HYSTERESIS, m_iThermostatHysteresis);
    mqttClient.PublishUInt(MQTT_LEGIONELLA_DANGER_ZONE_LOW_TEMP, m_iDangerZoneLowTemperature);
    mqttClient.PublishUInt(MQTT_LEGIONELLA_DANGER_ZONE_HIGH_TEMP, m_iDangerZoneHighTemperature);
    mqttClient.PublishUInt(MQTT_LEGIONELLA_DISINFECTION_INTERVAL, m_iDisinfectionIntervalDays);
    mqttClient.PublishUInt(MQTT_LEGIONELLA_DISINFECTION_DURATION, m_iDisinfectionDurationMinutes);
    mqttClient.PublishUInt(MQTT_LEGIONELLA_DISINFECTION_TEMPERATURE, m_iDisinfectionTemperature);
  }

  // Publish these MQTT values (if any) when timer expires (and connected)
  if (m_mqttPublishTimer > m_iMqttUpdateInterval * 1000 || m_bMqttRepublish)
  {
    m_mqttPublishTimer = 0;

    if (m_bPublishPowerBudget || m_bMqttRepublish)
    {
      m_bPublishPowerBudget = false;

      mqttClient.PublishInt(MQTT_SET_POWER_BUDGET, m_iPowerBudget);
    }

    if (m_bPublishPowerPercentage || m_bMqttRepublish)
    {
      m_bPublishPowerPercentage = false;

      mqttClient.PublishUInt(MQTT_SET_POWER_PERCENTAGE, m_iPowerPercentage);
    }

    if (m_bPublishThermostatSetpoint || m_bMqttRepublish)
    {
      m_bPublishThermostatSetpoint = false;

      mqttClient.PublishUInt(MQTT_SET_THERMOSTAT_SETPOINT, m_iThermostatSetpoint);
    }

    if (m_bPublishOutputPercentage || m_bMqttRepublish)
    {
      m_bPublishOutputPercentage = false;

      mqttClient.PublishFloat(MQTT_OUTPUT_PERCENTAGE, m_fCurrentPercentage, 2);
      mqttClient.PublishUInt(MQTT_OUTPUT_POWER, GetCurrentPower());
      mqttClient.PublishUInt(MQTT_TRIAC_PHASE_ANGLE, GetTriacPhaseAngle());

      if (m_dimStyle == DIM_STYLE_PHASE_ANGLE)
      {
        mqttClient.PublishFloat(MQTT_TRIAC_ANGLE_FACTOR, GetTriacAngleFactor(), 4);
      }
    }

    if (m_bPublishState || m_bMqttRepublish)
    {
      m_bPublishState = false;
      mqttClient.PublishMessage(MQTT_CONTROLLER_STATE, m_stateNames[m_state].strMqtt);
    }

    if (m_bPublishBoilerTemperature || m_bMqttRepublish)
    {
      m_bPublishBoilerTemperature = false;
      if (m_fBoilerTemperature > 0.0f)
      {
        mqttClient.PublishFloat(MQTT_BOILER_TEMPERATURE, m_fBoilerTemperature, 1);
      }
      else
      {
        mqttClient.PublishMessage(MQTT_BOILER_TEMPERATURE, "None"); // Home Assistant: state "unknown"
      }
    }

    if (m_bPublishBoilerOverHeated || m_bMqttRepublish)
    {
      m_bPublishBoilerOverHeated = false;
      mqttClient.PublishBool(MQTT_BOILER_OVERHEATED, m_bBoilerOverHeated);
    }

    if (m_bPublishThermostatAllowsHeating || m_bMqttRepublish)
    {
      m_bPublishThermostatAllowsHeating = false;
      mqttClient.PublishBool(MQTT_THERMOSTAT_ALLOWS_HEATING, m_bThermostatAllowsHeating);
    }

    if (m_bPublishDisinfectionEnabled || m_bMqttRepublish)
    {
      m_bPublishDisinfectionEnabled = false;
      mqttClient.PublishBool(MQTT_LEGIONELLA_DISINFECTION_ENABLED, m_bDisinfectionEnabled);
    }

    if (m_bPublishLegionellaDisinfectionRequired || m_bMqttRepublish)
    {
      m_bPublishLegionellaDisinfectionRequired = false;
      mqttClient.PublishBool(MQTT_LEGIONELLA_DISINFECTION_REQUIRED, m_bLegionellaDisinfectionRequired);
    }

    if (m_bPublishLegionellaDisinfectingElapsedMinutes || m_bMqttRepublish)
    {
      m_bPublishLegionellaDisinfectingElapsedMinutes = false;
      mqttClient.PublishUInt(MQTT_LEGIONELLA_DISINFECTING_ELAPSED, m_iLegionellaDisinfectingElapsedMinutes);
    }

    // Hold off publishing setters directly after subscribing so the (retained) broker values are
    // received first. Else we'd overwrite them with our own (reset) values
    const bool bSetterHoldOff = (m_mqttSetterHoldOffTimer < MQTT_SETTLE_TIME_MS);

    MqttPublishRetainedHours(mqttClient, MQTT_LEGIONELLA_DISINFECTION_AGE, MQTT_LEGIONELLA_DISINFECTION_AGE "/set", m_iLegionellaDisinfectionAgeHours,
                             m_bPublishLegionellaDisinfectionAgeHours, m_bPublishLegionellaDisinfectionAgeHoursSetter, m_iLegionellaDisinfectionAgeHoursSetter, bSetterHoldOff);
    MqttPublishRetainedHours(mqttClient, MQTT_LEGIONELLA_IN_DANGER_ZONE, MQTT_LEGIONELLA_IN_DANGER_ZONE "/set", m_iLegionellaInDangerZoneHours,
                             m_bPublishLegionellaInDangerZoneHours, m_bPublishLegionellaInDangerZoneHoursSetter, m_iLegionellaInDangerZoneHoursSetter, bSetterHoldOff);

    if (m_bPublishPowerGoodFlag || m_bMqttRepublish)
    {
      // Note: Clear before GetPowerGoodFlag() since it may set it again when the flag is restored
      m_bPublishPowerGoodFlag = false;
      mqttClient.PublishBool(MQTT_MAINS_ERROR, !GetPowerGoodFlag());
    }

    if (m_bPublishMainsValues || m_bMqttRepublish)
    {
      m_bPublishMainsValues = false;

      if (IsMainsPeriodValid())
      {
        // NOTE: Actual period is *2 since what we detect is rectified 50 Hz
        mqttClient.PublishUInt(MQTT_MAINS_PERIOD, m_iPeriodTime * 2);
        mqttClient.PublishFloat(MQTT_MAINS_FREQUENCY, (500.0f * 1000.0f) / m_iPeriodTime, 2);
      }
      else
      {
        mqttClient.PublishMessage(MQTT_MAINS_PERIOD, "None"); // Home Assistant: state "unknown"
        mqttClient.PublishMessage(MQTT_MAINS_FREQUENCY, "None");
      }

      if (IsMainsZeroCrossWindowValid())
      {
        mqttClient.PublishUInt(MQTT_MAINS_ZERO_CROSS_WINDOW, m_iZeroCrossWindow);
      }
      else
      {
        mqttClient.PublishMessage(MQTT_MAINS_ZERO_CROSS_WINDOW, "None"); // Home Assistant: state "unknown"
      }
    }

    // Publish uptime. Note: Always published since it changes every interval
    const CUptime::uptime_t upTime = GetUpTime();
    char strBuf[24]; // Enough room for "65535d 23:59:59"
    snprintf(strBuf, sizeof(strBuf), "%ud %02u:%02u:%02u", upTime.iDays, upTime.iHours, upTime.iMinutes, upTime.iSeconds);
    mqttClient.PublishMessage(MQTT_UP_TIME, strBuf);
  }

  m_bMqttRepublish = false;

  return true;
}


// Publish hours value that is restored from its retained setter topic after a reboot
void CPvBoiler::MqttPublishRetainedHours(CMqttClient& mqttClient, const char* strItem, const char* strSetterItem, const uint32_t iHours,
                                         bool& bPublish, bool& bPublishSetter, uint32_t& iSetterHours, const bool bSetterHoldOff)
{
  // Publish setter? Note: Only when changed else we'll loop on setting it over and over again via broker
  if (bPublishSetter && !bSetterHoldOff)
  {
    bPublishSetter = false;
    iSetterHours = iHours;
    mqttClient.PublishUInt(strSetterItem, iHours, true); // Retain: restored from broker after reboot
  }

  // Publish state?
  if (bPublish || m_bMqttRepublish)
  {
    bPublish = false;
    mqttClient.PublishUInt(strItem, iHours);
  }
}


void CPvBoiler::MqttUpdateDsmrSubscription()
{
  CMqttClient& mqttClient = m_network.GetMqttClient();

  // Get current grid power from DSMR-reader (used as power budget). Note: Only subscribe when used,
  // else its messages would keep feeding the network watchdog
  if (m_bDsmrPowerBudget)
  {
    mqttClient.subscribe(MQTT_DSMR_ELECTRICITY_DELIVERED, 1);
    mqttClient.subscribe(MQTT_DSMR_ELECTRICITY_RETURNED, 1);
  }
  else
  {
    mqttClient.unsubscribe(MQTT_DSMR_ELECTRICITY_DELIVERED);
    mqttClient.unsubscribe(MQTT_DSMR_ELECTRICITY_RETURNED);
  }
}


void CPvBoiler::MqttPublishDiscovery()
{
  CMqttClient& mqttClient = m_network.GetMqttClient();

  // (Re)subscribing makes the broker resend retained setter values: hold off publishing our own
  m_mqttSetterHoldOffTimer = 0;

  // Publish HA discovery config and subscribe to control (setter) topics
  mqttClient.PublishBinarySensorConfig(MQTT_MAINS_ERROR, true);

  // Note: Don't retain power budget: a stale (retained) budget would be applied after (re)connect and also feed the network watchdog
  mqttClient.PublishNumberConfig(MQTT_SET_POWER_BUDGET, 1.0f, -100000.0f, 100000.0f, "W", "power", true, false);
  mqttClient.PublishNumberConfig(MQTT_SET_POWER_PERCENTAGE, 1.0f, 0.0f, 100.0f, "%", "", false);
  mqttClient.PublishNumberConfig(MQTT_SET_THERMOSTAT_SETPOINT, 1.0f, THERMOSTAT_SETPOINT_MIN, THERMOSTAT_SETPOINT_MAX, "°C", "temperature", true);

  mqttClient.PublishSensorConfig(MQTT_ERROR_GAIN_POS, "", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_ERROR_GAIN_NEG, "", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_STEP_CLAMP_POS, "%", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_STEP_CLAMP_NEG, "%", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_DEAD_ZONE, "W", "power", "", true);
  mqttClient.PublishSensorConfig(MQTT_BUDGET_MARGIN, "W", "power", "", true);

  mqttClient.PublishSensorConfig(MQTT_OUTPUT_POWER, "W", "power");
  mqttClient.PublishSensorConfig(MQTT_OUTPUT_PERCENTAGE, "%", "");

  const char* strModes[MODE_COUNT];
  for (uint8_t it = 0; it < MODE_COUNT; it++)
  {
    strModes[it] = m_modeNames[it].strMqtt;
  }
  mqttClient.PublishSelectConfig(MQTT_SET_MODE, strModes, MODE_COUNT);

  const char* strStates[STATE_COUNT];
  for (uint8_t it = 0; it < STATE_COUNT; it++)
  {
    strStates[it] = m_stateNames[it].strMqtt;
  }
  mqttClient.PublishEnumSensorConfig(MQTT_CONTROLLER_STATE, strStates, STATE_COUNT);

  mqttClient.PublishSensorConfig(MQTT_BOILER_POWER_RATING, "W", "power", "", true);

  mqttClient.PublishSensorConfig(MQTT_DIM_STYLE, "", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_SSR_PERIOD_COUNT, "", "", "", true);

  // Diagnostic
  mqttClient.PublishSensorConfig(MQTT_WIFI_SSID, "", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_IP_ADDRESS, "", "", "", true);
//  mqttClient.PublishSensorConfig(MQTT_IP_NETMASK, "", "", true);

  mqttClient.PublishSensorConfig(MQTT_TRIAC_PHASE_ANGLE, "us", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_TRIAC_ANGLE_FACTOR, "", "", "measurement", true);

  mqttClient.PublishSensorConfig(MQTT_MAINS_PERIOD, "us", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_MAINS_FREQUENCY, "Hz", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_MAINS_ZERO_CROSS_WINDOW, "us", "", "", true);

  mqttClient.PublishSensorConfig(MQTT_BOILER_TEMPERATURE, "°C", "temperature", "");
  mqttClient.PublishBinarySensorConfig(MQTT_BOILER_OVERHEATED, true);
  mqttClient.PublishBinarySensorConfig(MQTT_THERMOSTAT_ALLOWS_HEATING, true);
  mqttClient.PublishBinarySensorConfig(MQTT_LEGIONELLA_DISINFECTION_ENABLED, true);
  mqttClient.PublishBinarySensorConfig(MQTT_LEGIONELLA_DISINFECTION_REQUIRED);
  mqttClient.PublishSensorConfig(MQTT_LEGIONELLA_DISINFECTING_ELAPSED, "min", "duration", "", true);

  mqttClient.PublishNumberConfig(MQTT_LEGIONELLA_DISINFECTION_AGE, 1.0f, 0.0f, LEGIONELLA_HOURS_MAX, "h");
  mqttClient.PublishNumberConfig(MQTT_LEGIONELLA_IN_DANGER_ZONE, 1.0f, 0.0f, LEGIONELLA_HOURS_MAX, "h");

  mqttClient.PublishSensorConfig(MQTT_THERMOSTAT_HYSTERESIS, "°C", "temperature", "", true);
  mqttClient.PublishSensorConfig(MQTT_LEGIONELLA_DANGER_ZONE_LOW_TEMP, "°C", "temperature", "", true);
  mqttClient.PublishSensorConfig(MQTT_LEGIONELLA_DANGER_ZONE_HIGH_TEMP, "°C", "temperature", "", true);
  mqttClient.PublishSensorConfig(MQTT_LEGIONELLA_DISINFECTION_INTERVAL, "d", "duration", "", true);
  mqttClient.PublishSensorConfig(MQTT_LEGIONELLA_DISINFECTION_DURATION, "min", "duration", "", true);
  mqttClient.PublishSensorConfig(MQTT_LEGIONELLA_DISINFECTION_TEMPERATURE, "°C", "temperature", "", true);

  mqttClient.PublishSensorConfig(MQTT_UPDATE_INTERVAL, "s", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_NET_WD_TIMEOUT, "s", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_NET_WD_RECOVERY, "s", "", "", true);
  mqttClient.PublishSensorConfig(MQTT_UP_TIME, "", "", "", true);
}


void CPvBoiler::MqttOnConnect()
{
  MqttPublishDiscovery();
  MqttUpdateDsmrSubscription();
}


void CPvBoiler::LoadSettings()
{
  uint8_t iVal8 = 0;

  CEepromSettings::Load(EEPROM_BP_RATING, m_iBoilerPowerRating, BOILER_POWER_RATING_MIN, BOILER_POWER_RATING_MAX, BOILER_POWER_RATING_DEFAULT);
  CEepromSettings::Load(EEPROM_DEAD_ZONE, m_iDeadZone, DEAD_ZONE_MIN, DEAD_ZONE_MAX, DEAD_ZONE_DEFAULT);
  CEepromSettings::Load(EEPROM_BUDGET_MARGIN, m_iBudgetMargin, BUDGET_MARGIN_MIN, BUDGET_MARGIN_MAX, BUDGET_MARGIN_DEFAULT);

  // Note: MODE_BOOST is never stored
  EEPROM.get(EEPROM_CTRL_MODE, iVal8);
  m_mode = (iVal8 < MODE_COUNT && iVal8 != CPvBoiler::MODE_BOOST) ? static_cast<CPvBoiler::mode_t>(iVal8) : CPvBoiler::MODE_BUDGET;

  EEPROM.get(EEPROM_DIM_STYLE, iVal8);
  m_dimStyle = (iVal8 == 0x01) ? CPvBoiler::DIM_STYLE_SSR : CPvBoiler::DIM_STYLE_PHASE_ANGLE;

  CEepromSettings::Load(EEPROM_SSR_PERIOD, m_iSsrPeriodCount, SSR_PERIOD_COUNT_MIN, SSR_PERIOD_COUNT_MAX, SSR_PERIOD_COUNT_DEFAULT);

  CEepromSettings::Load(EEPROM_ERROR_GAIN_POS, m_fErrorGainPos, ERROR_GAIN_POS_MIN, ERROR_GAIN_POS_MAX, ERROR_GAIN_POS_DEFAULT);
  CEepromSettings::Load(EEPROM_ERROR_GAIN_NEG, m_fErrorGainNeg, ERROR_GAIN_NEG_MIN, ERROR_GAIN_NEG_MAX, ERROR_GAIN_NEG_DEFAULT);
  CEepromSettings::Load(EEPROM_STEP_CLAMP_POS, m_fStepClampPos, STEP_CLAMP_POS_MIN, STEP_CLAMP_POS_MAX, STEP_CLAMP_POS_DEFAULT);
  CEepromSettings::Load(EEPROM_STEP_CLAMP_NEG, m_fStepClampNeg, STEP_CLAMP_NEG_MIN, STEP_CLAMP_NEG_MAX, STEP_CLAMP_NEG_DEFAULT);

  CEepromSettings::Load(EEPROM_NET_WD_TIMEOUT, m_iNetWatchDogTimeout, 0, NETWORK_WATCHDOG_TIMEOUT_MAX, NETWORK_WATCHDOG_TIMEOUT_DEFAULT);
  CEepromSettings::Load(EEPROM_NET_WD_RECOVER, m_iNetWatchDogRecovery, 0, NETWORK_WATCHDOG_RECOVERY_MAX, NETWORK_WATCHDOG_RECOVERY_DEFAULT);
  CEepromSettings::Load(EEPROM_MQTT_INTERVAL, m_iMqttUpdateInterval, MQTT_UPDATE_TIME_MIN, MQTT_UPDATE_TIME_MAX, MQTT_UPDATE_TIME_DEFAULT);

  CEepromSettings::Load(EEPROM_THERMOSTAT_SP, m_iThermostatSetpoint, THERMOSTAT_SETPOINT_MIN, THERMOSTAT_SETPOINT_MAX, THERMOSTAT_SETPOINT_DEFAULT);
  m_bPublishThermostatSetpoint = true;

  // Note: Anything other than 0x00 (eg. erased EEPROM) means enabled
  EEPROM.get(EEPROM_DISINFECTION_EN, iVal8);
  m_bDisinfectionEnabled = (iVal8 != 0x00);
  m_bPublishDisinfectionEnabled = true;

  // Note: Anything other than 0x01 (eg. erased EEPROM) means direct
  EEPROM.get(EEPROM_BUDGET_SRC, iVal8);
  m_bDsmrPowerBudget = (iVal8 == 0x01);

  CEepromSettings::Load(EEPROM_THERMOSTAT_HYST, m_iThermostatHysteresis, THERMOSTAT_HYSTERESIS_MIN, THERMOSTAT_HYSTERESIS_MAX, THERMOSTAT_HYSTERESIS_DEFAULT);

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

  CEepromSettings::Load(EEPROM_DISINFECTION_INTERVAL, m_iDisinfectionIntervalDays, DISINFECTION_INTERVAL_DAYS_MIN, DISINFECTION_INTERVAL_DAYS_MAX, DISINFECTION_INTERVAL_DAYS_DEFAULT);
  m_legionella.SetDisinfectionIntervalDays(m_iDisinfectionIntervalDays);

  CEepromSettings::Load(EEPROM_DISINFECTION_DURATION, m_iDisinfectionDurationMinutes, DISINFECTION_DURATION_MINUTES_MIN, DISINFECTION_DURATION_MINUTES_MAX, DISINFECTION_DURATION_MINUTES_DEFAULT);
  m_legionella.SetDisinfectionDurationMinutes(m_iDisinfectionDurationMinutes);

  CEepromSettings::Load(EEPROM_DISINFECTION_TEMP, m_iDisinfectionTemperature, DISINFECTION_TEMPERATURE_MIN, DISINFECTION_TEMPERATURE_MAX, DISINFECTION_TEMPERATURE_DEFAULT);
  m_legionella.SetDisinfectionTemperature(m_iDisinfectionTemperature);

  m_bPublishSettings = true;
}


void CPvBoiler::SetThermostatSetpoint(const uint8_t iTemperature)
{
  CEepromSettings::Store(EEPROM_THERMOSTAT_SP, m_iThermostatSetpoint, iTemperature);

  m_bPublishThermostatSetpoint = true;
}


void CPvBoiler::SetDisinfectionEnabled(const bool bEnable)
{
  CEepromSettings::Store(EEPROM_DISINFECTION_EN, m_bDisinfectionEnabled, bEnable);

  m_bPublishDisinfectionEnabled = true;
}


void CPvBoiler::SetDsmrPowerBudget(const bool bEnable)
{
  if (CEepromSettings::Store(EEPROM_BUDGET_SRC, m_bDsmrPowerBudget, bEnable) && m_network.IsMqttConnected())
  {
    MqttUpdateDsmrSubscription();
  }
}


void CPvBoiler::SetThermostatHysteresis(const uint8_t iHysteresis)
{
  if (CEepromSettings::Store(EEPROM_THERMOSTAT_HYST, m_iThermostatHysteresis, iHysteresis))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDangerZoneLowTemperature(const uint8_t iTemperature)
{
  if (CEepromSettings::Store(EEPROM_DZ_LOW_TEMP, m_iDangerZoneLowTemperature, iTemperature))
  {
    m_legionella.SetDangerZoneLowTemperature(iTemperature);

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDangerZoneHighTemperature(const uint8_t iTemperature)
{
  if (CEepromSettings::Store(EEPROM_DZ_HIGH_TEMP, m_iDangerZoneHighTemperature, iTemperature))
  {
    m_legionella.SetDangerZoneHighTemperature(iTemperature);

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDisinfectionIntervalDays(const uint8_t iDays)
{
  if (CEepromSettings::Store(EEPROM_DISINFECTION_INTERVAL, m_iDisinfectionIntervalDays, iDays))
  {
    m_legionella.SetDisinfectionIntervalDays(iDays);

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDisinfectionDurationMinutes(const uint8_t iMinutes)
{
  if (CEepromSettings::Store(EEPROM_DISINFECTION_DURATION, m_iDisinfectionDurationMinutes, iMinutes))
  {
    m_legionella.SetDisinfectionDurationMinutes(iMinutes);

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDisinfectionTemperature(const uint8_t iTemperature)
{
  if (CEepromSettings::Store(EEPROM_DISINFECTION_TEMP, m_iDisinfectionTemperature, iTemperature))
  {
    m_legionella.SetDisinfectionTemperature(iTemperature);

    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetBoilerPowerRating(const uint16_t iPower)
{
  if (CEepromSettings::Store(EEPROM_BP_RATING, m_iBoilerPowerRating, iPower))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetDeadZone(const uint8_t iDeadZone)
{
  if (CEepromSettings::Store(EEPROM_DEAD_ZONE, m_iDeadZone, iDeadZone))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetBudgetMargin(const uint16_t iMargin)
{
  if (CEepromSettings::Store(EEPROM_BUDGET_MARGIN, m_iBudgetMargin, iMargin))
  {
    m_bPublishSettings = true;
  }
}


bool CPvBoiler::GetModeFromName(const char* strName, CPvBoiler::mode_t& mode)
{
  for (uint8_t it = 0; it < MODE_COUNT; it++)
  {
    if (strcasecmp(strName, m_modeNames[it].strMqtt) == 0)
    {
      mode = static_cast<CPvBoiler::mode_t>(it);
      return true;
    }
  }

  return false;
}


bool CPvBoiler::GetModeFromCliName(const char* strName, CPvBoiler::mode_t& mode)
{
  for (uint8_t it = 0; it < MODE_COUNT; it++)
  {
    if (strcasecmp(strName, m_modeNames[it].strCli) == 0 || strcasecmp(strName, m_modeNames[it].strCliAlt) == 0)
    {
      mode = static_cast<CPvBoiler::mode_t>(it);
      return true;
    }
  }

  return GetModeFromName(strName, mode);
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
  if (CEepromSettings::Store(EEPROM_SSR_PERIOD, m_iSsrPeriodCount, iCount))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetErrorGainPos(const float fGain)
{
  if (CEepromSettings::Store(EEPROM_ERROR_GAIN_POS, m_fErrorGainPos, fGain))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetErrorGainNeg(const float fGain)
{
  if (CEepromSettings::Store(EEPROM_ERROR_GAIN_NEG, m_fErrorGainNeg, fGain))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetStepClampPos(const float fClamp)
{
  if (CEepromSettings::Store(EEPROM_STEP_CLAMP_POS, m_fStepClampPos, fClamp))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetStepClampNeg(const float fClamp)
{
  if (CEepromSettings::Store(EEPROM_STEP_CLAMP_NEG, m_fStepClampNeg, fClamp))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetMqttUpdateInterval(const uint8_t iInterval)
{
  if (CEepromSettings::Store(EEPROM_MQTT_INTERVAL, m_iMqttUpdateInterval, iInterval))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetNetWatchDogTimeout(const uint16_t iTime)
{
  if (CEepromSettings::Store(EEPROM_NET_WD_TIMEOUT, m_iNetWatchDogTimeout, iTime))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::SetNetWatchDogRecovery(const uint16_t iTime)
{
  if (CEepromSettings::Store(EEPROM_NET_WD_RECOVER, m_iNetWatchDogRecovery, iTime))
  {
    m_bPublishSettings = true;
  }
}


void CPvBoiler::FactoryReset()
{
  const bool bWasDsmrPowerBudget = m_bDsmrPowerBudget;

  // Erase all controller settings (network settings are stored before these). LoadSettings() then falls back to defaults
  CEepromSettings::Erase(EEPROM_BP_RATING, EEPROM_DISINFECTION_TEMP + DISINFECTION_TEMP_SIZE - EEPROM_BP_RATING);

  // Reset controller (also reloads settings)
  Reset();

  if (bWasDsmrPowerBudget && m_network.IsMqttConnected())
  {
    MqttUpdateDsmrSubscription();
  }
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
  state_t state = STATE_OFF;

  if (m_bBoilerOverHeated)
  {
    fNewPercentage = 0.0f;
    state = STATE_OVERHEATED;
  }
  else if (bLegionellaDisinfect)
  {
    if (IsThermostatMode() && !m_bThermostatAllowsHeating) // Thermostat only blocks on invalid sensor reading
    {
      fNewPercentage = 0.0f;
      state = STATE_SENSOR_FAULT;
    }
    else
    {
      fNewPercentage = 100.0f;
      state = (m_iLegionellaDisinfectingElapsedMinutes > 0) ? STATE_LEGIONELLA_HOLDING : STATE_LEGIONELLA_HEATING;
    }
  }
  else if (m_mode == MODE_SETPOINT || (m_mode == MODE_SETPOINT_BUDGET && m_bThermostatAllowsHeating))
  {
    fNewPercentage = 100.0f; // Below setpoint: always heat, independent of (network) budget/watchdog
    state = STATE_HEATING_TO_SETPOINT;
  }
  else if (m_mode == MODE_PERCENT_SETPOINT_BUDGET && m_bThermostatAllowsHeating)
  {
    fNewPercentage = m_iPowerPercentage; // Below setpoint: always heat at set percentage, independent of (network) budget/watchdog
    state = STATE_HEATING_TO_SETPOINT;
  }
  else if (m_iNetworkWatchdogRecoveryCounter > 0 || m_mode == MODE_STANDBY || m_mode == MODE_OFF)
  {
    if (m_mode == MODE_STANDBY || m_mode == MODE_OFF)
    {
      m_iNetworkWatchdogRecoveryCounter = 0; // When off: quick recovery
      state = (m_mode == MODE_OFF) ? STATE_OFF : STATE_STANDBY;
    }
    else
    {
      state = STATE_NETWORK_TIMEOUT;
    }

    if (m_fCurrentPercentage > 0)
    {
      fNewPercentage -= m_fStepClampNeg; // Device off or watch-dog triggered: output to 0%
    }
  }
  else if (m_mode == MODE_BOOST)
  {
    fNewPercentage = 100.0f; // Immediately 100% power
    state = STATE_BOOST;
  }
  else if (m_mode == MODE_PERCENT || m_mode == MODE_PERCENT_SETPOINT)
  {
    fNewPercentage = m_iPowerPercentage;
    state = STATE_FIXED_PERCENTAGE;
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

    state = (fNewPercentage > 0.0f) ? STATE_HEATING_ON_SURPLUS : STATE_WAITING_FOR_BUDGET;
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

    if (state != STATE_OVERHEATED)
    {
      // Note: Thermostat also blocks when sensor is installed but has no valid reading
      state = (m_fBoilerTemperature <= 0.0f) ? STATE_SENSOR_FAULT : STATE_SETPOINT_REACHED;
    }
  }

  if (fNewPercentage != m_fCurrentPercentage)
  {
    m_fCurrentPercentage = fNewPercentage;
    m_bPublishOutputPercentage = true;
  }

  if (state != m_state)
  {
    m_state = state;
    m_bPublishState = true;
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
