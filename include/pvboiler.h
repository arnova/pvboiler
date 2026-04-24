#ifndef PVBOILER_H
#define PVBOILER_H

#include <elapsedMillis.h>
#include <PubSubClient.h>

#include "system.h"

#define MQTT_MAX_SIZE 1024

#define MQTT_PVBOILER_NAME                          "pvboiler"

// Control topics
#define MQTT_CONTROLLER_ON_OFF                      "controller_enable"

#ifdef POWER_PERCENTAGE_CONTROL
#define MQTT_SET_POWER_PERCENTAGE                   "power_percentage"
#else
#define MQTT_SET_POWER_BUDGET                       "power_budget"
#endif

// Status topics
#define MQTT_FW_VERSION                             "firmware_version"
#define MQTT_OUTPUT_POWER                           "output_power"
#define MQTT_OUTPUT_PERCENTAGE                      "output_percentage"

#define WATCHDOG_TIMEOUT_TIME                 900   // Seconds = 15 minutes
#define WATCHDOG_RECOVERY_TIME                900   // Seconds = 15 minutes

#define MQTT_UPDATE_TIME                        1   // Seconds

// Triac angle/power conversion table (indexed as 0-100%)
const float triac_percentage_factor[101] = {
  1.000000, 0.993713, 0.987425, 0.981136, 0.974846, 0.968553, 0.962257, 0.955958, 0.949654, 0.943345,
  0.937030, 0.930707, 0.924377, 0.918038, 0.911689, 0.905330, 0.898959, 0.892575, 0.886178, 0.879766,
  0.873338, 0.866893, 0.860430, 0.853947, 0.847443, 0.840917, 0.834367, 0.827792, 0.821190, 0.814559,
  0.807898, 0.801205, 0.794478, 0.787715, 0.780914, 0.774074, 0.767191, 0.760263, 0.753288, 0.746263,
  0.739185, 0.732051, 0.724858, 0.717603, 0.710282, 0.702892, 0.695429, 0.687889, 0.680269, 0.672563,
  0.664767, 0.656877, 0.648888, 0.640795, 0.632593, 0.624276, 0.615839, 0.607275, 0.598577, 0.589739,
  0.580752, 0.571609, 0.562301, 0.552818, 0.543151, 0.533287, 0.523215, 0.512920, 0.502390, 0.491607,
  0.480555, 0.469214, 0.457562, 0.445573, 0.433219, 0.420468, 0.407282, 0.393616, 0.379420, 0.364635,
  0.349196, 0.333026, 0.316037, 0.298125, 0.279166, 0.259013, 0.237484, 0.214343, 0.189260, 0.161750,
  0.131065, 0.095982, 0.054353, 0.000000
};

class CPVBoiler
{
  public:
    typedef enum ha_config_type_e
    {
      SWITCH,
      NUMBER,
      BINARY_SENSOR,
      POWER_SENSOR,
      PERCENTAGE_SENSOR
    } ha_config_type_t;

    CPVBoiler(PubSubClient& MQTTClient);

    void loop();

    float GetTriacAngleFactor();

    bool MQTTPublishValues();

    void TriggerWatchdog() { m_iWatchdogCounter = 0; };

    void SetCtrlOnOff(const bool& bVal) { m_bCtrlEnable = bVal; m_bUpdateCtrlEnable = true; };
    void SetCtrlSetPowerBudget(const uint16_t& iVal) { m_iPowerBudget = iVal; m_bUpdatePowerBudget = true; };
    void SetCtrlSetPowerPercentage(const uint8_t& iVal) { m_iPowerPercentage = iVal; m_bUpdatePowerPercentage = true; };

  private:
    void Update();

    PubSubClient* m_pMQTTClient;
    elapsedMillis m_loopTimer = 0;
    elapsedMillis m_MQTTTimer = 0;
    uint32_t m_iWatchdogCounter = 0;
    uint32_t m_iWatchdogRecoveryCounter = 0;

    bool m_bCtrlEnable = true;
    bool m_bUpdateCtrlEnable = true;

    uint16_t m_iPowerBudget = 0;
    bool m_bUpdatePowerBudget = true;

    uint8_t m_iPowerPercentage = 0;
    bool m_bUpdatePowerPercentage = true;

    uint8_t m_iOutputPercentage = 0;
    bool m_bUpdateOutputPercentage = true;
};
#endif // PVBOILER_H