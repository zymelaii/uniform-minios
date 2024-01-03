#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("startup test-exec\n");
    exec("/orange/test-exec.bin");
    return 0;
}
