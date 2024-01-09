#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]) {
    clock_t time = clock();
    printf(
        "[%d.%03d] startup test-exec from pid=%d argc=%p argv=%p envp=%p\n",
        time / 1000,
        time % 1000,
        get_pid(),
        argc,
        argv,
        envp);
    int resp = exec("test-exec");
    unreachable();
}
