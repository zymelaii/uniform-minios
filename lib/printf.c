#include "stdio.h"

static void
printfputch(int ch, int *cnt)
{
	char buf = (char)ch; 
	write(STD_OUT, &buf, 1);
	(*cnt)++;
}

int
vprintf(const char *fmt, va_list ap)
{
	int cnt = 0;

	vprintfmt((void *)printfputch, &cnt, fmt, ap);
	
	return cnt;
}

int
printf(const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vprintf(fmt, ap);
	va_end(ap);

	return rc;
}

