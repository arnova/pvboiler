/* 
  Terminal Class
  (C) Copyright 2026

  Written by       : Arno van Amersfoort
  Dependencies     : util
  Initial date     : July 30, 2026
  Last modified    : October 26, 2026
*/

#include "Terminal.h"
#include "util.h"

#ifdef SOCKET_SERVER_PORT
NetClient* CTerminal::g_socketServerClient = nullptr;   // out-of-class definition
bool CTerminal::g_bSocketLoggedIn = true;               // Without TERMINAL_PASSWORD always logged in

#ifdef TERMINAL_PASSWORD
char CTerminal::g_strLoginBuf[TERMINAL_LOGIN_BUF_SIZE] = { 0 };
size_t CTerminal::g_iLoginBufCount = 0;
uint8_t CTerminal::g_iLoginAttempts = 0;
#endif
#endif

volatile bool CTerminal::g_bEchoOnOff = true; // out-of-class definition

#ifdef SOCKET_SERVER_PORT
CTerminal::CTerminal(CNetwork& network) : m_network(network)
#else
CTerminal::CTerminal()
#endif
{
}


#ifdef SOCKET_SERVER_PORT
void CTerminal::SetSocketClient(NetClient& wifiClient)
{
  g_socketServerClient = &wifiClient;

#ifdef TERMINAL_PASSWORD
  // New client: require login
  g_bSocketLoggedIn = false;
  g_iLoginBufCount = 0;
  g_iLoginAttempts = 0;

  wifiClient.print("Password: ");
#endif
}
#endif


#if defined(SOCKET_SERVER_PORT) && defined(TERMINAL_PASSWORD)
// Handle (non-echoed) password input of a socket client that isn't logged in yet
void CTerminal::ProcessLogin(NetClient& client, const char rxChar)
{
  if (rxChar == CH_CR || rxChar == CH_LF)
  {
    if (g_iLoginBufCount == 0)
      return; // Ignore empty lines (eg. LF of CR+LF)

    g_strLoginBuf[g_iLoginBufCount] = '\0';
    const bool bMatch = STREQUALS(g_strLoginBuf, TERMINAL_PASSWORD);

    // Don't keep the entered password in memory
    memset(g_strLoginBuf, 0, sizeof(g_strLoginBuf));
    g_iLoginBufCount = 0;

    if (bMatch)
    {
      g_bSocketLoggedIn = true;
      client.println("");
      client.println(FPSTR(VER_STR_P));
    }
    else if (++g_iLoginAttempts >= TERMINAL_LOGIN_MAX_ATTEMPTS)
    {
      client.println("");
      client.println("Login failed");
      client.stop();

#ifdef WIFI_DEBUG
      TERM_SERIAL.println("Socket terminal: Login failed, client disconnected");
#endif
    }
    else
    {
      client.println("");
      client.print("Wrong password. Password: ");
    }
  }
  else if (rxChar == CH_DELETE || rxChar == CH_BACKSPACE)
  {
    if (g_iLoginBufCount != 0)
      g_iLoginBufCount--;
  }
  else if (rxChar >= ' ' && rxChar <= '~' && g_iLoginBufCount < (TERMINAL_LOGIN_BUF_SIZE - 1))
  {
    g_strLoginBuf[g_iLoginBufCount++] = rxChar;
  }
}
#endif


