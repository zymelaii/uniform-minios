#pragma once

#include <stddef.h>

#define idiv_floor(x, ud)                          \
    ({                                             \
        __auto_type _x  = x;                       \
        __auto_type _ud = ud;                      \
        _x >= 0 ? _x / _ud : (_x - _ud + 1) / _ud; \
    })

#define idiv_ceil(x, ud)                          \
    ({                                            \
        __auto_type _x  = x;                      \
        __auto_type _ud = ud;                     \
        _x > 0 ? (_x + _ud - 1) / _ud : _x / _ud; \
    })

#define min(x, y)           \
    ({                      \
        __auto_type _x = x; \
        __auto_type _y = y; \
        _x < _y ? _x : _y;  \
    })

#define max(x, y)           \
    ({                      \
        __auto_type _x = x; \
        __auto_type _y = y; \
        _x > _y ? _x : _y;  \
    })

#define round_down(a, n)                \
    ({                                  \
        size_t __a = (size_t)(a);       \
        (__typeof__(a))(__a - __a % n); \
    })

#define round_up(a, n)                                           \
    ({                                                           \
        size_t __n = (size_t)(n);                                \
        (__typeof__(a))(round_down((size_t)(a) + __n - 1, __n)); \
    })
