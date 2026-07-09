#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define CH_BACKSPACE   0x08
#define CH_ESCAPE      0x1B
#define CH_DELETE      0x7F // Terminal/PuTTY alternative for backspace
#define CH_CR          0x0D // 13
#define CH_LF          0x0A // 10

/*
  Various handy functions/macros
*/

// Helper functions to make a numeric define into a string define
#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

// Some handy macros for string handling
#define STREQUALS(a,b) (strcmp(a, b) == 0)
#define STRIEQUALS(a,b) (strcasecmp(a, b) == 0)

// std::min/max equivalent function macro's
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// Get the integer size of n-bits
#define BIT_INT_SIZE(n) ((1 << n) - 1)

// Divide and round up
#define DIV_CEIL(x,y) ((x / y) + (x % y == 0 ? 0 : 1))

/* Macros for getting (sub)bytes from e.g. bigger numbers */
#define LOBYTE(x)     (x & 0xFF)
#define HIBYTE(x)     ((x >> 8) & 0xFF)
#define SUBBYTE(x, n) ((char*)(&x))[n]

uint32_t x_to_y(const uint32_t iX, const uint32_t iY);
bool check_bit_size(const uint8_t *buf, const size_t iBufSize, const uint8_t iMaxBitSize);
char *strsplit(char *str, const char* delims);
int strcount(const char *str, const char* delims);
char *strtrimleft(char *str, const char *chrstr);
char *strtrimright(char *str, const char *chrstr);
char *strtrim(char *str, const char *chrstr);
int strfind(const char *str, const char *substr);
int strcmpat(const char *str, const uint8_t start, const char *substr);
char *strncpy0(char *dest, const char *source, const uint8_t n);
char *int32_to_decstr(int32_t iNum, char *strDec, const size_t iBufSize);
char *uint32_to_decstr(uint32_t iNum, char *strDec, const size_t iBufSize);
char *int64_to_decstr(int64_t iNum, char *strDec, const size_t iBufSize);
char *uint64_to_decstr(uint64_t iNum, char *strDec, const size_t iBufSize);
char *uint64_to_binstr(uint64_t iNum, char *strBin, const uint8_t iLen);
char *uint64_to_hexstr(uint64_t iNum, char *strHex, const uint8_t iLen);
char *bytes_to_hexstr(const uint8_t *byteArray, char *strHex, const uint8_t iLen);
char *dbl_to_dblstr(const double dblNum, char *strDbl, const int8_t iDecimals, const size_t iBufSize);
bool dblstr_to_double(const char *str, double *pValue);
bool decstr_to_int32(const char *str, int32_t *pValue);
bool decstr_to_uint32(const char *str, uint32_t *pValue);
bool binstr_to_uint8(const char *strBin, uint8_t *pValue);
bool hexstr_to_uint8(const char *strHex, uint8_t *pValue);
bool binstr_to_uint64(const char *strBin, uint64_t *pValue);
bool hexstr_to_uint64(const char *strHex, uint64_t *pValue);
bool binstr_to_bytes(const char *strBin, uint8_t *pBuf, const size_t iBufSize);
bool hexstr_to_bytes(const char *strHex, uint8_t *pBuf, const size_t iBufSize);
char* bytes_to_ipstr(const uint8_t *bufIP, char* strIP);
bool bytes_to_int32(const uint8_t* buf, const uint8_t iBufSize, int32_t* pValue);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UTIL_H
