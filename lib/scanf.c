#include <stdio.h>
#include <const.h>

char getchar() {
    char ch;
    return read(STD_IN, &ch, 1) == 1 ? ch : EOF;
}

char *gets(char *str) {
    char  c;
    char *cs;
    cs = str;
    while ((c = getchar()) != EOF) {
        if ((*cs = c) == '\n') {
            *cs = '\0';
            break;
        }
        cs++;
    }
    return str;
}
