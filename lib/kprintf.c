#include <unios/syscall.h>
#include <unios/proto.h>
#include <unios/uart.h>
#include <unios/spinlock.h>
#include <stdarg.h>
#include <stdio.h>

static spinlock_t trace_lock;

static void kprintfputch(int ch, void *b) {
    vga_write_char(ch, WHITE_CHAR);
}

int vkprintf(const char *fmt, va_list ap) {
    vprintfmt((void *)kprintfputch, NULL, fmt, ap);

    return 0;
}

int kprintf(const char *fmt, ...) {
    va_list ap;
    int     rc;

    va_start(ap, fmt);
    rc = vkprintf(fmt, ap);
    va_end(ap);

    return rc;
}

static void uart_putch(int ch, void *b) {
    write_serial(ch);
}

int v_uart_kprintf(const char *fmt, va_list ap) {
    vprintfmt((void *)uart_putch, NULL, fmt, ap);
    return 0;
}

int uart_kprintf(const char *fmt, ...) {
    va_list ap;
    int     rc;

    va_start(ap, fmt);
    rc = v_uart_kprintf(fmt, ap);
    va_end(ap);

    return rc;
}

int trace_logging(const char *fmt, ...) {
    va_list ap;
    int     rc;

    //! FIXME: user program and kernel both hold a copy, this lock action can
    //! only lock kernel/user-prog itself, while the true critical resource is
    //! serial device, so the lock action is expected to move into the place
    //! where serial io really works to ensure the exclusive access between
    //! kernel and user prog
    va_start(ap, fmt);
    uart_kprintf("[tick %5d] ", do_get_ticks());
    rc = v_uart_kprintf(fmt, ap);
    release(&trace_lock);
    va_end(ap);

    return rc;
}
