#include <unios/tracing_handler.h>
#include <unios/syscall.h>
#include <unios/serial.h>
#include <unios/proc.h>
#include <stddef.h>
#include <stdio.h>
#include <fmt.h>
#include <time.h>

static const char *klog_strlevel(int level) {
    switch (level) {
        case KLOGLEVEL_TRACE: {
            return "TRACE";
        } break;
        case KLOGLEVEL_INFO: {
            return "INFO";
        } break;
        case KLOGLEVEL_WARN: {
            return "WARN";
        } break;
        case KLOGLEVEL_ERROR: {
            return "ERROR";
        } break;
        case KLOGLEVEL_FATAL: {
            return "FATAL";
        } break;
        default: {
            return NULL;
        } break;
    }
}

static char *_klog_serial_handler(char *buf, void *user, int len) {
    int nl_cnt = *(int *)user;
    for (int i = 0; i < len; ++i) {
        if (buf[i] == '\n') {
            ++nl_cnt;
            continue;
        }
        for (int j = 0; j < nl_cnt; ++j) { serial_write('\n'); }
        nl_cnt = 0;
        serial_write(buf[i]);
    }
    *(int *)user = nl_cnt;
    return buf;
}

void klog_serial_handler(void *user, int level, const char *fmt, va_list ap) {
    const char *strlevel = klog_strlevel(level);
    if (strlevel == NULL) { return; }

    clock_t tm  = clock_from_sysclk(do_get_ticks());
    int     ms  = tm % 1000;
    int     sec = tm / 1000 % 60;
    int     min = tm / 60000 % 60;
    int     hr  = tm / 3600000 % 60;

    int              nl_cnt  = 0;
    strfmt_handler_t handler = {
        .callback = _klog_serial_handler,
        .user     = &nl_cnt,
    };

    char buf[64] = {};
    strfmtcb(
        &handler,
        buf,
        sizeof(buf),
        "[%02d:%02d:%02d.%03d][%s] ",
        hr,
        min,
        sec,
        ms,
        strlevel);
    vstrfmtcb(&handler, buf, sizeof(buf), fmt, ap);
    serial_write('\n');
}

static char *_klog_stderr_handler(char *buf, void *user, int len) {
    do_write(stderr, buf, len);
    return buf;
}

void klog_stderr_handler(void *user, int level, const char *fmt, va_list ap) {
    if (p_proc_current == NULL) { return; }
    if (p_proc_current->pcb.filp[stderr] == NULL) { return; }

    strfmt_handler_t handler = {
        .callback = _klog_stderr_handler,
        .user     = NULL,
    };

    char buf[64] = {};
    vstrfmtcb(&handler, buf, sizeof(buf), fmt, ap);

    //! TODO: klog can be a ultimate fatal logging, may be flush is required
}
