#pragma once
#ifndef TERMINAL_H
#define TERMINAL_H

#include "system.h"

#ifdef SOCKET_SERVER_PORT
#include "Network.h"
#endif

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

#ifndef CMD_BUF_SIZE
#define CMD_BUF_SIZE 80
#endif

class CTerminal
{
  public:
    enum rx_state_e
    {
      RX_STATE_DONE = 0,
      RX_STATE_FILLING,
      RX_STATE_READY
    };

    struct rx_data_s
    {
      enum rx_state_e state;
      char buf[CMD_BUF_SIZE];
      size_t buf_count;
    };
    typedef struct rx_data_s rx_data_t;

#ifdef SOCKET_SERVER_PORT
    CTerminal(CNetwork& network); // Constructor
#else
    CTerminal(); // Constructor
#endif

    void Process();
    bool IsCommandReady() { return (m_pInactiveTermRxData->state == RX_STATE_READY); };
    char *GetCommand(bool bProgress = true);
    static bool GetLocalEchoEnabled() { return g_bEchoOnOff; };
    static void SetLocalEcho(const bool bEnable) { g_bEchoOnOff = bEnable; };

    template<typename T>
    static void print(const T& x)
    {
      TERM_SERIAL.print(x);
#ifdef SOCKET_SERVER_PORT
      if (g_socketServerClient && g_socketServerClient->connected())
          g_socketServerClient->print(x);
#endif
    }

    template<typename T>
    static void println(const T& x)
    {
      TERM_SERIAL.println(x);
#ifdef SOCKET_SERVER_PORT
      if (g_socketServerClient && g_socketServerClient->connected())
          g_socketServerClient->println(x);
#endif
    }

    template<typename T>
    static void printf(const T& x)
    {
        TERM_SERIAL.printf(x);
#ifdef SOCKET_SERVER_PORT
        if (g_socketServerClient && g_socketServerClient->connected())
            g_socketServerClient->printf(x);
#endif
    }

#ifdef SOCKET_SERVER_PORT
    static void SetSocketClient(NetClient& wifiClient)
    {
      g_socketServerClient = &wifiClient;
    }
#endif

  private:
#ifdef SOCKET_SERVER_PORT
    CNetwork& m_network;
   static NetClient* g_socketServerClient;
#endif

    volatile static bool g_bEchoOnOff;

    volatile rx_data_t m_termRxData1 = { };
    volatile rx_data_t m_termRxData2 = { };

    volatile rx_data_t* m_pActiveTermRxData = &m_termRxData1;
    volatile rx_data_t* m_pInactiveTermRxData = &m_termRxData2;
};
#endif // TERMINAL_H