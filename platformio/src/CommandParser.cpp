#include "CommandParser.h"
#include "util.h"
#include "system.h"

#include <string.h>
#include <limits.h>

/*
 * Pack error_code_t into result_code_t (overload)
 * Arguments : errorCode    = Error code (error_code_t)
 *             iArgNum      = Argument number (for get_error_string())
 *             *strArg1     = (Optional) char buffer for argument 1 (for get_error_string())
 *             *strArg2     = (Optional) char buffer for argument 2 (for get_error_string())
 * Returns   : resultCode (result_code_t)
 */
result_code_t pack_result_code(const error_code_t& errorCode, const uint8_t iArgNum /* = 1 */, const char* strArg1 /* = "" */, const char* strArg2 /* = "" */)
{
  result_code_t resultCode;

  resultCode.code = errorCode;
  int32_to_decstr(iArgNum, resultCode.strArgNum, sizeof(resultCode.strArgNum));
  strcpy(resultCode.strArg1, strArg1);
  strcpy(resultCode.strArg2, strArg2);

  return resultCode;
}

result_code_t store_arg1_int32(result_code_t& resultCode, const int32_t iVal32)
{
  int32_to_decstr(iVal32, resultCode.strArg1, MAX_ARG_SIZE);
  return resultCode;
}

result_code_t store_arg2_int32(result_code_t& resultCode, const int32_t iVal32)
{
  int32_to_decstr(iVal32, resultCode.strArg2, MAX_ARG_SIZE);
  return resultCode;
}

result_code_t store_arg1_dbl(result_code_t& resultCode, const double dblArg, const int8_t iDecimals /* = MAX_DBL_STRING_PREC */)
{
  dbl_to_dblstr(dblArg, resultCode.strArg1, iDecimals, MAX_ARG_SIZE);
  return resultCode;
}

result_code_t store_arg2_dbl(result_code_t& resultCode, const double dblArg, const int8_t iDecimals /* = MAX_DBL_STRING_PREC */)
{
  dbl_to_dblstr(dblArg, resultCode.strArg2, iDecimals, MAX_ARG_SIZE);
  return resultCode;
}


/*
 * Check arguments
 * Arguments : *strArgs  = Argument string
 *             arg_count = Amount of arguments required
 * Returns   : resultCode (result_code_t)
 */
result_code_t check_arguments(const char *strArgs, const uint8_t iArgCount, const char* delim /* = " " */)
{
  if (strArgs == NULL || !*strArgs)
  {
    if (iArgCount == 0)
      return pack_result_code(ERR_CODE_OK);
  }

  const auto iCount = strcount(strArgs, delim);
  if (iCount > iArgCount)
    return pack_result_code(ERR_CODE_TOO_MANY_ARGS, iArgCount);

  if (iCount < iArgCount)
    return pack_result_code(ERR_CODE_ARG_MISSING, iCount + 1);

  return pack_result_code(ERR_CODE_OK);
}


result_code_t get_byte_from_hex(const char *str, uint8_t *dataByte, const uint8_t iArgNum)
{
  uint64_t hex;
  if (!hexstr_to_uint64(str, &hex))
    return pack_result_code(ERR_CODE_ARG_FORMAT_HEX, iArgNum);

  *dataByte = (uint8_t) hex;
  return pack_result_code(ERR_CODE_OK);
}


result_code_t get_byte_from_bin(const char *str, uint8_t *dataByte, const uint8_t iArgNum)
{
  uint64_t bin;
  if (!binstr_to_uint64(str, &bin))
    return pack_result_code(ERR_CODE_ARG_FORMAT_BIN, iArgNum);

  *dataByte = (uint8_t) bin;
  return pack_result_code(ERR_CODE_OK);
}


/*
 * From provided string extract binary or hex value
 * Arguments : *str         = String to parse
 *             *buf         = Resulting (byte) buffer
 *             iMaxBitSize  = Maximum amount of bits
 *             iBufSize     = (Maximum) size of *buf
 * Returns   : resultCode (result_code_t)
 */
