#include <type.h>
#include <const.h>
#include <protect.h>
#include <string.h>
#include <proc.h>
#include <global.h>
#include <proto.h>
#include <stdio.h>

#define assert(expected)                                     \
 do {                                                        \
  if (expected) { break; }                                   \
  const char *file = __FILE__;                               \
  int         line = __LINE__;                               \
  const char *expr = #expected;                              \
  printf("%s:%d: assertion failed: %s\n", file, line, expr); \
  __asm__ volatile("hlt");                                   \
 } while (0);

#define CHECK_PTR(p) assert((p) != NULL)

void setup_for_all_tty() {
    int nr_tty = 0;
    while (nr_tty + 1 < NR_CONSOLES) {
        if (fork() == 0) {
            ++nr_tty;
            continue;
        }
        break;
    }

    char tty[MAX_PATH] = {};
    snprintf(tty, sizeof(tty), "/dev_tty%d", nr_tty);
    int stdin  = open(tty, O_RDWR);
    int stdout = open(tty, O_RDWR);
    int stderr = open(tty, O_RDWR);
    assert(stdin == 0 && stdout == 1 && stderr == 2);
}

bool arg_from_cmdline(const char *buf, int *p_argc, char ***p_argv) {
    CHECK_PTR(buf);
    CHECK_PTR(p_argc);
    CHECK_PTR(p_argv);

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
    CHECK_PTR(argbuf);
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
    CHECK_PTR(*p_argv);
    for (int i = 0; i < *p_argc; ++i) {
        q = p + 1;
        while (*q != CSTART && *q != CZERO) { ++q; }
        int size     = q - p;
        (*p_argv)[i] = (char *)malloc(size + 1);
        CHECK_PTR((*p_argv)[i]);
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
    CHECK_PTR(argv);
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
    CHECK_PTR(argv);
    printf("exec {\n");
    printf("  command: \"%s\",", argv[0]);
    do {
        if (argc == 1) {
            printf("  arguments: [],\n");
            break;
        }
        printf("  arguments: [");
        for (int i = 1; i < argc; ++i) { printf("    \"%s\",\n", argv[i]); }
        printf("  ]");
    } while (0);
    printf("}\n");
}

int main(int arg, char *argv[]) {
    setup_for_all_tty();

    char buf[MAX_PATH] = {};
    while (1) {
        printf("miniOS:/ $ ");
        gets(buf);

        int    argc = 0;
        char **argv = NULL;
        bool   ok   = arg_from_cmdline(buf, &argc, &argv);
        if (!ok) { continue; }

        print_exec_info(argc, argv);

        if (exec(buf) != 0) {
            printf("exec failed: file not found!\n");
            continue;
        }

        int n = arg_free(argv);
        assert(n == argc);
    }
}
