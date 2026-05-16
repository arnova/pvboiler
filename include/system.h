#ifndef SYSTEM_H
#define SYSTEM_H

#include "secrets.h"

#define CONTROL_LOOP_TIME_MS                    200   // ms

#define ZERO_CROSS_EDGE_MARGIN_US               100.0f // us

#define POWER_BUDGET_MIN                        10    // Watt

// Customize this for your boiler:
#define BOILER_POWER                            2500  // Watt

#define PERCENTAGE_CAP                          2.0f  // %

// Enable this if you want to control using setting power percentage instead of providing power budget
//#define POWER_PERCENTAGE_CONTROL

// Enable this to use SSR style mode instead of triac phase cut mode. This will blank/pass-through full periods like SSR does
#define SSR_STYLE_MODE

#define SSR_PERIOD_COUNT                        20 // Amount of (half) sinus / periods. Always use an even number!

#define GATE_PULSE_WIDTH                        50 // uS

// MQTT settings
#define MQTT_MAX_SIZE 1024
#define MQTT_NAME                              "pvboiler"

// (MQTT) home assistant settings
#define HA_DEVICE_NAME                         "PVBoiler"
#define HA_DEVICE_MODEL                        "PVBoiler Controller"
#define HA_MANUFACTURER                        "Arnova"

// WiFi settings
const char HOSTNAME[] = MQTT_NAME;
const char SSID[] = WIFI_SSID;          // Need to create + define in secrets.h
const char PASSWORD[] = WIFI_PASSWORD;  // Need to create + define in secrets.h

// MQTT server settings
const char mqtt_server[] = "192.168.1.65";
#define MQTT_PORT 1883

/**************************
 * Output i/o pin numbers *
 **************************/
#define STATUS_LED 2        // Onboard LED (GPIO2 / D4 on NodeMCUv2)
#define ZERO_CROSS_INPUT 13 // Input for zero-cross detection (GPIO13 / D7 on NodeMCUv2)
#define TRIAC_OUTPUT 14     // Output to optocoupler + triac (GPIO14 / D5  on NodeMCUv2)

#endif // SYSTEM_H