result_code_t get_bytes_from_bin_or_hex_string(const char *str, uint8_t *buf, const uint8_t iMaxBitSize, const size_t iBufSize, const uint8_t iArgNum)
{
  // Conversion of hex string
  if (strncmp(str, "0x", 2) == 0)
  {
    const char *strArgHex = str + 2;
    const size_t iHexLen = strlen(strArgHex);

    if (iHexLen == 0)
    {
      return pack_result_code(ERR_CODE_ARG_FORMAT_HEX, iArgNum);
    }

    // Divide by 8 /2 + round up
    const uint8_t iMaxHexSize = DIV_CEIL(iMaxBitSize, (8 / 2));
    if (iHexLen > iMaxHexSize) //check size
    {
      result_code_t resultCode = pack_result_code(ERR_CODE_ARG_STR_MAX, iArgNum);
      return store_arg1_int32(resultCode, iMaxHexSize);
    }

    if (!hexstr_to_bytes(strArgHex, buf, iBufSize))
      return pack_result_code(ERR_CODE_ARG_FORMAT_HEX, iArgNum);

    // Check binary size
    if (!check_bit_size(buf, iBufSize, iMaxBitSize))
    {
      result_code_t resultCode = pack_result_code(ERR_CODE_ARG_BIN_MAX, iArgNum);
      return store_arg1_int32(resultCode, iMaxBitSize);
    }
  }
  else if (strncmp(str, "0b", 2) == 0)   // conversion of bin string
  {
    const char *strArgBin = str + 2;
    const size_t iBinLen = strlen(strArgBin);

    if (iBinLen == 0)
    {
      return pack_result_code(ERR_CODE_ARG_FORMAT_BIN, iArgNum);
    }

    if (!binstr_to_bytes(strArgBin, buf, iBufSize))
      return pack_result_code(ERR_CODE_ARG_FORMAT_BIN, iArgNum);

    // Check binary size
    if (iBinLen > iMaxBitSize)
    {
      result_code_t resultCode = pack_result_code(ERR_CODE_ARG_BIN_MAX, iArgNum);
      return store_arg1_int32(resultCode, iMaxBitSize);
    }
  }
  else
  {
    return pack_result_code(ERR_CODE_ARG_VAL, iArgNum);
  }

  return pack_result_code(ERR_CODE_OK);
}


/*
 * From provided string extract boolean value (other "on"/"off" or "1"/"0")
 * Arguments : *str      = String to parse
 *             *bValue   = Resulting boolean value
 *             iArgNum   = Argument number (used for pack_result_code_arg())
 * Returns   : resultCode (result_code_t)
 */
result_code_t get_bool_from_string(const char *str, bool *bValue, const uint8_t iArgNum)
{
  if (STRIEQUALS(str, "on") || STRIEQUALS(str, "1"))
    *bValue = true;
  else if (STRIEQUALS(str, "off") || STRIEQUALS(str, "0"))
    *bValue = false;
  else
    return pack_result_code(ERR_CODE_ARG_VAL, iArgNum);

  return pack_result_code(ERR_CODE_OK);
}


/*
 * From provided string extract 32 bit signed integer value
 * Arguments : *str      = String to parse
 *             *iValue   = Resulting 32 bit signed integer value
 *             strMinVal = Minimal required integer value
 *             strMaxVal = Maximum required integer value
 *             iArgNum   = Argument number (used for pack_result_code_arg())
 * Returns   : resultCode (result_code_t)
 */
result_code_t get_int32_from_string(const char *str, int32_t *iValue, const int32_t iMinval, const int32_t iMaxVal, const uint8_t iArgNum)
{
  if (!decstr_to_int32(str, iValue))
    return pack_result_code(ERR_CODE_ARG_VAL, iArgNum);

  if (*iValue < iMinval)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_VAL_MIN, iArgNum);
    return store_arg1_int32(resultCode, iMinval);
  }

  if (*iValue > iMaxVal)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_VAL_MAX, iArgNum);
    return store_arg1_int32(resultCode, iMaxVal);
  }

  return pack_result_code(ERR_CODE_OK);
}


/*
 * From provided string extract 32 bit unsigned integer value
 * Arguments : *str      = String to parse
 *             *iValue   = Resulting 32 bit signed integer value
 *             strMinVal = Minimal required integer value
 *             strMaxVal = Maximum required integer value
 *             iArgNum   = Argument number (used for pack_result_code_arg())
 * Returns   : resultCode (result_code_t)
 */
result_code_t get_uint32_from_string(const char *str, uint32_t *iValue, const uint32_t iMinVal, const uint32_t iMaxVal, const uint8_t iArgNum)
{
  if (!decstr_to_uint32(str, iValue))
    return pack_result_code(ERR_CODE_ARG_VAL, iArgNum);

  if (*iValue < iMinVal)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_VAL_MIN, iArgNum);
    return store_arg1_int32(resultCode, iMinVal);
  }

  if (*iValue > iMaxVal)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_VAL_MAX, iArgNum);
    return store_arg1_int32(resultCode, iMaxVal);
  }

  return pack_result_code(ERR_CODE_OK);
}


/*
 * From provided string extract double (floating point) value
 * Arguments : *str      = String to parse
 *             *fValue   = Resulting double value
 *             strMinVal = Minimal required double value
 *             strMaxVal = Maximum required double value
 *             iArgNum   = Argument number (used for pack_result_code_arg())
 *             iDecimals = Number of decimals for error messages (optional)
 * Returns   : resultCode (result_code_t)
 */
