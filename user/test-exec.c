#include <sys/sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

const int screen_lock_uid = 0xcafebabe;

int sync_printf(const char *fmt, ...) {
    va_list ap;
    int     rc;
    va_start(ap, fmt);
    if (krnlobj_lookup(screen_lock_uid) != INVALID_HANDLE) {
        krnlobj_lock(screen_lock_uid);
        rc = vprintf(fmt, ap);
        krnlobj_unlock(screen_lock_uid);
    } else {
        rc = vprintf(fmt, ap);
    }
    va_end(ap);
    return rc;
}

#define printf sync_printf

int main(int argc, char *argv[], char *envp[]) {
    clock_t time = clock();
    int     i    = 0;

    bool exec_once     = false;
    int  expect_retval = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-once") == 0) {
            exec_once = true;
        } else if (strcmp(argv[i], "-code") == 0) {
            if (++i >= argc) { break; }
            expect_retval = argv[i][0] - '0';
        }
    }

    printf("[%d.%03d] exec { ", time / 1000, time % 1000);
    printf("pid: %d, ", get_pid());
    printf("argc: %d, ", argc);
    printf("argv: [ ");
    for (int i = 0; i < argc; ++i) { printf("\"%s\", ", argv[i]); }
    printf("%p, ", argv[argc]);
    printf("], ");
    printf("envp: [ ");
    while (envp[i] != NULL) { printf("\"%s\", ", envp[i++]); }
    printf("%p, ", envp[i]);
    printf("], ");
    printf("}\n");

    int total_envs = i;
    time           = clock();
    if (envp[0] != NULL) {
        char *const *envp_dup = getenv();
        printf("[%d.%03d] dump envs { ", time / 1000, time % 1000);
        printf("pid: %d, ", get_pid());
        printf("[ ");
        for (int i = 0; i < total_envs; ++i) { printf("\"%s\", ", envp[i]); }
        printf("%p, ", envp[total_envs]);
        printf("] }\n");
    }

    if (!exec_once) {
        int resp = exec("test-exec");
        exit(resp);
    }

    return expect_retval;
}
