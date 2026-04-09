#include <Arduino.h>
#include <limits.h>
#include "util.h"

bool BytesToFloat(const byte* bytes, const unsigned int len, float& fResult)
{
  char strBytes[8];

  if (len >= sizeof(strBytes))
    return false;

  memcpy(strBytes, bytes, len);
  strBytes[len] = '\0';

  char *end;
  fResult = strtof(strBytes, &end);

  // Make sure it's not zero-length and it's a proper number (not suffixed with anything):
  if (*end || end == strBytes)
    return false;

  // Check range
  return (!(fResult == LONG_MIN || fResult == LONG_MAX));
}


bool BytesToInt32(const byte* bytes, const unsigned int len, int32_t& iResult)
{
  char strBytes[12];

  if (len >= sizeof(strBytes))
    return false;

  memcpy(strBytes, bytes, len);
  strBytes[len] = '\0';

  char *end;
  int32_t iResult32 = strtol(strBytes, &end, 0);
  iResult = iResult32;

  // Make sure it's not zero-length and it's a proper number (not suffixed with anything):
  if (*end || end == strBytes)
    return false;

  // Check range
  return (!(iResult32 == LONG_MIN || iResult32 == LONG_MAX));
}
