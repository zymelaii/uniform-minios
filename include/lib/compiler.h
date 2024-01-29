#pragma once

#define auto __auto_type

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#define is_same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#define static_assert(expr, msg) _Static_assert(expr, msg)
