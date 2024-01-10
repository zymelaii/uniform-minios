#include <unios/syscall.h>
#include <unios/uart.h>
#include <unios/vga.h>
#include <unios/assert.h>
#include <unios/schedule.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <atomic.h>

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

int klog(const char *fmt, ...) {
    va_list ap;
    int     rc;

    clock_t tm  = clock_from_sysclk(do_get_ticks());
    int     ms  = tm % 1000;
    int     sec = tm / 1000 % 60;
    int     min = tm / 60000 % 60;
    int     hr  = tm / 3600000 % 60;

    static u32 lock = 0;
    va_start(ap, fmt);
    lock_or(&lock, schedule);
    uart_kprintf("[%02d:%02d:%02d.%03d][INFO] ", hr, min, sec, ms);
    rc = v_uart_kprintf(fmt, ap);
    release(&lock);
    va_end(ap);

    return rc;
}
