#pragma once
#ifndef TERM_PRINT_H
#define TERM_PRINT_H

#include "system.h"

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  using NetClient = WiFiClient;
#elif defined(ESP32)
  #include <WiFi.h>
  #include <ESPmDNS.h>
  using NetClient = WiFiClient;
#elif defined(ARDUINO_TEENSY41)
  #include <NativeEthernet.h>
  using NetClient = EthernetClient;
#endif

#ifndef TERM_SERIAL
#define TERM_SERIAL Serial
#endif

class CTermPrint
{
  public:
    template<typename T>
    static void print(const T& x)
    {
        TERM_SERIAL.print(x);
        if (socketServerClient_ && socketServerClient_->connected())
            socketServerClient_->print(x);
    }

    template<typename T>
    static void println(const T& x)
    {
        TERM_SERIAL.println(x);
        if (socketServerClient_ && socketServerClient_->connected())
            socketServerClient_->println(x);
    }

    template<typename T>
    static void printf(const T& x)
    {
        TERM_SERIAL.printf(x);
        if (socketServerClient_ && socketServerClient_->connected())
            socketServerClient_->printf(x);
    }

    static void SetSocketClient(NetClient& wifiClient)
    {
        socketServerClient_ = &wifiClient;
    }

  private:
    static NetClient* socketServerClient_;
};

#endif // TERM_PRINT_H