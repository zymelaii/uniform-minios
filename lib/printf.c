#include <unios/spinlock.h>
#include <stdlib.h>
#include <stdio.h>
#include <atomic.h>
#include <stdarg.h>

static void printfputch(int ch, int *cnt) {
    char buf = (char)ch;
    write(stdout, &buf, 1);
    ++*cnt;
}

int vprintf(const char *fmt, va_list ap) {
    int cnt = 0;
    vprintfmt((void *)printfputch, &cnt, fmt, ap);
    return cnt;
}

int printf(const char *fmt, ...) {
    static int lock = 0;
    lock_or_yield(&lock);
    va_list ap;
    int     rc;
    va_start(ap, fmt);
    rc = vprintf(fmt, ap);
    va_end(ap);
    lock = 0;
    release(&lock);
    return rc;
}
