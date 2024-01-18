#include <stdarg.h>
#include <stddef.h>

#define stdin  0
#define stdout 1
#define stderr 2

#define O_CREAT  1
#define O_RDWR   2
#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3

int open(const char *path, int flags);
int close(int fd);
int read(int fd, char *buf, int count);
int write(int fd, const char *buf, int count);
int lseek(int fd, int offset, int whence);
int unlink(const char *path);
int create(const char *path);
int delete(const char *path);
int opendir(const char *path);
int createdir(const char *path);
int deletedir(const char *path);

long strtol(const char *cp, char **endp, unsigned int base);

#define EOF -1

void printfmt(void (*putch)(int, void *), void *putdat, const char *fmt, ...);
void vprintfmt(
    void (*putch)(int, void *), void *putdat, const char *fmt, va_list ap);
int vsnprintf(char *buf, int n, const char *fmt, va_list ap);
int snprintf(char *buf, int n, const char *fmt, ...);

int vprintf(const char *fmt, va_list ap);
int printf(const char *fmt, ...);

int vkprintf(const char *fmt, va_list ap);
int kprintf(const char *fmt, ...);
int uart_kprintf(const char *fmt, ...);
int klog(const char *fmt, ...);

char  getchar();
char *gets(char *str);
