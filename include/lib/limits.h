#pragma once

#include <stdint.h>

#define CHAR_BIT 8

#define U8_MAX  0xffuhh
#define U16_MAX 0xffffuh
#define U32_MAX 0xfffffffful
#define U64_MAX 0xffffffffffffffffull

#define I8_MAX  0x7fhh
#define I16_MAX 0x7fffh
#define I32_MAX 0x7fffffffl
#define I64_MAX 0x7fffffffffffffffll
#define I8_MIN  (-I8_MAX - 1)
#define I16_MIN (-I16_MAX - 1)
#define I32_MIN (-I32_MAX - 1)
#define I64_MIN (-I64_MAX - 1)

#define SCHAR_MAX  I8_MAX
#define SCHAR_MIN  I8_MIN
#define UCHAR_MAX  U8_MAX
#define SHORT_MAX  I16_MAX
#define SHORT_MIN  I16_MIN
#define USHORT_MAX U16_MAX
#define LONG_MAX   I32_MAX
#define LONG_MIN   I32_MIN
#define ULONG_MAX  U32_MAX
#define LLONG_MAX  I64_MAX
#define LLONG_MIN  I64_MIN
#define ULLONG_MAX U64_MAX

#define INT_MAX  LONG_MAX
#define INT_MIN  LONG_MIN
#define UINT_MAX ULONG_MAX

#define LONG_LONG_MAX  LLONG_MAX
#define LONG_LONG_MIN  LLONG_MIN
#define ULONG_LONG_MAX ULLONG_MAX

#define SSIZE_MAX I32_MAX
#define SIZE_MAX  U32_MAX

#define FILENAME_MAX 12
#define PATH_MAX     128
