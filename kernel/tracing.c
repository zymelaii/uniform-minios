#include <unios/tracing.h>
#include <unios/sync.h>
#include <unios/schedule.h>
#include <unios/assert.h>
#include <arch/x86.h>

static spinlock_t        klog_lock;
static cb_klog_handler_t klog_handler;
static void             *klog_handler_user;

void klog_set_handler(cb_klog_handler_t handler, void *user) {
    lock_or(&klog_lock, schedule);
    klog_handler      = handler;
    klog_handler_user = user;
    release(&klog_lock);
}

static void _klog(int level, const char *fmt, va_list ap) {
    lock_or(&klog_lock, schedule);
    klog_handler(klog_handler_user, level, fmt, ap);
    release(&klog_lock);
}

void klog(int level, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _klog(level, fmt, ap);
    va_end(ap);
}

void kvlog(int level, const char *fmt, va_list ap) {
    _klog(level, fmt, ap);
}

void kfatal(const char *fmt, ...) {
    disable_int();
    clear_dir_flag();
    va_list ap;
    va_start(ap, fmt);
    _klog(KLOGLEVEL_FATAL, fmt, ap);
    va_end(ap);
    while (true) { halt(); }
    unreachable_silent();
}

void kvfatal(const char *fmt, va_list ap) {
    disable_int();
    clear_dir_flag();
    _klog(KLOGLEVEL_FATAL, fmt, ap);
    while (true) { halt(); }
    unreachable_silent();
}
