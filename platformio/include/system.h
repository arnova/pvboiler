#pragma once
#ifndef SYSTEM_H
#define SYSTEM_H

#include <Arduino.h>

// Version string:
#define MY_VERSION "1.10"

// Firmware version string
const char VER_STR_P[] PROGMEM = "PV-Boiler Controller " MY_VERSION " - (C) 2026 Arno van Amersfoort (Arnova)";

// Misc. (home assistant) settings
#define DEFAULT_HOST_NAME                       "PvBoiler"
#define DEVICE_NAME                             "PV-Boiler"
#define DEVICE_MODEL                            "PV-Boiler Controller"
#define MANUFACTURER                            "Arnova"

// Triac gate pulse width
#define GATE_PULSE_WIDTH                        50    // uS

// Triac phase margin we keep to prevent misfires
#define TRIAC_PHASE_ANGLE_MARGIN_US             500   // us

// Maximum amount of net period & zero cross window measurement outliers
#define MAX_CONSECUTIVE_OUTLIERS                8

// Maximum time elapsed before we flag an actual power error
#define POWER_GOOD_TIME_MAX                     1000  // ms

// Temperature related
#define TEMPERATURE_MAX_RETRIES                 255
#define TEMPERATURE_OVERHEATING_MAX             90.0f // Celsius

// Enable below for additional wifi / mqtt debug messages
#define WIFI_DEBUG
//#define MQTT_DEBUG

// Settle time after (re)connecting/subscribing: lets the broker resend retained setters (before we publish our own)
// and HA discover new entities (before we publish their values)
#define MQTT_SETTLE_TIME_MS                     5000  // ms

// Socket server port. Uncomment to disable
#define SOCKET_SERVER_PORT                      8000

// UART/socket settings
#define BAUD_RATE                               115200

// GPIO pin numbers
#define STATUS_LED 2        // Onboard LED (GPIO2 / D4 on NodeMCUv2)
#define ONE_WIRE 12         // One-wire temperature sensor (GPIO12 / D6 on NodeMCUv2)
#define TRIAC_OUTPUT 13     // Output to optocoupler + triac (GPIO13 / D7 on NodeMCUv2)
#define ZERO_CROSS_INPUT 14 // Input for zero-cross detection (GPIO14 / D5 on NodeMCUv2)

#endif // SYSTEM_H
