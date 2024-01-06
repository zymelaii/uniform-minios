#include <arch/x86.h>
#include <stdarg.h>
#include <stdio.h>

void _panic(const char* file, int line, const char* fmt, ...) {
    disable_int();
    clear_dir_flag();
    va_list ap;
    va_start(ap, fmt);
    kprintf("kernel panic at %s:%d: ", file, line);
    vkprintf(fmt, ap);
    kprintf("\n");
    va_end(ap);
    while (1) { halt(); }
}

void _warn(const char* file, int line, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    kprintf("kernel warning at %s:%d: ", file, line);
    vkprintf(fmt, ap);
    kprintf("\n");
    va_end(ap);
}
