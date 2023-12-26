#include <string.h>

int strlen(const char *s) {
    int n;

    for (n = 0; *s != '\0'; s++) n++;
    return n;
}

int strnlen(const char *s, size_t size) {
    int n;

    for (n = 0; size > 0 && *s != '\0'; s++, size--) n++;
    return n;
}

char *strcpy(char *dst, const char *src) {
    char *ret;

    ret = dst;
    while ((*dst++ = *src++) != '\0') /* do nothing */
        ;
    return ret;
}

char *strcat(char *dst, const char *src) {
    int len = strlen(dst);
    strcpy(dst + len, src);
    return dst;
}

char *strncpy(char *dst, const char *src, size_t size) {
    size_t i;
    char  *ret;

    ret = dst;
    for (i = 0; i < size; i++) {
        *dst++ = *src;
        // If strlen(src) < size, null-pad 'dst' out to 'size' chars
        if (*src != '\0') src++;
    }
    return ret;
}

int strcmp(const char *p, const char *q) {
    while (*p && *p == *q) p++, q++;
    return (int)((unsigned char)*p - (unsigned char)*q);
}

int strncmp(const char *p, const char *q, size_t n) {
    while (n > 0 && *p && *p == *q) n--, p++, q++;
    if (n == 0)
        return 0;
    else
        return (int)((unsigned char)*p - (unsigned char)*q);
}

void *memset(void *v, int c, size_t n) {
    char *p;
    int   m;

    p = v;
    m = n;
    while (--m >= 0) *p++ = c;

    return v;
}

void *memcpy(void *dst, const void *src, size_t n) {
    const char *s;
    char       *d;

    s = src;
    d = dst;

    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0) *--d = *--s;
    } else {
        while (n-- > 0) *d++ = *s++;
    }

    return dst;
}

int memcmp(const void *s1, const void *s2, int n) {
    if ((s1 == 0) || (s2 == 0)) { return (s1 - s2); }

    const char *p1 = (const char *)s1;
    const char *p2 = (const char *)s2;
    int         i;
    for (i = 0; i < n; i++, p1++, p2++) {
        if (*p1 != *p2) { return (*p1 - *p2); }
    }
    return 0;
}
