#include <stdlib.h>

bool isxdigit(int c) {
    return c >= '0' && c <= '9' || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F';
}

bool isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

bool isspace(int c) {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t';
}

bool ispunct(int c) {
    return isprint(c) && !isspace(c) && !isalnum(c);
}

bool isprint(int c) {
    return c >= 0x20 && c <= 0x7e;
}

bool islower(int c) {
    return c >= 'a' && c <= 'z';
}

bool isgraph(int c) {
    return c > 0x20 && c <= 0x7e;
}

bool isdigit(int c) {
    return c >= '0' && c <= '9';
}

bool iscntrl(int c) {
    return c >= 0x00 && c <= 0x1f || c == 0x7f;
}

bool isblank(int c) {
    return c == ' ' || c == '\t';
}

bool isascii(int c) {
    return c >= 0x00 && c <= 0x7f;
}

bool isalpha(int c) {
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}

bool isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int toupper(int c) {
    return islower(c) ? c - 'a' + 'A' : c;
}

int tolower(int c) {
    return isupper(c) ? c + 'a' - 'A' : c;
}
