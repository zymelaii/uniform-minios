#include <sys/sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "screen_sync.h"

int global = 0;

int main(int arg, char *argv[]) {
    int i = 0;
    int j = 0;
    global++;

    begin_exclusive_screen();
    printf("good\n");
    end_exclusive_screen();

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
            begin_exclusive_screen();
            printf("i am %d", i);
            end_exclusive_screen();
        }
    }

    while (1) {
        begin_exclusive_screen();
        printf("init");
        printf("%d", ++global);
        printf(" ");
        end_exclusive_screen();
        i = 1000000;
        while (--i) {}
    }
    return 0;
}
