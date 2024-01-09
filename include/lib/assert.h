#pragma once

__attribute__((noreturn)) void
    _abort(const char *msg, const char *file, unsigned line);

#define abort(msg)    _abort(msg, __FILE__, __LINE__)
#define unreachable() abort("unreachable!")
#define todo(msg)     abort("todo: " msg)

#ifndef NDEBUG
#define assert(expr) \
 (void)((!!(expr)) || (abort("assertion failed: " #expr), 0))
#endif
