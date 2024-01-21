#pragma once

#include <stdarg.h>

typedef char *(*cb_strfmt_t)(char *, void *, int);

typedef struct strfmt_handler_s {
    cb_strfmt_t callback;
    void       *user;
} strfmt_handler_t;

int strfmtcb(
    strfmt_handler_t *handler, char *buf, int size, const char *fmt, ...);
int vstrfmtcb(
    strfmt_handler_t *handler,
    char             *buf,
    int               size,
    const char       *fmt,
    va_list           ap);
int nstrfmt(char *buf, int n, const char *fmt, ...);
int vnstrfmt(char *buf, int n, const char *fmt, va_list ap);
