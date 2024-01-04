#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    printf("startup test-exec from pid=%d\n", get_pid());
    int resp = exec("test-exec");
    assert(false && "unreachable");
    return 0;
}