result_code_t get_double_from_string(const char *str, double *dblValue, const double dblMinVal, const double dblMaxVal, const uint8_t iArgNum, const int8_t iDecimals /* = -1 */)
{
  if (!dblstr_to_double(str, dblValue))
    return pack_result_code(ERR_CODE_ARG_VAL, iArgNum);

  if (*dblValue < dblMinVal)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_VAL_MIN, iArgNum);
    if (iDecimals >= 0)
      return store_arg1_dbl(resultCode, dblMinVal, iDecimals);
    else
      return store_arg1_dbl(resultCode, dblMinVal);
  }

  if (*dblValue > dblMaxVal)
  {
    result_code_t resultCode = pack_result_code(ERR_CODE_ARG_VAL_MAX, iArgNum);
    if (iDecimals >= 0)
      return store_arg1_dbl(resultCode, dblMaxVal, iDecimals);
    else
      return store_arg1_dbl(resultCode, dblMaxVal);
  }

  return pack_result_code(ERR_CODE_OK);
}


/*
 * From provided string extract IPv4 address
 * Arguments : *str      = String to parse
 *             *ipv4     = 4 byte IPv4 address
 *             iArgNum   = Argument number (used for pack_result_code_arg())
 * Returns   : resultCode (result_code_t)
 */
result_code_t get_ipv4_from_string(const char* str, uint8_t* ipv4, const uint8_t iArgNum)
{
  for (uint8_t pos = 0, pos_last = 0, count = 0; pos <= strlen(str); pos++)
  {
    if (str[pos] == '.' || str[pos] == '\0')
    {
      if (pos == 0 || (pos == pos_last) || (pos - pos_last > 3) || count >= 4)
        return pack_result_code(ERR_CODE_INVALID_IPV4, iArgNum);

      char strNum[4];
      strncpy0(strNum, str + pos_last, pos - pos_last);

      int32_t iNum;
      if (!decstr_to_int32(strNum, &iNum))
        return pack_result_code(ERR_CODE_INVALID_IPV4, iArgNum);

      if (iNum < 0 || iNum > 255)
        return pack_result_code(ERR_CODE_INVALID_IPV4, iArgNum);

      ipv4[count++] = (uint8_t) iNum;

      pos_last = pos + 1;
    }
  }

  return pack_result_code(ERR_CODE_OK);
}


/*
 * For provided result_code_t argument get error string
 * Arguments : resultCode = Result code (result_code_t)
 *             *strResult = String buffer to hold the error string (caller needs to allocate string!)
 *             bAppend    = True means append to existing string in *strResult
 * Returns   : void
 */
