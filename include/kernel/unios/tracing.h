#pragma once

#include <stdarg.h>

enum tracing_level {
    KLOGLEVEL_TRACE,
    KLOGLEVEL_INFO,
    KLOGLEVEL_WARN,
    KLOGLEVEL_ERROR,
    KLOGLEVEL_FATAL,
};

typedef void (*cb_klog_handler_t)(void *, int, const char *, va_list);

void klog_set_handler(cb_klog_handler_t handler, void *user);

void klog(int level, const char *fmt, ...);
void kvlog(int level, const char *fmt, va_list ap);

void kfatal(const char *fmt, ...) __attribute__((noreturn));
void kvfatal(const char *fmt, va_list ap) __attribute__((noreturn));

#define ktrace(fmt, ...) klog(KLOGLEVEL_TRACE, (fmt), ##__VA_ARGS__)
#define kinfo(fmt, ...)  klog(KLOGLEVEL_INFO, (fmt), ##__VA_ARGS__)
#define kwarn(fmt, ...)  klog(KLOGLEVEL_WARN, (fmt), ##__VA_ARGS__)
#define kerror(fmt, ...) klog(KLOGLEVEL_ERROR, (fmt), ##__VA_ARGS__)

#define kvtrace(fmt, ap) kvlog(KLOGLEVEL_TRACE, (fmt), (ap))
#define kvinfo(fmt, ap)  kvlog(KLOGLEVEL_INFO, (fmt), (ap))
#define kvwarn(fmt, ap)  kvlog(KLOGLEVEL_WARN, (fmt), (ap))
#define kverror(fmt, ap) kvlog(KLOGLEVEL_ERROR, (fmt), (ap))

#ifndef NDEBUG
#define kdebug(fmt, ...) ktrace((fmt), ##__VA_ARGS__)
#else
#define kdebug(...)
#endif
