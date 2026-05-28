#ifndef SYSTEM_H
#define SYSTEM_H

#include "secrets.h"

// Version string:
#define MY_VERSION "0.11"

// Customize this for your boiler:
#define BOILER_POWER                            2500  // Watt

// Enable this if you want to control using setting power percentage instead of providing power budget
//#define POWER_PERCENTAGE_CONTROL

// Enable this to use SSR style mode instead of triac phase cut mode. This will blank/pass-through full periods like an SSR does
#define SSR_STYLE_MODE

// Amount of (half) sinus / periods when ssr style mode is used. Always use an even number!
#define SSR_PERIOD_COUNT                        50 // (= 0.5s @ 50 Hz).

// The amount of +/- margin for power budget
#define POWER_BUDGET_MARGIN                     10   // Watt

// Proportional error gain
#define KP                                      0.1f

// Minimum time for positive/negative zero crossing
#define ZERO_CROSS_EDGE_MARGIN_US               100.0f // us

// Triac gate pulse width
#define GATE_PULSE_WIDTH                        50 // uS

// Watchdog timer settings. Comment WATCHDOG_TIMEOUT_TIME define to disable
#define WATCHDOG_TIMEOUT_TIME                   900  // Seconds = 15 minutes
#define WATCHDOG_RECOVERY_TIME                  60   // Seconds = 1 minute

// Customize your MQTT server settings here
#define MQTT_SERVER                             "192.168.1.65"
#define MQTT_PORT                               1883

// MQTT settings
#define MQTT_UPDATE_TIME                        1     // Seconds
#define MQTT_MAX_SIZE                           1024
#define MQTT_NAME                              "pvboiler"

// (MQTT) home assistant settings
#define HA_DEVICE_NAME                         "PVBoiler"
#define HA_DEVICE_MODEL                        "PVBoiler Controller"
#define HA_MANUFACTURER                        "Arnova"

// WiFi settings
const char HOSTNAME[] = MQTT_NAME;
const char SSID[] = WIFI_SSID;          // Need to create + define in secrets.h
const char PASSWORD[] = WIFI_PASSWORD;  // Need to create + define in secrets.h

/**************************
 * Output i/o pin numbers *
 **************************/
#define STATUS_LED 2        // Onboard LED (GPIO2 / D4 on NodeMCUv2)
#define TRIAC_OUTPUT 13     // Output to optocoupler + triac (GPIO13 / D7  on NodeMCUv2)
#define ZERO_CROSS_INPUT 14 // Input for zero-cross detection (GPIO14 / D5 on NodeMCUv2)

#endif // SYSTEM_H
