#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    assert(true && "this is ok.");
    assert(false && "this is bad.");
    printf("expect unreachable!\n");
    unreachable();
}
