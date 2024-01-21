#include <fmt.h>
#include <stdio.h>
#include <stdarg.h>

static char *putch_handler(char *buf, void *user, int len) {
    int resp = write(stdout, buf, len);
    if (resp > 0) { *(int *)user += resp; }
    return buf;
}

int vprintf(const char *fmt, va_list ap) {
    int              cnt     = 0;
    char             buf[64] = {};
    strfmt_handler_t handler = {
        .callback = putch_handler,
        .user     = &cnt,
    };
    vstrfmtcb(&handler, buf, sizeof(buf), fmt, ap);
    return cnt;
}

int printf(const char *fmt, ...) {
    va_list ap;
    int     rc;
    va_start(ap, fmt);
    rc = vprintf(fmt, ap);
    va_end(ap);
    return rc;
}
