#pragma once
#ifndef SYSTEM_H
#define SYSTEM_H

#include <Arduino.h>

// Version string:
#define MY_VERSION "1.10"

// Firmware version string
const char VER_STR_P[] PROGMEM = "PV-Boiler Controller " MY_VERSION " - (C) 2026 Arno van Amersfoort (Arnova)";

// Triac gate pulse width
#define GATE_PULSE_WIDTH                        50    // uS

// Triac phase margin we keep to prevent misfires
#define TRIAC_PHASE_ANGLE_MARGIN_US             500   // us

// Maximum amount of net period & zero cross window measurement outliers
#define MAX_CONSECUTIVE_OUTLIERS                8

// Maximum time elapsed before we flag an actual power error
#define POWER_GOOD_TIME_MAX                     1000  // ms

// Boiler power rating
#define BOILER_POWER_RATING_DEFAULT             2500  // Watt
#define BOILER_POWER_RATING_MIN                 100   // Watt
#define BOILER_POWER_RATING_MAX                 10000 // Watt

// The dead zone value represents the window where a change in power budget is ignored
#define DEAD_ZONE_DEFAULT                       0     // Watt
#define DEAD_ZONE_MIN                           0     // Watt
#define DEAD_ZONE_MAX                           254   // Watt

// The budget margin represents the margin used to prevent grid power use
#define BUDGET_MARGIN_DEFAULT                   50    // Watt
#define BUDGET_MARGIN_MIN                       0     // Watt
#define BUDGET_MARGIN_MAX                       5000  // Watt

// Amount of full (mains) periods when ssr style mode is used
#define SSR_PERIOD_COUNT_DEFAULT                25    // (= 0.5s @ 50 Hz).
#define SSR_PERIOD_COUNT_MIN                    1
#define SSR_PERIOD_COUNT_MAX                    254

// Positive error gain
#define ERROR_GAIN_POS_DEFAULT                  0.05f
#define ERROR_GAIN_POS_MIN                      0.0001f
#define ERROR_GAIN_POS_MAX                      100.0f

// Negative error gain
#define ERROR_GAIN_NEG_DEFAULT                  0.2f
#define ERROR_GAIN_NEG_MIN                      0.0001f
#define ERROR_GAIN_NEG_MAX                      100.0f

#define STEP_CLAMP_POS_DEFAULT                  3.0f    // %
#define STEP_CLAMP_POS_MIN                      0.01f   // %
#define STEP_CLAMP_POS_MAX                      100.0f  // %

#define STEP_CLAMP_NEG_DEFAULT                  12.0f   // %
#define STEP_CLAMP_NEG_MIN                      0.01f   // %
#define STEP_CLAMP_NEG_MAX                      100.0f  // %

// Zero crossing window related values
#define ZERO_CROSS_WINDOW_MIN_US                700   // us
#define ZERO_CROSS_WINDOW_MAX_US                1300  // us
#define ZERO_CROSS_WINDOW_INVALID               65535 // us

// Net period time related values (for (rectified) 49-62 Hz half-period)
#define NET_PERIOD_MIN_US                       8000  // us
#define NET_PERIOD_MAX_US                       11000 // us
#define NET_PERIOD_INVALID                      65535 // us


// Temperature related
#define TEMPERATURE_MAX_RETRIES                 255
#define TEMPERATURE_OVERHEATING_MAX             90.0f // Celsius
#define THERMOSTAT_SETPOINT_DEFAULT             55    // Celsius
#define THERMOSTAT_SETPOINT_MIN                 45    // Celsius
#define THERMOSTAT_SETPOINT_MAX                 90    // Celsius
#define THERMOSTAT_HYSTERESIS_DEFAULT           1     // Celsius
#define THERMOSTAT_HYSTERESIS_MIN               1     // Celsius
#define THERMOSTAT_HYSTERESIS_MAX               10    // Celsius

// Legionella related
#define DANGER_ZONE_LOW_TEMPERATURE_DEFAULT     20    // Celsius
#define DANGER_ZONE_HIGH_TEMPERATURE_DEFAULT    45    // Celsius
#define DANGER_ZONE_TEMPERATURE_MIN             10    // Celsius
#define DANGER_ZONE_TEMPERATURE_MAX             60    // Celsius

#define DISINFECTION_INTERVAL_DAYS_DEFAULT      7     // Days. Max. time spent in danger zone before disinfection is required
#define DISINFECTION_INTERVAL_DAYS_MIN          1     // Days
#define DISINFECTION_INTERVAL_DAYS_MAX          30    // Days

