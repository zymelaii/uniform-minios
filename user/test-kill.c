#include <stdio.h>
#include <stdlib.h>

void killall() {
    //! FIXME: not a killall method
    for (int i = 20; i >= 7; --i) {
        int state = killerabbit(i);
        if (state == 1) {
            printf("   successful killed![%d]\n", i);
        } else {
            printf("   skipped![%d]\n", i);
        }
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    printf("startup test kill\n");
    killall();
    exit(0);
}