void CTerminal::Process(void)
{
  char rxChar = 0x00;

  if (TERM_SERIAL.available())
  {
    rxChar = TERM_SERIAL.read();
  }
#ifdef SOCKET_SERVER_PORT
  else if (m_network.IsConnected())
  {
    WiFiClient& socketServerClient = m_network.GetSocketServerClient();
    if (socketServerClient && socketServerClient.connected())
    {
      if (socketServerClient.available()) // Data available?
      {
        rxChar = socketServerClient.read(); // Read data from socket

#ifdef TERMINAL_PASSWORD
        if (!g_bSocketLoggedIn)
        {
          ProcessLogin(socketServerClient, rxChar);
          return; // Never pass (password) input to the command buffer or local echo
        }
#endif
      }
    }
  }
#endif

  // No data?
  if (rxChar == 0x00)
    return;

  if (m_pActiveTermRxData->state == RX_STATE_READY)
    return; // All buffers full

  if (m_pActiveTermRxData->state == RX_STATE_DONE)
  {
    m_pActiveTermRxData->buf_count = 0;
    m_pActiveTermRxData->state = RX_STATE_FILLING;
  }

  if (rxChar == '!') // Repeat the previous command but don't execute it yet
  {
    if (m_pActiveTermRxData->buf_count == 0)
    {
      // Swap pointers
      volatile rx_data_t* p_temp = m_pActiveTermRxData;
      m_pActiveTermRxData = m_pInactiveTermRxData;
      m_pInactiveTermRxData = p_temp;
      m_pActiveTermRxData->state = RX_STATE_FILLING;
      m_pInactiveTermRxData->state = RX_STATE_DONE;

      if (g_bEchoOnOff)
      {
        TERM_SERIAL.print(const_cast<char*>(m_pActiveTermRxData->buf));
      }
    }
  }
  else if (rxChar == CH_DELETE || rxChar == CH_BACKSPACE) // Backspace OR delete (sometimes mixed up by terminal programs)
  {
    if (m_pActiveTermRxData->buf_count != 0)
    {
      if (g_bEchoOnOff)
      {
        // NOTE: Only for serial connection

        // Backspace
        TERM_SERIAL.write(CH_BACKSPACE);
        // Blank character
        TERM_SERIAL.write(' ');
        // And backspace again since the blank jumps forward
        TERM_SERIAL.write(CH_BACKSPACE);
      }

      m_pActiveTermRxData->buf_count--;
    }
  }
  else if (rxChar >= ' ' && rxChar <= '~') // Limit allowed characters
  {
    // Don't overflow + skip leading spaces:
    if (m_pActiveTermRxData->buf_count < (CMD_BUF_SIZE - 1) && !(m_pActiveTermRxData->buf_count == 0 && rxChar == ' '))
    {
      m_pActiveTermRxData->buf[m_pActiveTermRxData->buf_count++] = rxChar;

      if (g_bEchoOnOff && m_pActiveTermRxData->buf[0] != '[' /* Don't display special sequences that start with [ */)
      {
        TERM_SERIAL.write(rxChar);
      }
    }
  }

  if (rxChar == CH_CR || rxChar == CH_LF) // Continue until the command is "entered"
  {
    // Only check non-empty commands
    if (m_pActiveTermRxData->buf_count != 0)
    {
      // Linefeed for local echo
      if (g_bEchoOnOff)
      {
        TERM_SERIAL.println("");
      }

      m_pActiveTermRxData->state = RX_STATE_READY;
      m_pActiveTermRxData->buf[m_pActiveTermRxData->buf_count] = '\0';

      if (m_pInactiveTermRxData->state != RX_STATE_READY)
      {
        // Swap pointers
        volatile rx_data_t* p_temp = m_pActiveTermRxData;
        m_pActiveTermRxData = m_pInactiveTermRxData;
        m_pInactiveTermRxData = p_temp;
        m_pActiveTermRxData->state = RX_STATE_DONE;
      }
    }
  }

  // Handling of special commands
  if (m_pActiveTermRxData->buf_count == 4 && memcmp(const_cast<char*>(m_pActiveTermRxData->buf), "[13~", 4) == 0)  // F3, repeat command without <enter>
  {
    if (m_pInactiveTermRxData->state == RX_STATE_DONE && m_pInactiveTermRxData->buf_count != 0)
    {
      if (g_bEchoOnOff)
      {
        TERM_SERIAL.println(const_cast<char*>(m_pInactiveTermRxData->buf));
      }

      m_pInactiveTermRxData->state = RX_STATE_READY;
    }

    m_pActiveTermRxData->buf_count = 0;
    m_pActiveTermRxData->state = RX_STATE_DONE;
  }
  else if (m_pActiveTermRxData->buf[0] == '[' && m_pActiveTermRxData->buf_count > 4)
  {
    m_pActiveTermRxData->buf_count = 0;
    m_pActiveTermRxData->state = RX_STATE_DONE;
  }
}


char* CTerminal::GetCommand(bool bProgress /* = true */)
{
  if (!IsCommandReady())
    return nullptr;

  // NOTE: Inactive buffer always contains the "next" command to be processed
  char* strCommand = (char*) m_pInactiveTermRxData->buf;

  if (bProgress)
  {
    m_pInactiveTermRxData->state = RX_STATE_DONE;

    if (m_pActiveTermRxData->state == RX_STATE_READY)
    {
      // Swap pointers (so the command in the other buffer gets processed the next round
      volatile rx_data_t* p_temp = m_pActiveTermRxData;
      m_pActiveTermRxData = m_pInactiveTermRxData;
      m_pInactiveTermRxData = p_temp;
    }
  }

  return strCommand;
}