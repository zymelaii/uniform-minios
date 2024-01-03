#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

void _assert(const char *msg, const char *file, unsigned line) {
    printf("%s:%d: assertion failed: expect: %s\n", file, line, msg);
    exit(-1);
}
