#pragma once

#include <stdint.h>
#include <stdbool.h>

#define NULL ((void *)0)

#define TRUE  true
#define FALSE false

typedef uint32_t size_t;
typedef int32_t  ssize_t;

typedef int32_t  intptr_t;
typedef uint32_t uintptr_t;

typedef int32_t ptrdiff_t;

#define MIN(_a, _b)       \
 ({                       \
__typeof__(_a) __a = (_a); \
__typeof__(_b) __b = (_b); \
__a <= __b ? __a : __b;    \
 })

#define MAX(_a, _b)       \
 ({                       \
__typeof__(_a) __a = (_a); \
__typeof__(_b) __b = (_b); \
__a >= __b ? __a : __b;    \
 })

#define ROUNDDOWN(a, n)        \
 ({                            \
uint32_t __a = (uint32_t)(a);   \
(__typeof__(a))(__a - __a % n); \
 })

#define ROUNDUP(a, n)                                    \
 ({                                                      \
uint32_t __n = (uint32_t)(n);                             \
(__typeof__(a))(ROUNDDOWN((uint32_t)(a) + __n - 1, __n)); \
 })

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define offsetof(type, member) ((size_t)(&((type *)0)->member))
