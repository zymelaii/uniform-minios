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

//! WARNING: make sure that your x, y expr no side-effect! recommand using
//! the min/max methods provided in the math.h
#define MIN(_a, _b)                \
    ({                             \
        __typeof__(_a) __a = (_a); \
        __typeof__(_b) __b = (_b); \
        __a <= __b ? __a : __b;    \
    })

#define MAX(_a, _b)                \
    ({                             \
        __typeof__(_a) __a = (_a); \
        __typeof__(_b) __b = (_b); \
        __a >= __b ? __a : __b;    \
    })

#define ROUNDDOWN(a, n)                 \
    ({                                  \
        u32 __a = (u32)(a);             \
        (__typeof__(a))(__a - __a % n); \
    })

#define ROUNDUP(a, n)                                        \
    ({                                                       \
        u32 __n = (u32)(n);                                  \
        (__typeof__(a))(ROUNDDOWN((u32)(a) + __n - 1, __n)); \
    })

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define offsetof(type, member) ((size_t)(&((type *)0)->member))
