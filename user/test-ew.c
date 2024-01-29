#include <sys/sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

int times = 1;
int i     = 1;

handle_t screenlock = 0;
int      lock_id    = 11451;

#define sync_printf printf

void testfork() {
    int  t  = 0;
    char id = 32;
    while (1) {
        ++t;
        id++;
        if (fork() == 0) {
            while (1) {
                int pid = fork();
                if (pid == 0) {
                    i++;
                    sync_printf("I am %d, I am son\n", get_pid(), i);
                    if (i != 10) { continue; }
                }
                // malloc memory
                char *memblk    = malloc(sizeof(char) * 0x100000);
                memblk[0x0ffff] = get_pid() + '0';
                int c;
                int cpid;
                cpid = wait(&c);

                if (cpid == -1) {
                    sync_printf(
                        "[%c] I'm %d, I have no son.\n",
                        memblk[0x0ffff],
                        get_pid());
                } else {
                    if (id > 126) { id = 32; }
                    sync_printf(
                        "[%c] I'm %d, program %d exit with %d\n",
                        id,
                        get_pid(),
                        pid,
                        c);
                }
                // no release, direct quit (to make sure wait is working)
                exit(114);
            }
        } else {
            wait(NULL);
            sync_printf("-----waited %d times-----\n", ++times * 10);
            i = 1;
            if (t == 49) { return; }
        }
    }
}

void testrecycle() {
    int t = 0;
    while (1) {
        ++t;
        if (fork() == 0) {
            if (fork() == 0) {
                malloc(0x100000);
                // no release, direct quit (to make sure scavenger is working)
                while (1) {}
            } else {
                exit(0); // 父进程先于子进程退出，验证垃圾回收
            }
        } else {
            wait(NULL);
            sleep(3);
            sync_printf("good [%d]\n", t);
        }
        if (t == 10) { break; }
    }
}

int main(int argc, char *argv[]) {
    // handle_t handle = krnlobj_create(lock_id);
    // assert(handle != INVALID_HANDLE);
    //  sync_printf("startup test exit & wait [%d]\n", (int)handle);
    testrecycle();
    testfork();
    // krnlobj_destroy(handle);
}
