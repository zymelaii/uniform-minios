#pragma once

void _panic(const char*, int, const char*, ...) __attribute__((noreturn));

#define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)

#define unreachable() panic("unreachable!")
#define todo(msg)     panic("todo: " msg)

#ifndef NDEBUG
#define assert(x)                              \
 do {                                          \
  if (!(x)) panic("assertion failed: %s", #x); \
 } while (0)
#else
#define assert(...)
#endif
