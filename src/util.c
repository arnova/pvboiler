/*
  C Library to implement various handy functions

  Written by       : Arno van Amersfoort
  Language         : C99
  Target compiler  : (Generic)
  Dependencies     : (none)
  Initial date     : August 27, 2019
  Last modified    : July 13, 2025
*/

#include "util.h"

#include <string.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>

/*
 * Simple x to power y function for integers. Does NOT do overflow checking!
 */
uint32_t x_to_y(const uint32_t iX, const uint32_t iY)
{
  uint32_t iResult = 1;

  for (uint32_t it = 0; it < iY; it++)
  {
    iResult *= iX;
  }

  return iResult;
}


/* check_bit_size - Check the highest bit set in *buf and compare against iMaxBitSize
   Arguments : *buf         = Byte buffer to check
               iBufSize     = Size of *buf
               iMaxbitSize  = Maximum bit size to check against
   Returns   : True if the highest bit set in *buf is equal or lower than iMaxBitSize, else false
 */
bool check_bit_size(const uint8_t *buf, const size_t iBufSize, const uint8_t iMaxBitSize)
{
  if (iMaxBitSize == 0)
    return false;

  const uint8_t iBitByteSize = DIV_CEIL(iMaxBitSize, 8);

  // Check the highest non-zero byte
  for (size_t iByte = iBufSize; iByte > iBitByteSize; iByte--)
  {
    if (buf[iByte - 1] != 0)
    {
      return false; // No fit
    }
  }

  // For bit sizes not aligned to bytes, check individual bits of the MSbyte
  if (iMaxBitSize % 8 != 0)
  {
    // Check the highest bit set
    for (uint8_t iBit = 8; iBit > 0; iBit--)
    {
      if (buf[iBitByteSize - 1] & (1 << (iBit - 1)))
      {
        return (iBit <= (iMaxBitSize % 8));
      }
    }
  }

  return true; // Fits}
}


/* strsplit - Split string in *str1 using delim as delimiter
   Arguments : *str    = Buffer array holding string (will be modified!)
               *delims = Delimiter character(s)
   Returns   : Character pointer to the location of the string after the delimiter
   Notes     : str will be terminated at the position where one of delims was found
 */
char *strsplit(char *str, const char *delims)
{
  char *find = strpbrk(str, delims);

  if (find != NULL)
  {
    *find = '\0'; // Terminate "first" string
    find++;       // Set pointer to beginning of "second" string
  }

  return find; // May return NULL (when no delims found)!
}


int strcount(const char *str, const char *delims)
{
  if (str == NULL || !*str)
    return 0;

  int count = 1; // There's at least one string
  bool last = false;
  for (unsigned int it = 0; it < strlen(str); it++)
  {
    if (strchr(delims, str[it]))
    {
      if (!last) // Don't count null strings with multiple delims
      {
        count++;
        last = true;
      }
    }
    else
    {
      last = false;
    }
  }

  return count;
}


/* strtrimleft  - Trim leading chars provided in chrstr
   Arguments : *str    = Buffer array holding string (will be modified!)
               *chrstr = Remove these character(s)
   Returns   : New pointer to modified string (maybe same as *str)
   Notes     : str will be modified
 */
char *strtrimleft(char *str, const char *chrstr)
{
  if (strlen(str) == 0)
    return str;

  // Get pointer to start of string
  char *startptr = str;

  // Look for leading characters
  while (*startptr && strchr(chrstr, *startptr))
    startptr++;

  // Move string
  strcpy(str, startptr);

  return str;
}


/* strtrimright - Trim trailing chars provided in chrstr
   Arguments : *str    = Buffer array holding string (will be modified!)
               *chrstr = Remove these character(s)
   Returns   : New pointer to modified string (maybe same as *str)
   Notes     : str will be modified
 */
char *strtrimright(char *str, const char *chrstr)
{
  if (strlen(str) == 0)
    return str;

  // Get pointer to end of string
  char *endptr = strchr(str, '\0') - 1;

  // Remove trailing characters
  while (endptr >= str && strchr(chrstr, *endptr))
  {
    *endptr = '\0';
    endptr--;
  }

  return str;
}


/* strtrim  - Trim leading/trailing chars provided in chrstr
   Arguments : *str    = Buffer array holding string (will be modified!)
               *chrstr = Remove these character(s)
   Returns   : New pointer to modified string (maybe same as *str)
   Notes     : str will be modified
 */
