#include <sys/defs.h>
#include <assert.h>
#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include "screen_sync.h"

bool arg_from_cmdline(const char *buf, int *p_argc, char ***p_argv) {
    assert(buf != NULL);
    assert(p_argc != NULL);
    assert(p_argv != NULL);

    char *p    = (char *)buf;
    char *q    = NULL;
    char *tail = NULL;

    //! trim first
    //! NOTE: blank is the only whitespace code currently
    while (*p == ' ') { ++p; }
    if (*p == '\0') { return false; }
    tail = p;
    while (*++tail != '\0') {}
    while (*--tail == ' ') {}

    //! NOTE: here we use CSTART to mark the begin of a string, since
    //! zero-length str needs more info to identify
    const char CSTART = 0xfe;
    const char CZERO  = '\0';

    char *argbuf = (char *)malloc(tail - p + 4);
    assert(argbuf != NULL);
    memcpy(argbuf + 1, p, tail - p + 1);
    tail = &argbuf[tail - p + 1];
    p    = &argbuf[1];

    //! boundary for convenient loop
    tail[1] = CZERO;
    tail[2] = CSTART;
    p[-1]   = CSTART;

    *p_argc = 0;

    //! NOTE: here we simply ignore escape code and accept it as raw code
    while (p <= tail) {
        if (p[-1] != CSTART) {
            assert(p[-1] == CZERO);
            p[-1] = CSTART;
        }
        q = p;

        //! NOTE: both `"` and `'` is available, but a quoted string must with a
        //! pair of same-styled quote
        char quote = *p == '"' || *p == '\'' ? *p : 0;
        if (quote != 0) {
            while (*++p != quote && p <= tail) {}
            assert(p <= tail && "incomplete quoted string");
            q[-1] = CZERO;
            q[0]  = CSTART; //<! shift the start
            *p    = CZERO;
        } else {
            while (*++p != ' ' && p <= tail) {}
            --p;
        }
        assert(p == tail || p[1] == ' ' && "expect whitespace between args");

        ++*p_argc;

        //! NOTE: `p <= tail` is always true since `*tail != ' '`, unless the
        //! last iter case
        while (*++p == ' ') { *p = CZERO; }
    }

    p = &argbuf[1 + (argbuf[0] == CZERO)];
    assert(p[-1] == CSTART);
    assert(p[0] != ' ');

    *p_argv = (char **)malloc((*p_argc + 1) * sizeof(char *));
    assert(*p_argv != NULL);
    for (int i = 0; i < *p_argc; ++i) {
        q = p + 1;
        while (*q != CSTART && *q != CZERO) { ++q; }
        int size     = q - p;
        (*p_argv)[i] = (char *)malloc(size + 1);
        assert((*p_argv)[i] != NULL);
        (*p_argv)[i][q - p] = 0;
        memcpy((*p_argv)[i], p, size);
        p += size;
        assert(*p == CZERO || *p == CSTART);
        while (*p != CSTART) { ++p; }
        ++p;
    }
    (*p_argv)[*p_argc] = NULL;

    return true;
}

int arg_free(char **argv) {
    assert(argv != NULL);
    char **arg  = argv;
    int    argc = 0;
    while (*arg != NULL) {
        ++argc;
        free(*arg);
        ++arg;
    }
    free(argv);
    return argc;
}

void print_exec_info(int argc, char **argv) {
    assert(argc > 0);
    assert(argv != NULL);
    printf("exec {\n");
    printf("  command: \"%s\",\n", argv[0]);
    do {
        if (argc == 1) {
            printf("  arguments: [],\n");
            break;
        }
        printf("  arguments: [\n");
        for (int i = 1; i < argc; ++i) { printf("    \"%s\",\n", argv[i]); }
        printf("  ],\n");
    } while (0);
    printf("}\n");
}

