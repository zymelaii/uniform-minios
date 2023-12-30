#include <stdio.h>
#include <stdlib.h>

int global = 0;

char *str2, *str3;

void pthread_test1() {
    int i;
    while (1) {
        printf("pth1");
        printf("%d", ++global);
        printf(" ");
        i = 10000000;
        while (--i) {}
    }
}

int main(int arg, char *argv[]) {
    int i = 0;
    pthread(pthread_test1);
    while (1) {
        printf("init");
        printf("%d", ++global);
        printf(" ");
        i = 10000000;
        while (--i) {}
    }
    return 0;
}