char *strtrim(char *str, const char *chrstr)
{
  return strtrimright(strtrimleft(str, chrstr), chrstr);
}


int strfind(const char *str, const char *substr)
{
  char *p = strstr(str, substr);

  if (p == NULL)
    return -1;

  return str - p; // Return offset from input string at 0
}


/* Find substring in specified string at specified start in string */
int strncmpat(const char *str, const uint8_t start, const char *substr)
{
  if (start >= strlen(str))
    return -1;

  return strncmp(str + start, substr, strlen(substr));
}


/*
   Function copy string (source) to string (dest) limited by size (n) (left-to-right)
   Parameters : target (dest), source (source)
   Return     : pointer to (dest)
*/
char *strncpy0(char *dest, const char *source, const uint8_t n)
{
  strncpy(dest, source, n);
  if (strlen(source) >= n)
    dest[n] = '\0';
  
  return dest;
}


/*
 * Convert int32_t to a decimal string
 * Arguments : iNum     = int32_t value to convert
 *             *strDec  = Buffer array for resulting decimal string (must be able to hold iMaxLen bytes + 1!)
 *             iBufSize = Size of *strDec
 * Returns   : *strDec
 */
char *int32_to_decstr(int32_t iNum, char *strDec, const size_t iBufSize)
{
  // Obviously this should never happen:
  if (iBufSize < 2)
  {
    strDec = NULL;
    return "";
  }

  size_t iPos = 0;

  // Special case for 0
  if (iNum == 0)
  {
    strDec[iPos++] = '0';
  }
  else
  {
    // Check sign
    if (iNum < 0)
    {
      strDec[iPos++] = '-';
      iNum = -(iNum); // Get rid of - sign (absolute value)
    }

    bool bShow = false;
    for (int32_t iDec = 1000000000 /* Biggest decimal that fits in an int32_t */; (iDec != 0 && iPos < (iBufSize - 1)); iDec /= 10)
    {
      if (iDec <= iNum)
        bShow = true;

      if (bShow)
      {
        uint8_t decVal = (iNum / iDec);
        strDec[iPos++] = ('0' + decVal); // Convert into character
        iNum -= (decVal * iDec);
      }
    }
  }

  strDec[iPos] = '\0'; // End of string

  return strDec;
}


/*
 * Convert uint32_t to a decimal string
 * Arguments : iNum    = uint32_t value to convert
 *             *strDec  = Buffer array for resulting decimal string (must be able to hold iMaxLen bytes + 1!)
 *             iBufSize = Size of *strDec
 * Returns   : *strDec
 */
char *uint32_to_decstr(uint32_t iNum, char *strDec, const size_t iBufSize)
{
  // Obviously this should never happen:
  if (iBufSize < 2)
  {
    strDec = NULL;
    return "";
  }

  size_t iPos = 0;

  // Special case for 0
  if (iNum == 0)
  {
    strDec[iPos++] = '0';
  }
  else
  {
    bool bShow = false;
    for (uint32_t iDec = 1000000000 /* Biggest decimal that fits in an uint32_t */; (iDec != 0 && iPos < (iBufSize - 1)); iDec /= 10)
    {
      if (iDec <= iNum)
        bShow = true;

      if (bShow)
      {
        uint8_t decVal = (iNum / iDec);
        strDec[iPos++] = ('0' + decVal); // Convert into character
        iNum -= (decVal * iDec);
      }
    }
  }

  strDec[iPos] = '\0'; // End of string

  return strDec;
}


/*
 * Convert int64_t to a decimal string
 * Arguments : iNum     = int64_t value to convert
 *             *strDec  = Buffer array for resulting decimal string (must be able to hold iMaxLen bytes + 1!)
 *             iBufSize = Size of *strDec
 * Returns   : *strDec
 */
char *int64_to_decstr(int64_t iNum, char *strDec, const size_t iBufSize)
{
  // Obviously this should never happen:
  if (iBufSize < 2)
  {
    strDec = NULL;
    return "";
  }

  size_t iPos = 0;

  // Special case for 0
  if (iNum == 0)
  {
    strDec[iPos++] = '0';
  }
  else
  {
    // Check sign
    if (iNum < 0)
    {
      strDec[iPos++] = '-';
      iNum = -(iNum); // Get rid of - sign (absolute value)
    }

    bool bShow = false;
    for (int64_t iDec = 1e18 /* Biggest decimal that fits in an int32_t */; (iDec != 0 && iPos < (iBufSize - 1)); iDec /= 10)
    {
      if (iDec <= iNum)
        bShow = true;

      if (bShow)
      {
        uint8_t decVal = (iNum / iDec);
        strDec[iPos++] = ('0' + decVal); // Convert into character
        iNum -= (decVal * iDec);
      }
    }
  }

  strDec[iPos] = '\0'; // End of string

  return strDec;
}


