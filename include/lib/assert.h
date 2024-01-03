#pragma once

void _assert(const char *msg, const char *file, unsigned line);

#ifndef NDEBUG
#define assert(expr) \
 (void)((!!(expr)) || (_assert(#expr, __FILE__, __LINE__), 0))
#endif
