#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("startup dead-exec\n");
    exec("/orange/dead-exec.bin");
    return 0;
}
