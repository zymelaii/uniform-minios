#pragma once

#include <macro_helper.h>

__attribute__((noreturn)) void
     _abort(const char* file, const char* func, int line, const char* fmt, ...);
void _warn(const char* file, const char* func, int line, const char* fmt, ...);

#define ASSERT_PINFO_IMPL(method, fmt, ...) \
    MH_EXPAND(MH_CONCAT(_, method)(         \
        __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__))
#define abort(fmt, ...) ASSERT_PINFO_IMPL(abort, fmt, ##__VA_ARGS__)
#define warn(fmt, ...)  ASSERT_PINFO_IMPL(warn, fmt, ##__VA_ARGS__)

#ifndef NDEBUG
#define assert(x)                                        \
    do {                                                 \
        if (!(x)) { abort("assertion failed: %s", #x); } \
    } while (0)
#else
#define assert(...)
#endif

#define unreachable() abort("unreachable")
#define todo(msg)     abort("todo: " msg)

#define unreachable_silent() __builtin_unreachable()

#define UNIMPLEMENTED_IMPL0()       warn("not implemented")
#define UNIMPLEMENTED_IMPL1(target) warn("not implemented: " target)
#define UNIMPLEMENTED_IMPL(N, ...) \
    MH_EXPAND(MH_CONCAT(UNIMPLEMENTED_IMPL, N)(__VA_ARGS__))
#define unimplemented(...) \
    MH_EXPAND(UNIMPLEMENTED_IMPL(MH_EXPAND(MH_NARG(__VA_ARGS__)), __VA_ARGS__))
