#include "stdio.h"
#include "protect.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "uart.h"

static void 
kprintfputch(int ch, void *b)
{
	vga_write_char(ch, WHITE_CHAR);
}

int
vkprintf(const char *fmt, va_list ap)
{

	vprintfmt((void*)kprintfputch, NULL, fmt, ap);

	return 0;
}

int
kprintf(const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vkprintf(fmt, ap);
	va_end(ap);

	return rc;
}

static void
uart_putch(int ch, void *b) {
	write_serial(ch);
}


int
v_uart_kprintf(const char *fmt, va_list ap)
{
	vprintfmt((void*)uart_putch, NULL, fmt, ap);
	return 0;
}

int uart_kprintf(const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = v_uart_kprintf(fmt, ap);
	va_end(ap);

	return rc;
}