#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "CommandParser.h"

#include <stdlib.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

// Maximum byte size for arguments stored in m_bufArg1
#define ARG_BUF_BYTE_SIZE 6

class CCommandHandler
{
  public:
    CCommandHandler() {};   // Empty ctor
    ~CCommandHandler() {};  // Empty dtor

    bool GetLocalEchoEnabled(void) const { return m_bLocalEcho; };

    static result_code_t FormatIP(uint8_t *data, char *strResult);

  protected:
    result_code_t CmdEchoOnOff(const char *strArgs);
    result_code_t CmdReboot(const char *strArgs);

  private:
    bool m_bLocalEcho = true;
};

#endif // COMMAND_HANDLER_H
