#pragma once
#ifndef SYSTEM_H
#define SYSTEM_H

#include <Arduino.h>

// Version string:
#define MY_VERSION "1.01"

// Firmware version string
const char VER_STR_P[] PROGMEM = "PvBoiler " MY_VERSION " - (C) 2026 Arno van Amersfoort (Arnova)";

// Boiler power rating
#define BOILER_POWER_RATING_DEFAULT             2500  // Watt
#define BOILER_POWER_RATING_MAX                 10000 // Watt

// The amount of +/- margin for power budget
#define POWER_BUDGET_MARGIN_DEFAULT             10    // Watt
#define POWER_BUDGET_MARGIN_MAX                 1000  // Watt

// Amount of (half) sinus / periods when ssr style mode is used. Always use an even number!
#define SSR_PERIOD_COUNT_DEFAULT                50    // (= 0.5s @ 50 Hz).
#define SSR_PERIOD_COUNT_MAX                    254

// Proportional error gain
#define ERROR_GAIN_DEFAULT                      0.01f
#define ERROR_GAIN_MIN                          0.0001f
#define ERROR_GAIN_MAX                          100.0f

#define ERROR_CLAMP_DEFAULT                     10
#define ERROR_CLAMP_MIN                         1
#define ERROR_CLAMP_MAX                         100

// Initial value for phase correction time
#define ZERO_CROSS_WINDOW_DEFAULT               1100 // uS

// Minimum time for positive/negative zero crossing
#define ZERO_CROSS_EDGE_MIN_US                  500 // us

// Maximum time for positive/negative zero crossing
#define ZERO_CROSS_EDGE_MAX_US                  2000 // us

// Maximum net period time
#define NET_PERIOD_MAX_US                       12000 // us

// Triac gate pulse width
#define GATE_PULSE_WIDTH                        50 // uS

// Network watchdog timer settings
#define NETWORK_WATCHDOG_TIMEOUT_DEFAULT        900  // Seconds = 15 minutes
#define NETWORK_WATCHDOG_TIMEOUT_MAX            65000

#define NETWORK_WATCHDOG_RECOVERY_DEFAULT       60   // Seconds = 1 minute
#define NETWORK_WATCHDOG_RECOVERY_MAX           65000

// Enable below for additional wifi / mqtt debug messages
#define WIFI_DEBUG
//#define MQTT_DEBUG

// Wifi connect timeout
#define WIFI_CONNECT_TIMEOUT                    10000 // ms

// MQTT connect timeout
#define MQTT_CONNECT_TIMEOUT                    10000 // ms

// MQTT settings
#define MQTT_PORT                               1883
#define MQTT_UPDATE_TIME                        15    // Seconds
#define MQTT_MAX_MESSAGE_SIZE                   1024
#define MQTT_MAX_TOPIC_ITEM_SIZE                32
#define MQTT_MAX_CONFIG_TOPIC_SIZE              128

// Control topic items
#define MQTT_CONTROLLER_ON_OFF                  "controller_enable"

#define MQTT_SET_POWER_PERCENTAGE               "power_percentage"
#define MQTT_SET_POWER_BUDGET                   "power_budget"

// Status topic items
#define MQTT_FW_VERSION                         "firmware_version"
#define MQTT_OUTPUT_POWER                       "output_power"
#define MQTT_OUTPUT_PERCENTAGE                  "output_percentage"

#define MQTT_LOGIC_MODE                         "logic_mode"
#define MQTT_BOILER_POWER_RATING                       "boiler_power_rating"
#define MQTT_BUDGET_MARGIN                      "budget_margin"
#define MQTT_DIM_STYLE                          "dim_style"
#define MQTT_SSR_PERIOD_COUNT                   "ssr_period_count"
#define MQTT_ERROR_GAIN                         "error_gain"
#define MQTT_ERROR_CLAMP                        "error_clamp"

// Diagnostic topic items
#define MQTT_WIFI_SSID                          "wifi_ssid"
#define MQTT_IP_ADDRESS                         "ip_address"
#define MQTT_IP_NETMASK                         "ip_netmask"
#define MQTT_PHASE_ANGLE_FACTOR                 "phase_angle_factor"
#define MQTT_PHASE_ANGLE                        "phase_angle"
#define MQTT_NET_PERIOD                         "net_period"
#define MQTT_NET_FREQUENCY                      "net_frequency"
#define MQTT_ZERO_CROSS_WINDOW                  "zero_cross_window"
#define MQTT_POWER_ERROR                        "power_error"
#define MQTT_NET_WD_TIMEOUT                     "network_watchdog_timeout"
#define MQTT_NET_WD_RECOVERY                    "network_watchdog_recovery"
#define MQTT_UP_TIME                            "up_time"

