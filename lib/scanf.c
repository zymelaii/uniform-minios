#include <stdio.h>

char getchar() {
    char ch;
    return read(stdin, &ch, 1) == 1 ? ch : EOF;
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
