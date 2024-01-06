#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    int total_worker = 8;
    for (int i = 1; i < total_worker; ++i) {
        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0) { break; }
    }
    printf("start exec worker pid=%d\n", get_pid());
    int resp = exec("test-exec");
    assert(false && "unreachable");
    return 0;
}
