#pragma once
#ifndef SETTING_LIMITS_H
#define SETTING_LIMITS_H

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

// Thermostat related
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

// Zero crossing window related values
#define ZERO_CROSS_WINDOW_MIN_US                700   // us
#define ZERO_CROSS_WINDOW_MAX_US                1300  // us
#define ZERO_CROSS_WINDOW_INVALID               65535 // us

// Net period time related values (for (rectified) 49-62 Hz half-period)
#define NET_PERIOD_MIN_US                       8000  // us
#define NET_PERIOD_MAX_US                       11000 // us
#define NET_PERIOD_INVALID                      65535 // us

#endif // SETTING_LIMITS_H