#define DISINFECTION_DURATION_MINUTES_DEFAULT   15    // Minutes
#define DISINFECTION_DURATION_MINUTES_MIN       2     // Minutes
#define DISINFECTION_DURATION_MINUTES_MAX       150   // Minutes

#define DISINFECTION_TEMPERATURE_DEFAULT        60    // Celsius
#define DISINFECTION_TEMPERATURE_MIN            45    // Celsius
#define DISINFECTION_TEMPERATURE_MAX            80    // Celsius

// Network watchdog timer settings
#define NETWORK_WATCHDOG_TIMEOUT_DEFAULT        60    // Seconds = 1 minute
#define NETWORK_WATCHDOG_TIMEOUT_MAX            65000

#define NETWORK_WATCHDOG_RECOVERY_DEFAULT       60    // Seconds = 1 minute
#define NETWORK_WATCHDOG_RECOVERY_MAX           65000

#define MQTT_UPDATE_TIME_DEFAULT                10     // Seconds
#define MQTT_UPDATE_TIME_MIN                    1      // Seconds
#define MQTT_UPDATE_TIME_MAX                    254    // Seconds

// Enable below for additional wifi / mqtt debug messages
#define WIFI_DEBUG
//#define MQTT_DEBUG

// Wifi connect timeout
#define WIFI_CONNECT_TIMEOUT                    10000 // ms

// Wifi disconnect timeout (max. wait for disconnect to complete when restarting WiFi)
#define WIFI_DISCONNECT_TIMEOUT                 1000  // ms

// MQTT connect timeout
#define MQTT_CONNECT_TIMEOUT                    10000 // ms

// Settle time after (re)connecting/subscribing: lets the broker resend retained setters (before we publish our own)
// and HA discover new entities (before we publish their values)
#define MQTT_SETTLE_TIME_MS                     5000  // ms

// MQTT settings
#define MQTT_PORT                               1883
#define MQTT_MAX_MESSAGE_SIZE                   1024
#define MQTT_MAX_TOPIC_ITEM_SIZE                64
#define MQTT_MAX_CONFIG_TOPIC_SIZE              128

// Control topic items
#define MQTT_SET_MODE                           "mode"
#define MQTT_SET_POWER_PERCENTAGE               "power_percentage"
#define MQTT_SET_POWER_BUDGET                   "power_budget"
#define MQTT_SET_THERMOSTAT_SETPOINT            "thermostat_setpoint"

// Status topic items
#define MQTT_FW_VERSION                         "firmware_version"
#define MQTT_OUTPUT_POWER                       "output_power"
#define MQTT_OUTPUT_PERCENTAGE                  "output_percentage"
#define MQTT_CONTROLLER_STATE                   "state"

#define MQTT_BOILER_POWER_RATING                "boiler_power_rating"
#define MQTT_DEAD_ZONE                          "dead_zone"
#define MQTT_BUDGET_MARGIN                      "budget_margin"
#define MQTT_DIM_STYLE                          "dim_style"
#define MQTT_SSR_PERIOD_COUNT                   "ssr_period_count"
#define MQTT_ERROR_GAIN_POS                     "error_gain_positive"
#define MQTT_ERROR_GAIN_NEG                     "error_gain_negative"
#define MQTT_STEP_CLAMP_POS                     "step_clamp_positive"
#define MQTT_STEP_CLAMP_NEG                     "step_clamp_negative"

// Diagnostic topic items
#define MQTT_WIFI_SSID                          "wifi_ssid"
#define MQTT_IP_ADDRESS                         "ip_address"
#define MQTT_IP_NETMASK                         "ip_netmask"
#define MQTT_TRIAC_ANGLE_FACTOR                 "triac_angle_factor"
#define MQTT_TRIAC_PHASE_ANGLE                  "triac_phase_angle"
#define MQTT_MAINS_PERIOD                       "mains_period"
#define MQTT_MAINS_FREQUENCY                    "mains_frequency"
#define MQTT_MAINS_ZERO_CROSS_WINDOW            "mains_zero_cross_window"
#define MQTT_MAINS_ERROR                        "mains_error"
#define MQTT_UPDATE_INTERVAL                    "update_interval"
#define MQTT_NET_WD_TIMEOUT                     "network_watchdog_timeout"
#define MQTT_NET_WD_RECOVERY                    "network_watchdog_recovery"
#define MQTT_UP_TIME                            "up_time"

