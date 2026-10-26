#pragma once
#ifndef MQTT_TOPICS_H
#define MQTT_TOPICS_H

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

#endif // MQTT_TOPICS_H
