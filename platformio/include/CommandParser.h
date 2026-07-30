#pragma once
#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// Maximum actual string size for converted doubles (excluding \0 at end of string)
// This is able to hold something like -1000000.0000, which requires 13 bytes (including sign & dot, excluding \0 at end of string)
#define MAX_DBL_STRING_SIZE 13

// Maximum number of decimals of a double for string conversion
#define MAX_DBL_STRING_PREC  5

// Error (return) codes
enum error_code_e
{
  ERR_CODE_OK = 0,
  ERR_CODE_OK_NULL,
  ERR_CODE_OK_AFTER_REBOOT,
  ERR_CODE_UNKNOWN,
  ERR_CODE_CMD_UNKNOWN,
  ERR_CODE_CMD_INVALID,
  ERR_CODE_SUBCMD_UNKNOWN,
  ERR_CODE_SUBCMD_MISSING,
  ERR_CODE_CMD_NO_IMPLEMENT,
  ERR_CODE_CMD_SYNTAX,
  ERR_CODE_TOO_MANY_ARGS,
  ERR_CODE_ARG_MISSING,
  ERR_CODE_ARG_VAL,
  ERR_CODE_ARG_VAL_MIN,
  ERR_CODE_ARG_VAL_MAX,
  ERR_CODE_ARG_FORMAT_MIN_NUMBER,
  ERR_CODE_ARG_FORMAT_MAX_NUMBER,
  ERR_CODE_ARG_FORMAT,
  ERR_CODE_INVALID_IPV4,
  ERR_CODE_INVALID_PORT,
  ERR_CODE_ARG_STR_MAX,
  ERR_CODE_ARG_BIN_MAX,
  ERR_CODE_ARG_FORMAT_HEX,
  ERR_CODE_ARG_FORMAT_BIN,
  ERR_CODE_BUSY
};
typedef enum error_code_e error_code_t;

enum arg_int32_e
{
  ARG_INT32_NONE = 0,
  ARG_INT32_NUM1,
  ARG_INT32_NUM2,
  ARG_INT32_NUM3,
  ARG_INT32_NUM4
};

#define MAX_ARG_SIZE 12 // String buffer that can store max. int32 or double with 11 characters

struct result_code_s
{
  error_code_t code;
  char strArgNum[2];
  char strArg1[MAX_ARG_SIZE];
  char strArg2[MAX_ARG_SIZE];
};
typedef struct result_code_s result_code_t;

result_code_t pack_result_code(const error_code_t& errorCode, const uint8_t iArgNum = 1, const char* strArg1 = "", const char* strArg2 = "");
result_code_t store_arg1_int32(result_code_t& resultCode, const int32_t iVal32);
result_code_t store_arg2_int32(result_code_t& resultCode, const int32_t iVal32);
result_code_t store_arg1_dbl(result_code_t& resultCode, const double dblArg, const int8_t iDecimals = MAX_DBL_STRING_PREC);
result_code_t store_arg2_dbl(result_code_t& resultCode, const double dblArg, const int8_t iDecimals = MAX_DBL_STRING_PREC);
result_code_t check_arguments(const char *args, const uint8_t arg_count, const char* delim = " ");
result_code_t get_bool_from_string(const char *str, bool *bValue, const uint8_t iArgNum);
result_code_t get_int32_from_string(const char *str, int32_t *iValue, const int32_t iMinVal, const int32_t iMaxVal, const uint8_t iArgNum);
result_code_t get_uint32_from_string(const char *str, uint32_t *iValue, const uint32_t iMinVal, const uint32_t iMaxVal, const uint8_t iArgNum);
result_code_t get_double_from_string(const char *str, double *dblValue, const double dblMinVal, const double dblMaxVal, const uint8_t iArgNum, const int8_t iDecimals = -1);
result_code_t get_byte_from_hex(const char *str, uint8_t *dataByte, const uint8_t iArgNum);
result_code_t get_byte_from_bin(const char *str, uint8_t *dataByte, const uint8_t iArgNum);
result_code_t get_bytes_from_bin_or_hex_string(const char *str, uint8_t *buf, const uint8_t iMaxBitSize, const size_t iBufSize, const uint8_t iArgNum);
result_code_t get_ipv4_from_string(const char* str, uint8_t* ipv4, const uint8_t iArgNum);

void get_error_string(const result_code_t& resultCode, char *strResult, const bool bAppend = false);

result_code_t parse_decimal_arg(const char* strArgs, const int32_t iMinIntSize, const int32_t iMaxIntSize, int32_t* iVal32);
result_code_t parse_hex_or_bin_arg(const char* strArgs, const uint8_t iMaxBitSize, uint8_t* pBuf, size_t iBufSize);
result_code_t parse_ipv4_arg(const char* strArgs, uint8_t* pIPAddress);

#endif // COMMAND_PARSER_H