// Value (diag) sensors
#define MQTT_BOILER_TEMPERATURE                 "boiler_temperature"
#define MQTT_BOILER_OVERHEATED                  "boiler_overheated"
#define MQTT_THERMOSTAT_ALLOWS_HEATING          "thermostat_allows_heating"
#define MQTT_LEGIONELLA_DISINFECTION_ENABLED    "legionella_disinfection_enabled"
#define MQTT_LEGIONELLA_DISINFECTION_REQUIRED   "legionella_disinfection_required"
#define MQTT_LEGIONELLA_DISINFECTING_ELAPSED    "legionella_disinfecting_elapsed"
#define MQTT_LEGIONELLA_DISINFECTION_AGE        "legionella_disinfection_age"
#define MQTT_LEGIONELLA_IN_DANGER_ZONE          "legionella_in_danger_zone"

// Setting diag sensors
#define MQTT_THERMOSTAT_HYSTERESIS              "thermostat_hysteresis"
#define MQTT_LEGIONELLA_DANGER_ZONE_LOW_TEMP    "legionella_danger_zone_low_temperature"
#define MQTT_LEGIONELLA_DANGER_ZONE_HIGH_TEMP   "legionella_danger_zone_high_temperature"
#define MQTT_LEGIONELLA_DISINFECTION_INTERVAL   "legionella_disinfection_interval"
#define MQTT_LEGIONELLA_DISINFECTION_DURATION   "legionella_disinfection_duration"
#define MQTT_LEGIONELLA_DISINFECTION_TEMPERATURE "legionella_disinfection_temperature"

// Home Assistant birth/last will topic (HA publishes "online" here after it (re)started)
#define MQTT_HA_STATUS_TOPIC                    "homeassistant/status"

// DSMR-reader topics
#define MQTT_DSMR_ELECTRICITY_DELIVERED         "dsmr/reading/electricity_currently_delivered"
#define MQTT_DSMR_ELECTRICITY_RETURNED          "dsmr/reading/electricity_currently_returned"

// Socket server settings
#define SOCKET_SERVER_PORT                      8000

// Misc. (home assistant) settings
#define DEFAULT_HOST_NAME                       "PvBoiler"
#define DEVICE_NAME                             "PV-Boiler"
#define DEVICE_MODEL                            "PV-Boiler Controller"
#define MANUFACTURER                            "Arnova"

// UART/socket settings
#define BAUD_RATE                     115200
#define CMD_BUF_SIZE                  80
#define RESULT_BUF_SIZE               80
#define SOCKET_CLIENT_TIMEOUT_MS      100 // ms

// EEPROM byte sizes
#define CHECKSUM_SIZE           2
#define RESERVED_SIZE           100
#define WIFI_SSID_MAX_SIZE      32
#define WIFI_PASSWORD_MAX_SIZE  64
#define HOST_NAME_MAX_SIZE      32
#define MQTT_USER_MAX_SIZE      32
#define MQTT_PASSWORD_MAX_SIZE  64
#define IP_BYTE_SIZE            4
#define BP_RATING_SIZE          2
#define CTRL_MODE_SIZE          1
#define DIM_STYLE_SIZE          1
#define SSR_PERIOD_SIZE         1
#define NET_WD_TIMEOUT_SIZE     2
#define NET_WD_RECOVER_SIZE     2
#define MQTT_INTERVAL_SIZE      1
#define ERROR_GAIN_SIZE         sizeof(float)
#define STEP_CLAMP_SIZE         sizeof(float)
#define DEAD_ZONE_SIZE          1
#define BUDGET_MARGIN_SIZE      2
#define THERMOSTAT_SP_SIZE      1
#define DISINFECTION_EN_SIZE    1
#define BUDGET_SRC_SIZE         1
#define THERMOSTAT_HYST_SIZE    1
#define DZ_LOW_TEMP_SIZE        1
#define DZ_HIGH_TEMP_SIZE       1
#define DISINFECTION_INTERVAL_SIZE 1
#define DISINFECTION_DURATION_SIZE 1
#define DISINFECTION_TEMP_SIZE  1

