#include <proto.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("startup dead-exec\n");
    exec("/orange/dead-exec.bin");
    return 0;
}
