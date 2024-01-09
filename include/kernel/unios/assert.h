#pragma once

void _warn(const char*, int, const char*, ...);
void _panic(const char*, int, const char*, ...) __attribute__((noreturn));

#define warn(...)  _warn(__FILE__, __LINE__, __VA_ARGS__)
#define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)

#define unreachable() panic("unreachable!")
#define todo(msg)     panic("todo: " msg)

#ifndef NDEBUG
#define assert(x)                              \
 do {                                          \
  if (!(x)) panic("assertion failed: %s", #x); \
 } while (0)
#endif