// EEPROM locations
// NOTE: Erased EEPROM (0xFF) must always be invalid for controller settings (EEPROM_BP_RATING and up),
//       as CPvBoiler::FactoryReset() relies on LoadSettings() falling back to defaults for erased values
#define EEPROM_CHECKSUM       0                                                   // 0
#define EEPROM_WIFI_SSID      EEPROM_CHECKSUM + CHECKSUM_SIZE                     // 2
#define EEPROM_WIFI_PASSWORD  EEPROM_WIFI_SSID + WIFI_SSID_MAX_SIZE + 1           // 35
#define EEPROM_IP_ADDR        EEPROM_WIFI_PASSWORD + WIFI_PASSWORD_MAX_SIZE + 1   // 100
#define EEPROM_IP_NETMASK     EEPROM_IP_ADDR + IP_BYTE_SIZE                       // 104
#define EEPROM_MQTT_IP_ADDR   EEPROM_IP_NETMASK + IP_BYTE_SIZE                    // 108
#define EEPROM_MQTT_USER      EEPROM_MQTT_IP_ADDR + IP_BYTE_SIZE                  // 112
#define EEPROM_MQTT_PASSWORD  EEPROM_MQTT_USER + MQTT_USER_MAX_SIZE + 1           // 145
#define EEPROM_HOST_NAME      EEPROM_MQTT_PASSWORD + MQTT_PASSWORD_MAX_SIZE + 1   // 210
#define EEPROM_RESERVED       EEPROM_HOST_NAME + HOST_NAME_MAX_SIZE + 1           // 243
#define EEPROM_BP_RATING      EEPROM_RESERVED + RESERVED_SIZE                     // 343
#define EEPROM_CTRL_MODE      EEPROM_BP_RATING + BP_RATING_SIZE                   // 345
#define EEPROM_DIM_STYLE      EEPROM_CTRL_MODE + CTRL_MODE_SIZE                   // 346
#define EEPROM_SSR_PERIOD     EEPROM_DIM_STYLE + DIM_STYLE_SIZE                   // 347
#define EEPROM_NET_WD_TIMEOUT EEPROM_SSR_PERIOD + SSR_PERIOD_SIZE                 // 348
#define EEPROM_NET_WD_RECOVER EEPROM_NET_WD_TIMEOUT + NET_WD_TIMEOUT_SIZE         // 350
#define EEPROM_MQTT_INTERVAL  EEPROM_NET_WD_RECOVER + NET_WD_RECOVER_SIZE         // 352
#define EEPROM_ERROR_GAIN_POS EEPROM_MQTT_INTERVAL + MQTT_INTERVAL_SIZE           // 353
#define EEPROM_ERROR_GAIN_NEG EEPROM_ERROR_GAIN_POS + ERROR_GAIN_SIZE             // 357
#define EEPROM_STEP_CLAMP_POS EEPROM_ERROR_GAIN_NEG + ERROR_GAIN_SIZE             // 361
#define EEPROM_STEP_CLAMP_NEG EEPROM_STEP_CLAMP_POS + STEP_CLAMP_SIZE             // 365
#define EEPROM_DEAD_ZONE      EEPROM_STEP_CLAMP_NEG + STEP_CLAMP_SIZE             // 369
#define EEPROM_BUDGET_MARGIN  EEPROM_DEAD_ZONE + DEAD_ZONE_SIZE                   // 370
#define EEPROM_THERMOSTAT_SP  EEPROM_BUDGET_MARGIN + BUDGET_MARGIN_SIZE           // 372
#define EEPROM_DISINFECTION_EN EEPROM_THERMOSTAT_SP + THERMOSTAT_SP_SIZE          // 373
#define EEPROM_BUDGET_SRC     EEPROM_DISINFECTION_EN + DISINFECTION_EN_SIZE       // 374
#define EEPROM_THERMOSTAT_HYST EEPROM_BUDGET_SRC + BUDGET_SRC_SIZE                // 375
#define EEPROM_DZ_LOW_TEMP    EEPROM_THERMOSTAT_HYST + THERMOSTAT_HYST_SIZE       // 376
#define EEPROM_DZ_HIGH_TEMP   EEPROM_DZ_LOW_TEMP + DZ_LOW_TEMP_SIZE               // 377
#define EEPROM_DISINFECTION_INTERVAL EEPROM_DZ_HIGH_TEMP + DZ_HIGH_TEMP_SIZE         // 378
#define EEPROM_DISINFECTION_DURATION EEPROM_DISINFECTION_INTERVAL + DISINFECTION_INTERVAL_SIZE // 379
#define EEPROM_DISINFECTION_TEMP EEPROM_DISINFECTION_DURATION + DISINFECTION_DURATION_SIZE // 380
                                                                                  // end 381

// GPIO pin numbers
#define STATUS_LED 2        // Onboard LED (GPIO2 / D4 on NodeMCUv2)
#define ONE_WIRE 12         // One-wire temperature sensor (GPIO12 / D6 on NodeMCUv2)
#define TRIAC_OUTPUT 13     // Output to optocoupler + triac (GPIO13 / D7 on NodeMCUv2)
#define ZERO_CROSS_INPUT 14 // Input for zero-cross detection (GPIO14 / D5 on NodeMCUv2)

#endif // SYSTEM_H
