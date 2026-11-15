#ifndef STDINT_H
#define STDINT_H

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;

#define UINT32_MAX 0xFFFFFFFFU

_Static_assert(__CHAR_BIT__ == 8, "C byte must be 8 bits");
_Static_assert(sizeof(uint8_t) == 1, "uint8_t must be 8 bits");
_Static_assert(sizeof(uint16_t) == 2, "uint16_t must be 16 bits");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t must be 32 bits");
_Static_assert(sizeof(uint64_t) == 8, "uint64_t must be 64 bits");

#endif
