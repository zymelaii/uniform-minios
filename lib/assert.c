#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

void _abort(const char *msg, const char *file, unsigned line) {
    printf("%s:%d: %s\n", file, line, msg);
    exit(-1);
    __builtin_unreachable();
}