/*
 * Convert uint64 to a decimal string
 * Arguments : iNum    = uint64 value to convert
 *             *strDec  = Buffer array for resulting decimal string (must be able to hold iMaxLen bytes + 1!)
 *             iBufSize = Size of *strDec
 * Returns   : *strDec
 */
char *uint64_to_decstr(uint64_t iNum, char *strDec, const size_t iBufSize)
{
  // Obviously this should never happen:
  if (iBufSize < 2)
  {
    strDec = NULL;
    return "";
  }

  size_t iPos = 0;

  // Special case for 0
  if (iNum == 0)
  {
    strDec[iPos++] = '0';
  }
  else
  {
    bool bShow = false;
    for (uint32_t iDec = 1000000000 /* Biggest decimal that fits in an uint32_t */; (iDec != 0 && iPos < (iBufSize - 1)); iDec /= 10)
    {
      if (iDec <= iNum)
        bShow = true;

      if (bShow)
      {
        uint8_t decVal = (iNum / iDec);
        strDec[iPos++] = ('0' + decVal); // Convert into character
        iNum -= (decVal * iDec);
      }
    }
  }

  strDec[iPos] = '\0'; // End of string

  return strDec;
}


/*
 * Convert double to a double string
 * Arguments : dblNum     = Double value to convert
 *             *strDbl    = Buffer array for resulting double string
 *             iDecimals  = Number of decimals precision to use
 *             iBufSize   = Allowable size of *strDbl (sizeof)
 * Returns   : *strDbl
 */
char *dbl_to_dblstr(const double dblNum, char *strDbl, const int8_t iDecimals, const size_t iBufSize)
{
  // NOTE: substract one extra to prevent buffer overflows due to rounding errors
  int16_t iSizeLeft = iBufSize - iDecimals - 1;

  // Take into account - sign
  if (dblNum < 0)
    iSizeLeft--;

  // Take into account dot
  if (iDecimals != 0)
    iSizeLeft--;

  // Protect against buffer overflows
  if (iSizeLeft < 0)
  {
    strcpy(strDbl, "");
    return strDbl;
  }

  // Make sure dblNum is not too big for strDbl
  const double dblMax = pow(10, iSizeLeft);
  if (dblNum >= dblMax || dblNum <= -dblMax)
    strcpy(strDbl, "?");
  else
  {
    //dtostrf(dblNum, 0, iDecimals, strDbl); // NOTE: This adds significant code size, so use snprintf instead!
    snprintf(strDbl, iBufSize, "%f", dblNum);

    // Need to handle iDecimals:
    for (int8_t it = strlen(strDbl) - 1; it > iDecimals + 1; it--)
    {
      if (strDbl[it - iDecimals - 1] == '.')
      {
        strDbl[it] = '\0'; // Truncate number of decimals
        break;
      }
    }
  }

  return strDbl;
}


/*
 * Convert unsigned integer to binary string, prefixed with 0's to fill up till iLen
 * Arguments : iNum    = Unsigned int64 value to convert
 *             *strBin = Buffer array for the resulting string (must be able to hold iLen + 1 bytes!)
 *             iLen    = Number of binary-digits strBin should have
 * Returns   : *strBin
 */
char *uint64_to_binstr(uint64_t iNum, char *strBin, const uint8_t iLen)
{
  // Obviously this should never happen:
  if (iLen < 2)
  {
    strBin = NULL;
    return "";
  }

  for (int8_t iPos = iLen - 1; iPos >= 0; iPos--)
  {
    // Test bit and select character
    strBin[iPos] = (iNum & 1) ? '1' : '0';

    iNum >>= 1; // (Shift to) next
  }
  strBin[iLen] = '\0'; // End of string

  return strBin;
}


/*
 * Convert unsigned integer to hexadecimal string, prefixed with 0's to fill up till iLen
 * Arguments : iNum    = Unsigned int64 value to convert
 *             *strHex = Buffer array for the resulting string (must be able to hold iLen + 1 bytes!)
 *             iLen    = Number of hex-digits strHex should have
 * Returns   : *strHex
 */
