#include <sys/types.h>
#include <sys/sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
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

int main(int argc, char *argv[]) {
    bool sync = true;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[1], "-no-sync") == 0) { sync = false; }
    }

    if (sync) {
        if (krnlobj_lookup(screen_lock_uid) == INVALID_HANDLE) {
            handle_t hd = krnlobj_create(screen_lock_uid);
            assert(hd != INVALID_HANDLE);
        }
    }
    int total_worker = 8;
    for (int i = 1; i < total_worker; ++i) {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0) { break; }
    }
    printf("start exec worker pid=%d\n", get_pid());
    int resp = exec("test-exec");
    unreachable();
}