void info_handler(int argc, char *argv[]) {
    if (argc == 0) {
        printf("warn: expect info type\n");
        return;
    }
    if (strcmp(argv[0], "env") == 0) {
        char *const *envp = getenv();
        assert(envp != NULL);
        char *const *p_env = envp;
        printf("envs [\n");
        while (true) {
            printf("  \"%s\",\n", *p_env);
            if (*p_env++ == NULL) { break; }
        }
        printf("]\n");
    } else if (strcmp(argv[0], "pid") == 0) {
        printf("{\n");
        printf("  pid: %d,\n", get_pid());
        printf("  ppid: %d,\n", get_ppid());
        printf("}\n");
    } else if (strcmp(argv[0], "clock") == 0) {
        int tick = get_ticks();
        printf("{\n");
        printf("  system ticks: %d tick,\n", tick);
        printf("  running time clock: %d ms,\n", clock_from_sysclk(tick));
        printf("  cpu clock frequency: %d Hz,\n", SYSCLK_FREQ_HZ);
        printf("}\n");
    } else if (strcmp(argv[0], "help") == 0) {
        printf("available info types:\n");
        printf("  help   print this help info\n");
        printf("  env    current environments\n");
        printf("  pid    pid & ppid of current proc\n");
        printf("  clock  current time status\n");
    } else {
        printf("warn: unknown info type `%s`\n", argv[0]);
    }
}

bool route(int argc, char *argv[]) {
    assert(argc > 0 && argv != NULL);
    if (argc == 1 && strcmp(argv[0], "exit") == 0) {
        exit(0);
        assert(false && "unreachable");
    } else if (strcmp(argv[0], "parse-cmd") == 0) {
        --argc;
        ++argv;
        if (argc > 0) {
            print_exec_info(argc, argv);
        } else {
            printf("error: expect cmdline\n");
        }
        return true;
    } else if (strcmp(argv[0], "info") == 0) {
        info_handler(argc - 1, argv + 1);
        return true;
    } else if (strcmp(argv[0], "echo") == 0) {
        for (int i = 1; i < argc; ++i) { printf("%s\n", argv[i]); }
        return true;
    } else if (strcmp(argv[0], "new-env") == 0) {
        if (argc == 1) {
            printf("error: expect env\n");
            return true;
        }
        char *const env[2] = {argv[1], NULL};
        bool        ok     = putenv(env);
        printf("info: update env %s\n", ok ? "done" : "failed");
        return true;
    } else if (strcmp(argv[0], "sync") == 0) {
        int enabled = -1;
        if (argc >= 2) {
            if (strcmp(argv[1], "enable") == 0) {
                enabled = 1;
            } else if (strcmp(argv[1], "disable") == 0) {
                enabled = 0;
            }
        }
        if (enabled == -1) {
            printf("info: sync [ enable | disable ]\n");
            return true;
        }
        if (enabled && krnlobj_lookup(screen_lock_uid) == INVALID_HANDLE) {
            init_exclusive_screen();
            assert(krnlobj_lookup(screen_lock_uid) != INVALID_HANDLE);
        }
        if (!enabled) {
            handle_t handle = krnlobj_lookup(screen_lock_uid);
            if (handle != INVALID_HANDLE) { krnlobj_destroy(handle); }
            assert(krnlobj_lookup(screen_lock_uid) == INVALID_HANDLE);
        }
        return true;
    }
    return false;
}

int main(int arg, char *argv[]) {
    char buf[PATH_MAX] = {};
    while (true) {
        printf("unios$ ");
        gets(buf);
        int    cmd_argc = 0;
        char **cmd_argv = NULL;
        bool   ok       = arg_from_cmdline(buf, &cmd_argc, &cmd_argv);
        if (!ok) { continue; }

        ok = route(cmd_argc, cmd_argv);
        while (!ok) {
            const uint32_t ENOTFOUND = 114;
            pid_t          pid       = fork();
            if (pid < 0) {
                printf("warn: pcb res not available\n");
                ok = true;
                break;
            }
            if (pid == 0) {
                int errno = execve(cmd_argv[0], &cmd_argv[1], NULL);
                exit(ENOTFOUND);
                assert(false && "unreachable");
            }
            int   status     = 0;
            pid_t waited_pid = wait(&status);
            assert(waited_pid == pid);
            if (status != ENOTFOUND) {
                printf("info: `%s` exit with %d\n", cmd_argv[0], status);
                ok = true;
            }
            break;
        }
        if (!ok) { printf("unknown command: `%s`\n", cmd_argv[0]); }

        int n = arg_free(cmd_argv);
        assert(n == cmd_argc);
    }

    return 0;
}