char *uint64_to_hexstr(uint64_t iNum, char *strHex, const uint8_t iLen)
{
  // Obviously this should never happen:
  if (iLen < 2)
  {
    strHex = NULL;
    return "";
  }

  for (int8_t iPos = iLen - 1; iPos >= 0; iPos--)
  {
    // Check four LSB bits
    char hex = iNum & 15;

    if (hex < 10)
      strHex[iPos] = '0' + hex;
    else
      strHex[iPos] = 'A' - 10 + hex;

    iNum >>= 4; // (Shift to) next hex digit
  }

  strHex[iLen] = '\0'; // End of string

  return strHex;
}


char *bytes_to_hexstr(const uint8_t *byteArray, char *strHex, const uint8_t iLen)
{
  uint8_t charCount = 0;

  for (uint8_t i = 0; i < iLen; i++)
  {
    for (uint8_t j = 0; j < 2 ; j++)
    {
      const uint8_t nibble = (byteArray[i] >> (4*(1 - j))) & 0x0F;
      if (/*nibble >= 0 && */ nibble <= 9)
      {
        strHex[charCount++] = '0' + nibble;
      }
      else
      {
        strHex[charCount++] = 'A' + (nibble - 10);
      }
    }
  }

  strHex[charCount] = '\0'; // End of string

  return strHex;
}


bool dblstr_to_double(const char *str, double *pValue)
{
  char *end;
  *pValue = strtod(str, &end);

  // Make sure it's not zero-length and it's a proper number (not suffixed with anything): 
  if (*end || end == str)
    return false;

  // Check value
  return (!isnan(*pValue));
}


bool decstr_to_int32(const char *str, int32_t *pValue)
{
  char *end;
  *pValue = strtol(str, &end, 0);

  // Make sure it's not zero-length and it's a proper number (not suffixed with anything):
  if (*end || end == str)
    return false;

  // Check range
  return (!(*pValue == LONG_MIN || *pValue == LONG_MAX));
}


bool decstr_to_uint32(const char *str, uint32_t *pValue)
{
  char *end;
  *pValue = strtoul(str, &end, 0);

  // Make sure it's not zero-length and it's a proper number (not suffixed with anything):
  if (*end || end == str)
    return false;

  // Check range
  return (!(*pValue == ULONG_MAX));
}


bool binstr_to_uint8(const char *strBin, uint8_t *pValue)
{
  *pValue = 0;

  const size_t iBinSize = strlen(strBin);

  if (iBinSize > 8)
    return false;

  uint8_t binVal = 1;
  for (int8_t pos = iBinSize - 1; pos >= 0; pos--)
  {
    if (strBin[pos] == '1')
      *pValue |= binVal;
    else if (strBin[pos] != '0')
      return false; // Bad syntax

    binVal <<= 1; // Next bit
  }

  return true;
}


bool binstr_to_uint64(const char *strBin, uint64_t *pValue)
{
  *pValue = 0;

  if (strlen(strBin) > 64)
    return false;

  uint64_t binVal = 1;
  for (int8_t pos = strlen(strBin) - 1; pos >= 0; pos--)
  {
    if (strBin[pos] == '1')
      *pValue |= binVal;
    else if (strBin[pos] != '0')
      return false; // Bad syntax

    binVal <<= 1; // Next bit
  }

  return true;
}


bool binstr_to_bytes(const char *strBin, uint8_t *pBuf, const size_t iBufSize)
{
  char strBinByte[9];
  strBinByte[8] = 0; // Make sure end of string is in place
  size_t iBufCount = 0;

  // Parse binary from right to left
  int8_t iPos = strlen(strBin);
  while (iPos > 0 && iBufCount < iBufSize)
  {
    // Special handling for the last byte because of possibly non 8-char sizes bin strings
    if (iPos - 8 < 0)
    {
      const size_t iLen = iPos % 8;
      memset(strBinByte, '0', 8 - iLen);
      memcpy(strBinByte + 8 - iLen, strBin, iLen);

      iPos = 0; // We're done
    }
    else
    {
      iPos -= 8;
      memcpy(strBinByte, strBin + iPos, 8);
    }

    uint8_t iBin8;
    if (!binstr_to_uint8(strBinByte, &iBin8))
      return false;
    
    pBuf[iBufCount++] = iBin8;
  }

  // Zero pad buffer
  for (size_t i = iBufCount; i < iBufSize; i++)
  {
    pBuf[i] = 0x00;
  }

  return true;
}


