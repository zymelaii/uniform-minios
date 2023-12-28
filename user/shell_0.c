#include <type.h>
#include <const.h>
#include <protect.h>
#include <string.h>
#include <proc.h>
#include <global.h>
#include <proto.h>
#include <stdio.h>

#define assert(expected)                                    \
 do {                                                       \
  if (expected) { break; }                                  \
  const char *file = __FILE__;                              \
  int         line = __LINE__;                              \
  const char *expr = #expected;                             \
  printf("assertion failed:%s:%d: %s\n", file, line, expr); \
  __asm__ volatile("hlt");                                  \
 } while (0);

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

int main(int arg, char *argv[]) {
    setup_for_all_tty();

    char buf[MAX_PATH] = {};
    while (1) {
        printf("miniOS:/ $ ");
        gets(buf);
        if (strlen(buf) == 0) { continue; }
        if (exec(buf) != 0) {
            printf("exec failed: file not found!\n");
            continue;
        }
    }
}
