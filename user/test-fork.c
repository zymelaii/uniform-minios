#include <sys/sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

const int screen_lock_uid = 0xcafebabe;

int sync_printf(const char *fmt, ...) {
    va_list ap;
    int     rc;
    va_start(ap, fmt);
    krnlobj_lock(screen_lock_uid);
    rc = vprintf(fmt, ap);
    krnlobj_unlock(screen_lock_uid);
    va_end(ap);
    return rc;
}

#define printf sync_printf

int global = 0;

int main(int arg, char *argv[]) {
    if (krnlobj_lookup(screen_lock_uid) == INVALID_HANDLE) {
        handle_t hd = krnlobj_create(screen_lock_uid);
        assert(hd != INVALID_HANDLE);
    }

    int i = 0;
    int j = 0;
    global++;

    printf("good\n");
    while (i < 3) {
        if (fork() == 0) {
            i++;
            continue;
        }
        break;
    }

    while (1) {
        j++;
        if (j == 1000000) {
            j = 0;
            printf("i am %d", i);
        }
    }

    while (1) {
        printf("init");
        printf("%d", ++global);
        printf(" ");
        i = 1000000;
        while (--i) {}
    }
    return 0;
}