bool hexstr_to_uint8(const char *strHex, uint8_t *pValue)
{
  *pValue = 0;

  const size_t iHexLen = strlen(strHex);

  if (iHexLen > 2)
    return false;

  uint8_t hexVal = 1;
  for (int8_t pos = iHexLen - 1; pos >= 0; pos--)
  {
    if (strHex[pos] >= '0' && strHex[pos] <= '9')
    {
      *pValue += (hexVal * (strHex[pos] - '0'));
    }
    else if (strHex[pos] >= 'A' && strHex[pos] <= 'F')
    {
      *pValue += (hexVal * (strHex[pos] - 'A' + 10));
    }
    else if (strHex[pos] >= 'a' && strHex[pos] <= 'f')
    {
      *pValue += (hexVal * (strHex[pos] - 'a' + 10));
    }
    else
    {
      return false; // Bad syntax
    }
    hexVal <<= 4; // Next digit
  }
  return true;
}


bool hexstr_to_uint64(const char *strHex, uint64_t *pValue)
{
  *pValue = 0;

  if (strlen(strHex) > 16)
    return false;

  uint64_t hexVal = 1;
  for (int8_t pos = strlen(strHex) - 1; pos >= 0; pos--)
  {
    if (strHex[pos] >= '0' && strHex[pos] <= '9')
    {
      *pValue += (hexVal * (strHex[pos] - '0'));
    }
    else if (strHex[pos] >= 'A' && strHex[pos] <= 'F')
    {
      *pValue += (hexVal * (strHex[pos] - 'A' + 10));
    }
    else if (strHex[pos] >= 'a' && strHex[pos] <= 'f')
    {
      *pValue += (hexVal * (strHex[pos] - 'a' + 10));
    }
    else
    {
      return false; // Bad syntax
    }
    hexVal <<= 4; // Next digit
  }
  return true;
}


bool hexstr_to_bytes(const char *strHex, uint8_t *pBuf, const size_t iBufSize)
{
  char strHexByte[3];
  strHexByte[2] = 0; // Make sure end of string is in place
  size_t iBufCount = 0;

  // Parse binary from right to left
  int8_t iPos = strlen(strHex);
  while (iPos > 0 && iBufCount < iBufSize)
  {
    // Special handling for the last byte because of possibly non 2-char sizes hex strings
    if (iPos - 2 < 0)
    {
      strHexByte[0] = '0';
      strHexByte[1] = strHex[0];

      iPos = 0; // We're done
    }
    else
    {
      iPos -= 2;
      memcpy(strHexByte, strHex + iPos, 2);
    }

    uint8_t iHex8;
    if (!hexstr_to_uint8(strHexByte, &iHex8))
      return false;
    
    pBuf[iBufCount++] = iHex8;
  }

  // Zero pad buffer
  for (size_t i = iBufCount; i < iBufSize; i++)
  {
    pBuf[i] = 0x00;
  }

  return true;
}


// Helper function to format IP Address
char* bytes_to_ipstr(const uint8_t *bufIP, char* strIP)
{
  strIP[0] = '\0';
  for (uint8_t i = 0; i < 4; i++)
  {
    char strDec[4];
    strcat(strIP, uint32_to_decstr(bufIP[i], strDec, sizeof(strDec)));
    if (i < 3)
      strcat(strIP, ".");
  }

  return strIP;
}


// Helper function to format IP Address
char* bytes_to_macstr(const uint8_t *bufMAC, char* strMAC)
{
  strMAC[0] = '\0';
  for (uint8_t i = 0; i < 6; i++)
  {
    char strDec[3];
    strcat(strMAC, bytes_to_hexstr(bufMAC + i, strDec, 1));

    // Pad zeros
    if (strlen(strDec) == 1)
      strcat(strMAC, "0");

    if (i < 5)
      strcat(strMAC, ":");
  }

  return strMAC;
}


bool bytes_to_int32(const uint8_t* buf, const uint8_t iBufSize, int32_t* pValue)
{
  char strBytes[12];

  if (iBufSize >= sizeof(strBytes))
    return false;

  memcpy(strBytes, buf, iBufSize);
  strBytes[iBufSize] = '\0';

  char *end;
  int32_t iResult32 = strtol(strBytes, &end, 0);
  *pValue = iResult32;

  // Make sure it's not zero-length and it's a proper number (not suffixed with anything):
  if (*end || end == strBytes)
    return false;

  // Check range
  return (!(iResult32 == LONG_MIN || iResult32 == LONG_MAX));
}