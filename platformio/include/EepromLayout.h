#pragma once
#ifndef EEPROM_LAYOUT_H
#define EEPROM_LAYOUT_H

#include <Arduino.h>

#include "EepromSettings.h"

// EEPROM byte sizes
#define EEPROM_MAX_SIZE         512
#define CHECKSUM_SIZE           2
#define RESERVED_SIZE           100
#define WIFI_SSID_MAX_SIZE      32
#define WIFI_PASSWORD_MAX_SIZE  64
#define HOST_NAME_MAX_SIZE      32
#define MQTT_USER_MAX_SIZE      32
#define MQTT_PASSWORD_MAX_SIZE  64
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
#define EEPROM_CHECKSUM              0                                                              // 0
#define EEPROM_WIFI_SSID             (EEPROM_CHECKSUM + CHECKSUM_SIZE)                              // 2
#define EEPROM_WIFI_PASSWORD         (EEPROM_WIFI_SSID + WIFI_SSID_MAX_SIZE + 1)                    // 35
#define EEPROM_IP_ADDR               (EEPROM_WIFI_PASSWORD + WIFI_PASSWORD_MAX_SIZE + 1)            // 100
#define EEPROM_IP_NETMASK            (EEPROM_IP_ADDR + EEPROM_IP_SIZE)                              // 104
#define EEPROM_MQTT_IP_ADDR          (EEPROM_IP_NETMASK + EEPROM_IP_SIZE)                           // 108
#define EEPROM_MQTT_USER             (EEPROM_MQTT_IP_ADDR + EEPROM_IP_SIZE)                         // 112
#define EEPROM_MQTT_PASSWORD         (EEPROM_MQTT_USER + MQTT_USER_MAX_SIZE + 1)                    // 145
#define EEPROM_HOST_NAME             (EEPROM_MQTT_PASSWORD + MQTT_PASSWORD_MAX_SIZE + 1)            // 210
#define EEPROM_RESERVED              (EEPROM_HOST_NAME + HOST_NAME_MAX_SIZE + 1)                    // 243
#define EEPROM_BP_RATING             (EEPROM_RESERVED + RESERVED_SIZE)                              // 343
#define EEPROM_CTRL_MODE             (EEPROM_BP_RATING + BP_RATING_SIZE)                            // 345
#define EEPROM_DIM_STYLE             (EEPROM_CTRL_MODE + CTRL_MODE_SIZE)                            // 346
#define EEPROM_SSR_PERIOD            (EEPROM_DIM_STYLE + DIM_STYLE_SIZE)                            // 347
#define EEPROM_NET_WD_TIMEOUT        (EEPROM_SSR_PERIOD + SSR_PERIOD_SIZE)                          // 348
#define EEPROM_NET_WD_RECOVER        (EEPROM_NET_WD_TIMEOUT + NET_WD_TIMEOUT_SIZE)                  // 350
#define EEPROM_MQTT_INTERVAL         (EEPROM_NET_WD_RECOVER + NET_WD_RECOVER_SIZE)                  // 352
#define EEPROM_ERROR_GAIN_POS        (EEPROM_MQTT_INTERVAL + MQTT_INTERVAL_SIZE)                    // 353
#define EEPROM_ERROR_GAIN_NEG        (EEPROM_ERROR_GAIN_POS + ERROR_GAIN_SIZE)                      // 357
#define EEPROM_STEP_CLAMP_POS        (EEPROM_ERROR_GAIN_NEG + ERROR_GAIN_SIZE)                      // 361
#define EEPROM_STEP_CLAMP_NEG        (EEPROM_STEP_CLAMP_POS + STEP_CLAMP_SIZE)                      // 365
#define EEPROM_DEAD_ZONE             (EEPROM_STEP_CLAMP_NEG + STEP_CLAMP_SIZE)                      // 369
#define EEPROM_BUDGET_MARGIN         (EEPROM_DEAD_ZONE + DEAD_ZONE_SIZE)                            // 370
#define EEPROM_THERMOSTAT_SP         (EEPROM_BUDGET_MARGIN + BUDGET_MARGIN_SIZE)                    // 372
#define EEPROM_DISINFECTION_EN       (EEPROM_THERMOSTAT_SP + THERMOSTAT_SP_SIZE)                    // 373
#define EEPROM_BUDGET_SRC            (EEPROM_DISINFECTION_EN + DISINFECTION_EN_SIZE)                // 374
#define EEPROM_THERMOSTAT_HYST       (EEPROM_BUDGET_SRC + BUDGET_SRC_SIZE)                          // 375
#define EEPROM_DZ_LOW_TEMP           (EEPROM_THERMOSTAT_HYST + THERMOSTAT_HYST_SIZE)                // 376
#define EEPROM_DZ_HIGH_TEMP          (EEPROM_DZ_LOW_TEMP + DZ_LOW_TEMP_SIZE)                        // 377
#define EEPROM_DISINFECTION_INTERVAL (EEPROM_DZ_HIGH_TEMP + DZ_HIGH_TEMP_SIZE)                      // 378
#define EEPROM_DISINFECTION_DURATION (EEPROM_DISINFECTION_INTERVAL + DISINFECTION_INTERVAL_SIZE)    // 379
#define EEPROM_DISINFECTION_TEMP     (EEPROM_DISINFECTION_DURATION + DISINFECTION_DURATION_SIZE)    // 380
#define EEPROM_END                   (EEPROM_DISINFECTION_TEMP + DISINFECTION_TEMP_SIZE)            // 381

static_assert(EEPROM_END <= EEPROM_MAX_SIZE, "EEPROM layout exceeds EEPROM_MAX_SIZE");

#endif // EEPROM_LAYOUT_H
