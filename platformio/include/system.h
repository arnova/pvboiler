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

// Misc. (home assistant) settings
#define DEFAULT_HOST_NAME       "PvBoiler"
#define DEVICE_NAME             "PV-Boiler"
#define DEVICE_MODEL            "PV-Boiler Controller"
#define MANUFACTURER            "Arnova"

// Socket server settings
#define SOCKET_SERVER_PORT      8000

// UART/socket settings
#define BAUD_RATE               115200
#define CMD_BUF_SIZE            80
#define RESULT_BUF_SIZE         80

// GPIO pin numbers
#define STATUS_LED 2        // Onboard LED (GPIO2 / D4 on NodeMCUv2)
#define ONE_WIRE 12         // One-wire temperature sensor (GPIO12 / D6 on NodeMCUv2)
#define TRIAC_OUTPUT 13     // Output to optocoupler + triac (GPIO13 / D7 on NodeMCUv2)
#define ZERO_CROSS_INPUT 14 // Input for zero-cross detection (GPIO14 / D5 on NodeMCUv2)

#endif // SYSTEM_H
