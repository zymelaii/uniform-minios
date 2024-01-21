#include <unios/tracing.h>
#include <stdarg.h>

/*!
 * \brief redirect tracing logs to serial dev
 */
void klog_serial_handler(void *user, int level, const char *fmt, va_list ap);

/*!
 * \brief redirect tracing logs to stderr of current console
 */
void klog_stderr_handler(void *user, int level, const char *fmt, va_list ap);
