#ifndef SYSTEM_H
#define SYSTEM_H

#include <Arduino.h>

// Version string:
#define MY_VERSION "0.11"

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
#define SSR_PERIOD_COUNT_MAX                    1000  // (= 0.5s @ 50 Hz).

// Proportional error gain
#define ERROR_GAIN_DEFAULT                      0.1f
#define ERROR_GAIN_MIN                          0.001f
#define ERROR_GAIN_MAX                          100.0f

// Minimum time for positive/negative zero crossing
#define ZERO_CROSS_EDGE_MARGIN_US               100.0f // us

// Triac gate pulse width
#define GATE_PULSE_WIDTH                        50 // uS

// Watchdog timer settings. Comment WATCHDOG_TIMEOUT_TIME define to disable
#define WATCHDOG_TIMEOUT_TIME                   900  // Seconds = 15 minutes
#define WATCHDOG_RECOVERY_TIME                  60   // Seconds = 1 minute

// Enable below for additional wifi debug messages
#define WIFI_DEBUG

// Wifi connect timeout
#define WIFI_CONNECT_TIMEOUT                    10000 // ms

// MQTT settings
#define MQTT_PORT                               1883
#define MQTT_UPDATE_TIME                        1     // Seconds
#define MQTT_MAX_SIZE                           1024

// Socket server settings
#define SOCKET_SERVER_PORT                      8000

// Misc. (home assistant) settings
#define HOST_NAME                              "pvboiler"
#define MQTT_NAME                              HOST_NAME
#define HA_DEVICE_NAME                         "PVBoiler"
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
#define IP_BYTE_SIZE          4
#define BP_RATING_SIZE        2
#define PB_MARGIN_SIZE        2
#define CTRL_MODE_SIZE        1
#define DIM_STYLE_SIZE        1
#define SSR_PERIOD_SIZE       1
#define WIFI_SSID_MAX_SIZE      32
#define WIFI_PASSWORD_MAX_SIZE  64

// EEPROM locations
#define EEPROM_BP_RATING      0
#define EEPROM_PB_MARGIN      EEPROM_BP_RATING + BP_RATING_SIZE
#define EEPROM_CTRL_MODE      EEPROM_PB_MARGIN + PB_MARGIN_SIZE
#define EEPROM_DIM_STYLE      EEPROM_CTRL_MODE + CTRL_MODE_SIZE
#define EEPROM_SSR_PERIOD     EEPROM_DIM_STYLE + DIM_STYLE_SIZE
#define EEPROM_ERROR_GAIN     EEPROM_SSR_PERIOD + SSR_PERIOD_SIZE
#define EEPROM_WIFI_SSID      EEPROM_ERROR_GAIN + sizeof(float)
#define EEPROM_WIFI_PASSWORD  EEPROM_WIFI_SSID + WIFI_SSID_MAX_SIZE + 1
#define EEPROM_IP_ADDR        EEPROM_WIFI_PASSWORD + WIFI_PASSWORD_MAX_SIZE + 1
#define EEPROM_IP_NETMASK     EEPROM_IP_ADDR + IP_BYTE_SIZE
#define EEPROM_SERVER_IP_ADDR EEPROM_IP_NETMASK + IP_BYTE_SIZE

#endif // SYSTEM_H
