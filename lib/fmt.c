#include <stddef.h>
#include <fmt.h>

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>

int strfmtcb(
    strfmt_handler_t *handler, char *buf, int size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int resp = vstrfmtcb(handler, buf, size, fmt, ap);
    va_end(ap);
    return resp;
}

int vstrfmtcb(
    strfmt_handler_t *handler,
    char             *buf,
    int               size,
    const char       *fmt,
    va_list           ap) {
    if (handler == NULL) { return -1; }
    int resp =
        stbsp_vsprintfcb(handler->callback, handler->user, buf, fmt, ap, size);
    return resp;
}

int nstrfmt(char *buf, int n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int resp = stbsp_vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return resp;
}

int vnstrfmt(char *buf, int n, const char *fmt, va_list ap) {
    return stbsp_vsnprintf(buf, n, fmt, ap);
}