void get_error_string(const result_code_t& resultCode, char *strResult, const bool bAppend /* = false */)
{
  if (!bAppend)
    strResult[0] = '\0'; // Start with empty string

  // Silent OK:
  if (resultCode.code == ERR_CODE_OK_NULL)
    return;

  if (resultCode.code == ERR_CODE_OK)
  {
    STRCAT_PSTR(strResult, "OK\r\n");
    return;
  }

  if (resultCode.code  == ERR_CODE_OK_AFTER_RESTART)
  {
    STRCAT_PSTR(strResult, "OK (Will take effect after restart)\r\n");
    return;
  }

  // The rest of the codes are errors:
  STRCAT_PSTR(strResult, "ERROR: "); // Prefix

  // Process value
  switch(resultCode.code)
  {
    case ERR_CODE_UNKNOWN:
      STRCAT_PSTR(strResult, "Unknown error");
      break;

    case ERR_CODE_CMD_UNKNOWN:
      STRCAT_PSTR(strResult, "Unknown command");
      break;

    case ERR_CODE_CMD_INVALID:
      STRCAT_PSTR(strResult, "Invalid command");
      break;

    case ERR_CODE_SUBCMD_MISSING:
      STRCAT_PSTR(strResult, "Missing sub command");
      break;

    case ERR_CODE_SUBCMD_UNKNOWN:
      STRCAT_PSTR(strResult, "Unknown sub command");
      break;

    case ERR_CODE_CMD_NO_IMPLEMENT:
      STRCAT_PSTR(strResult, "Command not implemented");
      break;

    case ERR_CODE_CMD_SYNTAX:
      STRCAT_PSTR(strResult, "Command syntax error");
      break;

    case ERR_CODE_TOO_MANY_ARGS:
      strcat(strResult, resultCode.strArgNum);
      STRCAT_PSTR(strResult, " arguments required");
      break;

    case ERR_CODE_ARG_MISSING:
      STRCAT_PSTR(strResult, "Argument ");
      strcat(strResult, resultCode.strArgNum);
      STRCAT_PSTR(strResult, " missing");
      break;

    case ERR_CODE_ARG_VAL:
      STRCAT_PSTR(strResult, "Argument ");
      strcat(strResult, resultCode.strArgNum);
      STRCAT_PSTR(strResult, " invalid");
      break;

    case ERR_CODE_ARG_FORMAT_MIN_NUMBER:
    case ERR_CODE_ARG_FORMAT_MAX_NUMBER:
      STRCAT_PSTR(strResult, "Numeric part of ");
      // fall through
    case ERR_CODE_ARG_VAL_MIN:
      // fall through
    case ERR_CODE_ARG_VAL_MAX:
      STRCAT_PSTR(strResult, "argument ");
      strcat(strResult, resultCode.strArgNum);

      if (resultCode.code == ERR_CODE_ARG_VAL_MIN || resultCode.code == ERR_CODE_ARG_FORMAT_MIN_NUMBER)
        STRCAT_PSTR(strResult, " min. value (");
      else
        STRCAT_PSTR(strResult, " max. value (");

      strcat(strResult, resultCode.strArg1);
      STRCAT_PSTR(strResult, ") exceeded");
      break;

    case ERR_CODE_ARG_FORMAT:
      STRCAT_PSTR(strResult, "Argument ");
      strcat(strResult, resultCode.strArgNum);
      STRCAT_PSTR(strResult, " should consist of letter(s) followed by number(s)");
      break;

    case ERR_CODE_INVALID_IPV4:
      STRCAT_PSTR(strResult, "Invalid IPv4 address");
      break;

    case ERR_CODE_INVALID_PORT:
      STRCAT_PSTR(strResult, "Invalid port number");
    break;

    case ERR_CODE_ARG_STR_MAX:
      STRCAT_PSTR(strResult, "Maximum of ");
      strcat(strResult, resultCode.strArg1);
      STRCAT_PSTR(strResult, " characters exceeded in argument ");
      strcat(strResult, resultCode.strArgNum);
      break;

    case ERR_CODE_ARG_BIN_MAX:
      STRCAT_PSTR(strResult, "Maximum of ");
      strcat(strResult, resultCode.strArg1);
      STRCAT_PSTR(strResult, " binary bits exceeded in argument ");
      strcat(strResult, resultCode.strArgNum);
      break;

    case ERR_CODE_ARG_FORMAT_HEX:
      STRCAT_PSTR(strResult, "Hex format error in argument ");
      strcat(strResult, resultCode.strArgNum);
      break;

    case ERR_CODE_ARG_FORMAT_BIN:
      STRCAT_PSTR(strResult, "Binary format error in argument ");
      strcat(strResult, resultCode.strArgNum);
      break;

    case ERR_CODE_BUSY:
      STRCAT_PSTR(strResult, "Device busy");
      break;

    default:
      char strTemp[4];
      strcat(strResult, int32_to_decstr(resultCode.code, strTemp, sizeof(strTemp)));
      strcat(strResult, " ");
      strcat(strResult, resultCode.strArgNum);
      strcat(strResult, " ");
      strcat(strResult, resultCode.strArg1);
      strcat(strResult, " ");
      strcat(strResult, resultCode.strArg2);
      break;
  }

  strcat(strResult, "\r\n");
}


result_code_t parse_decimal_arg(const char* strArgs, const int32_t iMinIntSize, const int32_t iMaxIntSize, int32_t* iVal32)
{
  result_code_t resultCode = check_arguments(strArgs, ARG_INT32_NUM1);
  if (resultCode.code != ERR_CODE_OK)
    return resultCode;

  return get_int32_from_string(strArgs, iVal32, iMinIntSize, iMaxIntSize, ARG_INT32_NUM1);
}


result_code_t parse_hex_or_bin_arg(const char* strArgs, const uint8_t iMaxBitSize, uint8_t* pBuf, size_t iBufSize)
{
  result_code_t resultCode = check_arguments(strArgs, ARG_INT32_NUM1);
  if (resultCode.code != ERR_CODE_OK)
    return resultCode;

  return get_bytes_from_bin_or_hex_string(strArgs, pBuf, iMaxBitSize, iBufSize, ARG_INT32_NUM1);
}


result_code_t parse_ipv4_arg(const char* strArgs, uint8_t* pIPAddress)
{
  result_code_t resultCode = check_arguments(strArgs, ARG_INT32_NUM1);
  if (resultCode.code != ERR_CODE_OK)
    return resultCode;

  return get_ipv4_from_string(strArgs, pIPAddress, ARG_INT32_NUM1);
}
