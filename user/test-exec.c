#include <sys/sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "screen_sync.h"

int main(int argc, char *argv[], char *envp[]) {
    clock_t time = clock();
    int     i    = 0;

    bool exec_once     = false;
    int  expect_retval = 0;
    bool no_env        = false;
    bool minimal       = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-once") == 0) {
            exec_once = true;
        } else if (strcmp(argv[i], "-code") == 0) {
            if (++i >= argc) { break; }
            expect_retval = argv[i][0] - '0';
        } else if (strcmp(argv[i], "-no-env") == 0) {
            no_env = true;
        } else if (strcmp(argv[i], "-min") == 0) {
            minimal = true;
            no_env  = true;
        }
    }

    begin_exclusive_screen();

    printf("[%d.%03d] exec { ", time / 1000, time % 1000);
    printf("pid: %d, ", get_pid());
    printf("argc: %d, ", argc);
    if (!minimal) {
        printf("argv: [ ");
        for (int i = 0; i < argc; ++i) { printf("\"%s\", ", argv[i]); }
        printf("%p, ", argv[argc]);
        printf("], ");
    }
    if (!no_env) {
        printf("envp: [ ");
        while (envp[i] != NULL) { printf("\"%s\", ", envp[i++]); }
        printf("%p, ", envp[i]);
        printf("], ");
    }
    printf("}\n");

    if (!no_env) {
        int total_envs = i;
        time           = clock();
        if (envp[0] != NULL) {
            char *const *envp_dup = getenv();
            printf("[%d.%03d] dump envs { ", time / 1000, time % 1000);
            printf("pid: %d, ", get_pid());
            printf("[ ");
            for (int i = 0; i < total_envs; ++i) {
                printf("\"%s\", ", envp[i]);
            }
            printf("%p, ", envp[total_envs]);
            printf("] }\n");
        }
    }

    end_exclusive_screen();

    if (!exec_once) {
        int resp = exec("test-exec");
        exit(resp);
    }

    return expect_retval;
}
