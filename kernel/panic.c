#include <unios/tracing.h>
#include <unios/tracing_handler.h>
#include <arch/x86.h>
#include <stdarg.h>
#include <stdio.h>

void _panic(const char* file, int line, const char* fmt, ...) {
    klog_set_handler(klog_stderr_handler, NULL);

    va_list ap;
    va_start(ap, fmt);
    kerror("kernel panic at %s:%d: ", file, line);
    kverror(fmt, ap);
    va_end(ap);

    kfatal("\n");

    //! NOTE: ok, so now that the kernel is dead, nothing can be done except
    //! shutdown
}