// Socket server settings
#define SOCKET_SERVER_PORT                      8000

// Misc. (home assistant) settings
#define HOST_NAME                              "pvboiler"
#define MQTT_NAME                              HOST_NAME
#define DEVICE_NAME                            "PVBoiler"
#define HA_DEVICE_NAME                         DEVICE_NAME
#define HA_DEVICE_MODEL                        "PVBoiler Controller"
#define HA_MANUFACTURER                        "Arnova"

/**************************
 * Output i/o pin numbers *
 **************************/
#define STATUS_LED 2        // Onboard LED (GPIO2 / D4 on NodeMCUv2)
#define TRIAC_OUTPUT 13     // Output to optocoupler + triac (GPIO13 / D7  on NodeMCUv2)
#define ZERO_CROSS_INPUT 14 // Input for zero-cross detection (GPIO14 / D5 on NodeMCUv2)

#define BAUD_RATE                     115200
#define CMD_BUF_SIZE                  80
#define RESULT_BUF_SIZE               80
#define SOCKET_CLIENT_TIMEOUT_MS      100 // ms

// EEprom byte sizes
#define IP_BYTE_SIZE            4
#define BP_RATING_SIZE          2
#define PB_MARGIN_SIZE          2
#define CTRL_MODE_SIZE          1
#define DIM_STYLE_SIZE          1
#define SSR_PERIOD_SIZE         1
#define WIFI_SSID_MAX_SIZE      32
#define WIFI_PASSWORD_MAX_SIZE  64
#define ERROR_GAIN_SIZE         sizeof(float)
#define ERROR_CLAMP_SIZE        1
#define NET_WD_TIMEOUT_SIZE     2
#define NET_WD_RECOVER_SIZE     2

// EEPROM locations
#define EEPROM_WIFI_SSID      0                                                   // offset 32
#define EEPROM_WIFI_PASSWORD  EEPROM_WIFI_SSID + WIFI_SSID_MAX_SIZE + 1           // offset 96
#define EEPROM_IP_ADDR        EEPROM_WIFI_PASSWORD + WIFI_PASSWORD_MAX_SIZE + 1   // offset 100
#define EEPROM_IP_NETMASK     EEPROM_IP_ADDR + IP_BYTE_SIZE                       // offset 104
#define EEPROM_SERVER_IP_ADDR EEPROM_IP_NETMASK + IP_BYTE_SIZE                    // offset 108
#define EEPROM_BP_RATING      EEPROM_SERVER_IP_ADDR + IP_BYTE_SIZE                // offset 110
#define EEPROM_PB_MARGIN      EEPROM_BP_RATING + BP_RATING_SIZE                   // offset 112
#define EEPROM_CTRL_MODE      EEPROM_PB_MARGIN + PB_MARGIN_SIZE                   // offset 113
#define EEPROM_DIM_STYLE      EEPROM_CTRL_MODE + CTRL_MODE_SIZE                   // offset 114
#define EEPROM_SSR_PERIOD     EEPROM_DIM_STYLE + DIM_STYLE_SIZE                   // offset 115
#define EEPROM_ERROR_GAIN     EEPROM_SSR_PERIOD + SSR_PERIOD_SIZE                 // offset 119
#define EEPROM_ERROR_CLAMP    EEPROM_ERROR_GAIN + ERROR_GAIN_SIZE                 // offset 120
#define EEPROM_NET_WD_TIMEOUT EEPROM_ERROR_CLAMP + ERROR_CLAMP_SIZE               // offset 122
#define EEPROM_NET_WD_RECOVER EEPROM_NET_WD_TIMEOUT + NET_WD_TIMEOUT_SIZE         // offset 124

// Timer1 at DIV1 (80 MHz clock) → 80 ticks per µs on esp8266
// Maximum ~104 ms at this prescaler; no need for DIV256 in our range.
#define ESP8266_TICKS_PER_US  80

#endif // SYSTEM_H
