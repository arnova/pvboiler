#ifndef PVBOILER_H
#define PVBOILER_H

#include <elapsedMillis.h>

#include "system.h"
#include "Network.h"
#include "PvBoilerCommandHandler.h"

// Control topics
#define MQTT_CONTROLLER_ON_OFF                      "controller_enable"

#define MQTT_SET_POWER_PERCENTAGE                   "power_percentage"
#define MQTT_SET_POWER_BUDGET                       "power_budget"

// Status topics
#define MQTT_FW_VERSION                             "firmware_version"
#define MQTT_OUTPUT_POWER                           "output_power"
#define MQTT_OUTPUT_PERCENTAGE                      "output_percentage"

// Triac phase control firing delay lookup table
// Index = power percentage (0-100)
// Value = firing delay as fraction of half-period (0.0 = start, 1.0 = end)
// Formula: P/Pmax = 1 - alpha/pi + sin(2*alpha)/(2*pi)
const float triac_percentage_factor[101] =
{
  // 0-9%
  1.0000f, 0.8840f, 0.8531f, 0.8310f, 0.8132f, 0.7980f, 0.7846f, 0.7724f, 0.7612f, 0.7508f,
  // 10-19%
  0.7411f, 0.7319f, 0.7231f, 0.7147f, 0.7067f, 0.6990f, 0.6915f, 0.6842f, 0.6772f, 0.6704f,
  // 20-29%
  0.6637f, 0.6572f, 0.6508f, 0.6445f, 0.6384f, 0.6324f, 0.6264f, 0.6206f, 0.6149f, 0.6092f,
  // 30-39%
  0.6036f, 0.5980f, 0.5926f, 0.5871f, 0.5818f, 0.5765f, 0.5712f, 0.5659f, 0.5607f, 0.5556f,
  // 40-49%
  0.5504f, 0.5453f, 0.5402f, 0.5351f, 0.5301f, 0.5251f, 0.5200f, 0.5150f, 0.5100f, 0.5050f,
  // 50-59%
  0.5000f, 0.4950f, 0.4900f, 0.4850f, 0.4800f, 0.4749f, 0.4699f, 0.4649f, 0.4598f, 0.4547f,
  // 60-69%
  0.4496f, 0.4444f, 0.4393f, 0.4341f, 0.4288f, 0.4235f, 0.4182f, 0.4129f, 0.4074f, 0.4020f,
  // 70-79%
  0.3964f, 0.3908f, 0.3851f, 0.3794f, 0.3736f, 0.3676f, 0.3616f, 0.3555f, 0.3492f, 0.3428f,
  // 80-89%
  0.3363f, 0.3296f, 0.3228f, 0.3158f, 0.3085f, 0.3010f, 0.2933f, 0.2853f, 0.2769f, 0.2681f,
  // 90-99%
  0.2589f, 0.2492f, 0.2388f, 0.2276f, 0.2154f, 0.2020f, 0.1868f, 0.1690f, 0.1469f, 0.1160f,
  // 100-100%
  0.0000f
};

class CPVBoiler
{
  public:
    CPVBoiler(CNetwork& network) : m_network(network) {}; // Constructor

    enum dim_style_e
    {
      DIM_STYLE_PHASE_CUT = 0,
      DIM_STYLE_SSR
    };
    typedef enum dim_style_e dim_style_t;

    enum logic_mode_e
    {
      LOGIC_MODE_BUDGET = 0,
      LOGIC_MODE_PERCENTAGE
    };
    typedef enum logic_mode_e logic_mode_t;

    void Loop();
    bool MQTTPublishValues();
    void LoadSettings();

    void TriggerWatchdog() { m_iWatchdogCounter = 0; };

    void SetOnOff(const bool& bVal) { m_bCtrlEnable = bVal; m_bUpdateCtrlEnable = true; };
    void SetPowerBudget(const int32_t& iVal) { m_iPowerBudget = iVal; m_bUpdatePowerBudget = true; };
    void SetPowerPercentage(const uint8_t& iVal) { m_iPowerPercentage = iVal; m_bUpdatePowerPercentage = true; };

    void SetBoilerPowerRating(const uint16_t& iPower);
    void SetPowerBudgetMargin(const uint16_t& iBudget);
    void SetLogicMode(const logic_mode_t& logicMode);
    void SetDimStyle(const dim_style_t& dimStyle);
    void SetSsrPeriodCount(const uint8_t& iPeriod);
    void SetErrorGain(const float& fGain);

    inline const float& GetTriacAngleFactor() const { return triac_percentage_factor[m_iCurrentPercentage]; };
    inline const uint8_t& GetCurrentPercentage() const { return m_iCurrentPercentage; };
    const uint16_t GetCurrentPower() const { return (m_iBoilerPowerRating * m_iCurrentPercentage) / 100; };

    const bool& GetCtrlOnOff() const { return m_bCtrlEnable; };
    const int32_t& GetPowerBudget() const { return m_iPowerBudget; };
    const uint8_t& GetPowerPercentage() const { return m_iPowerPercentage; };

    const uint16_t& GetBoilerPowerRating() const { return m_iBoilerPowerRating; };
    const uint16_t& GetPowerBudgetMargin() const { return m_iPowerBudgetMargin; };
    const logic_mode_t& GetLogicMode() const { return m_logicMode; };
    inline const dim_style_t& GetDimStyle() const { return m_dimStyle; };
    inline const uint8_t& GetSsrPeriodCount() const { return m_iSsrPeriodCount; };
    const float& GetErrorGain() const { return m_fErrorGain; };

  private:
    static void EepromCommit();

    void Update();
    void CheckWatchDog();

    CNetwork& m_network;

    elapsedMillis m_loopTimer = 0;
    elapsedMillis m_MQTTTimer = 0;
    uint32_t m_iWatchdogCounter = 0;
    uint32_t m_iWatchdogRecoveryCounter = 0;

    bool m_bCtrlEnable = true;
    bool m_bUpdateCtrlEnable = true;

    int32_t m_iPowerBudget = 0;
    bool m_bUpdatePowerBudget = true;

    uint8_t m_iPowerPercentage = 0;
    bool m_bUpdatePowerPercentage = true;

    uint8_t m_iCurrentPercentage = 0;
    bool m_bUpdateOutputPercentage = true;

    uint16_t m_iBoilerPowerRating = BOILER_POWER_RATING_DEFAULT;  // Watt
    uint16_t m_iPowerBudgetMargin = POWER_BUDGET_MARGIN_DEFAULT;  // Watt

    // Enable this to use SSR style mode instead of triac phase cut mode. This will blank/pass-through full periods like an SSR does
    CPVBoiler::dim_style_t m_dimStyle = CPVBoiler::DIM_STYLE_PHASE_CUT;

    // Amount of (half) sinus / periods when ssr style mode is used. Always use an even number!
    uint8_t m_iSsrPeriodCount = 50; // (= 0.5s @ 50 Hz).

    // (Proportional) error gain
    float m_fErrorGain = ERROR_GAIN_DEFAULT;

    logic_mode_t m_logicMode = LOGIC_MODE_BUDGET; // Select if you want to control using setting power percentage or providing power budget
};
#endif // PVBOILER_H