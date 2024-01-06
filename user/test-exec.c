#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]) {
    printf(
        "startup test-exec from pid=%d argc=%p argv=%p envp=%p\n",
        get_pid(),
        argc,
        argv,
        envp);
    int resp = exec("test-exec");
    assert(false && "unreachable");
    return 0;
}
