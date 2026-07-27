#pragma once
#ifndef TERM_PRINT_H
#define TERM_PRINT_H

#include "system.h"

#include <Arduino.h>
#ifdef SOCKET_SERVER_PORT
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  using NetClient = WiFiClient;
#elif defined(ESP32)
  #include <WiFi.h>
  using NetClient = WiFiClient;
#elif defined(ARDUINO_TEENSY41)
  #include <NativeEthernet.h>
  using NetClient = EthernetClient;
#endif
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
#ifdef SOCKET_SERVER_PORT
        if (socketServerClient_ && socketServerClient_->connected())
            socketServerClient_->print(x);
#endif
    }

    template<typename T>
    static void println(const T& x)
    {
        TERM_SERIAL.println(x);
#ifdef SOCKET_SERVER_PORT
        if (socketServerClient_ && socketServerClient_->connected())
            socketServerClient_->println(x);
#endif
    }

    template<typename T>
    static void printf(const T& x)
    {
        TERM_SERIAL.printf(x);
#ifdef SOCKET_SERVER_PORT
        if (socketServerClient_ && socketServerClient_->connected())
            socketServerClient_->printf(x);
#endif
    }

#ifdef SOCKET_SERVER_PORT
    static void SetSocketClient(NetClient& wifiClient)
    {
        socketServerClient_ = &wifiClient;
    }
#endif
  private:
#ifdef SOCKET_SERVER_PORT
    static NetClient* socketServerClient_;
#endif
};
#endif // TERM_PRINT_H