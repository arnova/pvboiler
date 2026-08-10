# PvBoiler

This project is a complete hardware, firmware & software solution to turn your "dumb" electronic hot water boiler into a smart **PvBoiler**. It aims to optimize the amount of excess solar electricity that can be stored in a boiler as hot water using as little grid power as possible.

Most other solutions use SSR modules for power control. This design uses a (BTA25/BTA40) **triac** for power control to enable **phase-angle control**, which allows for more accurate power control than with a normal SSR.

The hardware was designed using **KiCAD**, and the software uses **MQTT** for communication, targeted to be used with **Home Assistant**. The hardware and firmware were developed for a NodeMCU v2 (Amica) ESP8266 module, but should in principle work with any ESP8266 or ESP32 board. Obviously ESP boards with alternative formfactors and/or pinouts cannot be directly fitted on the PCB but should be connected using wires and you may need to modify platformio.ini for your specific ESP board.

## Features

- Home Assistant (MQTT) support
- Phase angle control (default) or SSR dim style
- Configuration via USB serial connection or network terminal connection (port 8000)
- OTA updates (e.g. via PlatformIO)
- Output control using automatic power budget mode, or manually by setting output power percentage. The "logic-mode" can be dynamically changed using MQTT
- Boost mode to temporarely set output power to 100%. Can be used for external (eg. using Home Assistant) legionella prevention control
- Support for an optional OLED 128×64 screen to display output power/percentage and connection status
- The controller has a network watchdog. If there hasn't been any MQTT traffic to the controller for a while (when eg. network fails), the output will automatically decrease to 0% instead of being stuck on the last value. This behaviour can be disabled/configured with the `netwdt` and `netwdr` commands.

## Planned features & improvements

- Automatic legionella prevention using an external one wire temperature sensor
- Improve control loop for budget logic mode
- Standalone support to directly interface with MQTT P1 providers like DSMR Reader

## Hardware Assembly Hints

1. In the pictures-folder of this project you can find photos of my assembled enclosure which can be used as a guideline to build your own
2. Use a sufficiently sized heatsink with a little thermal compound for mounting the BTA-triac. Generally it is recommended to use a heatsink with a thermal resistance better than 1.0C/W when using a ~2500W boiler
3. Around some power traces the mask has been intentionally removed. You should solder these traces with extra solder to reduce the power losses due to trace resistance
4. It is recommended to use 1.5mm2 wires for internal wiring

## First Time Use

1. Flash the ESP firmware using e.g. PlatformIO (future firmware updates can be done over-the-air (OTA)). You may need to customize `platform` and `board` in `platformio.ini` for your specific ESP board.
2. After the initial flash, you can change the firmware upload mechanism in `platformio.ini` from serial to OTA by uncommenting `upload_protocol = espota` and `upload_port = pvboiler.local`. Note that you *may* need to substitute the controller's IP address for `pvboiler.local` in case mDNS is not available in your network (or it's failing somehow).
3. Connect to the microcontroller's terminal interface, either via the USB connection or a socket connection to the device's IP at port 8000, using a terminal program (e.g. PuTTY).
4. In the command terminal, `help` (+ <kbd>Enter</kbd>) will show all available commands with their descriptions. Initially, these operations must be performed:
   - Reset all settings to default with the `factoryreset`-command.
   - Set your WiFi network SSID with the `ssid`-command, providing the name of your network as an argument.
   - Set your WiFi network password with the `pass`-command, providing your network's security password as an argument. You may provide a zero-length value when your network requires no password.
   - Set your MQTT server IP with the `serverip`-command, providing the server IP as an argument.
   - Set your MQTT server username with the `mqttuser`-command. You may provide a zero-length value when your server requires no username.
   - Set your MQTT server password with the `mqttpass`-command. You may provide a zero-length value when your server requires no password.
   - Set boiler power rating with the `boiler`-command, providing the power rating of your boiler as an argument (the factory default is 2500W).

> **Notes**
> - Only change other settings when you understand what they do!
> - When using budget logic mode, you must regularly (at least 5 second intervals are recommended) tell the PvBoiler controller how much "budget" (= excess solar PV electricity) is available, e.g. from Home Assistant. A sample automation for Home Assistant can be found in the `home assistant` folder.

---

⚠️ **This project is provided "as-is" and is to be used entirely at your own risk!** Please note the hardware involves hazardous voltages that may be lethal.
