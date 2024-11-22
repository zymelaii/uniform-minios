#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <fmt.h>

typedef struct fmtinfo_args {
    const char *color_csi;
    const char *slevel;
    const char *file;
    const char *func;
    int         line;
} fmtinfo_args_t;

static void fmtinfo(
    char           *buffer,
    size_t          size,
    fmtinfo_args_t *args,
    const char     *fmt,
    va_list         ap) {
    int off  = 0;
    off     += nstrfmt(
        buffer,
        size,
        "\e[0m%s%s:%s:%d: %s: ",
        args->color_csi,
        args->file,
        args->func,
        args->line,
        args->slevel);
    off += vnstrfmt(buffer + off, size - off, fmt, ap);
    off += nstrfmt(buffer + off, size - off, "\e[0m\n");
    //! ATTENTION: total off must be less than size, but do not assert here due
    //! to potential recursion call
}

void _abort(
    const char *file, const char *func, int line, const char *fmt, ...) {
    fmtinfo_args_t args = {
        .color_csi = "\e[91m",
        .slevel    = "fatal",
        .file      = file,
        .func      = func,
        .line      = line,
    };
    char buffer[256] = {};

    va_list ap;
    va_start(ap, fmt);
    fmtinfo(buffer, sizeof(buffer), &args, fmt, ap);
    va_end(ap);

    printf("%s", buffer);

    exit(-1);
}

void _warn(const char *file, const char *func, int line, const char *fmt, ...) {
    fmtinfo_args_t args = {
        .color_csi = "\e[93m",
        .slevel    = "warn",
        .file      = file,
        .func      = func,
        .line      = line,
    };
    char buffer[256] = {};

    va_list ap;
    va_start(ap, fmt);
    fmtinfo(buffer, sizeof(buffer), &args, fmt, ap);
    va_end(ap);

    printf("%s", buffer);
}
