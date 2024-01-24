#pragma once

#define idiv_floor(x, ud)                 \
 ({                                       \
__auto_type _x  = x;                       \
__auto_type _ud = ud;                      \
_x >= 0 ? _x / _ud : (_x - _ud + 1) / _ud; \
 })

#define idiv_ceil(x, ud)                 \
 ({                                      \
__auto_type _x  = x;                      \
__auto_type _ud = ud;                     \
_x > 0 ? (_x + _ud - 1) / _ud : _x / _ud; \
 })

#define min(x, y)  \
 ({                \
__auto_type _x = x; \
__auto_type _y = y; \
_x < _y ? _x : _y;  \
 })

#define max(x, y)  \
 ({                \
__auto_type _x = x; \
__auto_type _y = y; \
_x > _y ? _x : _y;  \
 })
