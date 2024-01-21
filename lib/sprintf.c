#include <fmt.h>
#include <stdarg.h>

int snprintf(char *buf, int n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int resp = vnstrfmt(buf, n, fmt, ap);
    va_end(ap);
    return resp;
}

int vsnprintf(char *buf, int n, const char *fmt, va_list ap) {
    return vnstrfmt(buf, n, fmt, ap);
}
