#pragma once
#ifndef TERMINAL_H
#define TERMINAL_H

#include "system.h"

#ifdef SOCKET_SERVER_PORT
#include "Network.h"
#endif

#include <Arduino.h>

#ifndef TERM_SERIAL
#define TERM_SERIAL Serial
#endif

#ifndef CMD_BUF_SIZE
#define CMD_BUF_SIZE 80
#endif

#ifndef RESULT_BUF_SIZE
#define RESULT_BUF_SIZE 80
#endif

// Socket (network) terminal login. Only used when TERMINAL_PASSWORD is defined
#ifndef TERMINAL_LOGIN_BUF_SIZE
#define TERMINAL_LOGIN_BUF_SIZE 33 // Max. password length + 1
#endif

#ifndef TERMINAL_LOGIN_MAX_ATTEMPTS
#define TERMINAL_LOGIN_MAX_ATTEMPTS 3 // Client is disconnected after this many failed attempts
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
      if (IsSocketOutputEnabled())
          g_socketServerClient->print(x);
#endif
    }

    template<typename T>
    static void println(const T& x)
    {
      TERM_SERIAL.println(x);
#ifdef SOCKET_SERVER_PORT
      if (IsSocketOutputEnabled())
          g_socketServerClient->println(x);
#endif
    }

    // Print x on serial only, socket gets xSocket instead (eg. to hide sensitive data)
    template<typename T, typename U>
    static void printSerialOnly(const T& x, const U& xSocket)
    {
      TERM_SERIAL.print(x);
#ifdef SOCKET_SERVER_PORT
      if (IsSocketOutputEnabled())
          g_socketServerClient->print(xSocket);
#else
      (void) xSocket;
#endif
    }

    template<typename T>
    static void printf(const T& x)
    {
        TERM_SERIAL.printf(x);
#ifdef SOCKET_SERVER_PORT
        if (IsSocketOutputEnabled())
            g_socketServerClient->printf(x);
#endif
    }

#ifdef SOCKET_SERVER_PORT
    static void SetSocketClient(NetClient& wifiClient);
#endif

  private:
#ifdef SOCKET_SERVER_PORT
    // Socket output is only allowed when connected and logged in (when login is enabled)
    static bool IsSocketOutputEnabled() { return g_socketServerClient && g_socketServerClient->connected() && g_bSocketLoggedIn; };

#ifdef TERMINAL_PASSWORD
    static void ProcessLogin(NetClient& client, const char rxChar);
#endif

    CNetwork& m_network;
    static NetClient* g_socketServerClient;
    static bool g_bSocketLoggedIn;

#ifdef TERMINAL_PASSWORD
    static char g_strLoginBuf[TERMINAL_LOGIN_BUF_SIZE];
    static size_t g_iLoginBufCount;
    static uint8_t g_iLoginAttempts;
#endif
#endif

    volatile static bool g_bEchoOnOff;

    volatile rx_data_t m_termRxData1 = { };
    volatile rx_data_t m_termRxData2 = { };

    volatile rx_data_t* m_pActiveTermRxData = &m_termRxData1;
    volatile rx_data_t* m_pInactiveTermRxData = &m_termRxData2;
};
#endif // TERMINAL_H