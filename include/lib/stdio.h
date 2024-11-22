#include <stdarg.h>
#include <stddef.h> // IWYU pragma: keep

#define EOF -1

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
int read(int fd, void *buf, int count);
int write(int fd, const void *buf, int count);
int lseek(int fd, int offset, int whence);
int unlink(const char *path);
int create(const char *path);
int delete (const char *path);
int opendir(const char *path);
int createdir(const char *path);
int deletedir(const char *path);

int snprintf(char *buf, int n, const char *fmt, ...);
int vsnprintf(char *buf, int n, const char *fmt, va_list ap);

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);

char  getchar();
char *gets(char *str);
