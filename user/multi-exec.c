#include <sys/types.h>
#include <sys/sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "screen_sync.h"

int main(int argc, char *argv[]) {
    bool sync = true;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[1], "-no-sync") == 0) { sync = false; }
    }

    if (sync) { init_exclusive_screen(); }

    int total_worker = 8;
    for (int i = 1; i < total_worker; ++i) {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0) { break; }
    }

    begin_exclusive_screen();
    printf("start exec worker pid=%d\n", get_pid());
    end_exclusive_screen();

    int resp = exec("test-exec");
    return resp;
}
