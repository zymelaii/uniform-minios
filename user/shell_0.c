#include <sys/defs.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

void setup_for_all_tty() {
    int nr_tty = 0;
    while (nr_tty + 1 < NR_CONSOLES) {
        if (fork() == 0) {
            ++nr_tty;
            continue;
        }
        break;
    }

    char tty[PATH_MAX] = {};
    snprintf(tty, sizeof(tty), "/dev_tty%d", nr_tty);
    int fd[3] = {};
    for (int i = 0; i < 3; ++i) { fd[i] = open(tty, O_RDWR); }
    assert(fd[0] == stdin && fd[1] == stdout && fd[2] == stderr);
}

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
    }
    return false;
}

int main(int arg, char *argv[]) {
    setup_for_all_tty();

    char buf[PATH_MAX] = {};
    while (true) {
        printf("unios[%d]:/ $ ", get_pid());
        gets(buf);
        int    cmd_argc = 0;
        char **cmd_argv = NULL;
        bool   ok       = arg_from_cmdline(buf, &cmd_argc, &cmd_argv);
        if (!ok) { continue; }

        ok = route(cmd_argc, cmd_argv);
        while (!ok) {
            const u32 ENOTFOUND = 114;
            pid_t     pid       = fork();
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
