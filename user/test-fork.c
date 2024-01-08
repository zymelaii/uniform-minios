#include <stdio.h>
#include <stdlib.h>

int global = 0;

int main(int arg, char *argv[]) {
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
