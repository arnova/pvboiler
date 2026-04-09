#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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

bool BytesToFloat(const byte* bytes, const unsigned int len, float& fResult);
bool BytesToInt32(const byte* bytes, const unsigned int len, int32_t& iResult);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UTIL_H
