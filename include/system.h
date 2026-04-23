#ifndef SYSTEM_H
#define SYSTEM_H

#include "secrets.h"

#define CONTROL_LOOP_TIME_MS                    100   // ms

#define ZERO_CROSS_EDGE_MARGIN_US               100   // us

#define POWER_BUDGET_MIN                        10    // Watt

// Customize this for your boiler:
#define BOILER_POWER                            2000  // Watt

#define PERCENTAGE_CAP                          2.0 // %

// Enable this if you want to control using setting power percentage instead of providing power budget
//#define POWER_PERCENTAGE_CONTROL

// WiFi settings
const char HOSTNAME[] = "pvboiler";
const char SSID[] = WIFI_SSID;          // Need to create + define in secrets.h
const char PASSWORD[] = WIFI_PASSWORD;  // Need to create + define in secrets.h

// MQTT server settings
const char mqtt_server[] = "192.168.1.65";
#define MQTT_PORT 1883

/**************************
 * Output i/o pin numbers *
 **************************/
#define LED_RED 25          // Onboard LED
#define TRIAC_OUTPUT 13     // Output to optocoupler + triac
#define ZERO_CROSS_INPUT 1  // Input for zero-cross detection

#endif // SYSTEM_H
