#include <unios/syscall.h>
#include <stdint.h>

static int syscall0(u32 NR_syscall) {
    int ret = 0;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(NR_syscall)
                     : "cc", "memory");
    return ret;
}

static int syscall1(u32 NR_syscall, u32 p1) {
    int ret = 0;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(NR_syscall), "b"(p1)
                     : "cc", "memory");
    return ret;
}

static int syscall2(u32 NR_syscall, u32 p1, u32 p2) {
    int ret = 0;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(NR_syscall), "b"(p1), "c"(p2)
                     : "cc", "memory");
    return ret;
}

static int syscall3(u32 NR_syscall, u32 p1, u32 p2, u32 p3) {
    int ret = 0;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(NR_syscall), "b"(p1), "c"(p2), "d"(p3)
                     : "cc", "memory");
    return ret;
}

static int syscall4(u32 NR_syscall, u32 p1, u32 p2, u32 p3, u32 p4) {
    int ret = 0;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(NR_syscall), "b"(p1), "c"(p2), "d"(p3), "S"(p4)
                     : "cc", "memory");
    return ret;
}

static int syscall5(u32 NR_syscall, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5) {
    int ret = 0;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(NR_syscall), "b"(p1), "c"(p2), "d"(p3), "S"(p4), "D"(p5)
        : "cc", "memory");
    return ret;
}

int get_ticks() {
    return syscall0(NR_get_ticks);
}

void *malloc(int size) {
    return (void *)syscall1(NR_malloc, size);
}

void *malloc_4k() {
    return (void *)syscall0(NR_malloc_4k);
}

int free(void *ptr) {
    return syscall1(NR_free, (u32)ptr);
}

int pthread(void *arg) {
    return syscall1(NR_pthread, (u32)arg);
}

int execve(const char *path, char *const *argv, char *const *envp) {
    return syscall3(NR_execve, (u32)path, (u32)argv, (u32)envp);
}

int fork() {
    return syscall0(NR_fork);
}

int wait(int *wstatus) {
    return syscall1(NR_wait, (u32)wstatus);
}

void exit(int exit_code) {
    syscall1(NR_exit, exit_code);
}

int get_pid() {
    return syscall0(NR_get_pid);
}

void yield() {
    syscall0(NR_yield);
}

void sleep(int n) {
    syscall1(NR_sleep, n);
}

void wakeup(void *channel) {
    syscall1(NR_wakeup, (u32)channel);
}

int open(const char *path, int flags) {
    return syscall2(NR_open, (u32)path, flags);
}

int close(int fd) {
    return syscall1(NR_close, fd);
}

int read(int fd, char *buf, int count) {
    return syscall3(NR_read, fd, (u32)buf, count);
}

int write(int fd, const char *buf, int count) {
    return syscall3(NR_write, fd, (u32)buf, count);
}

int lseek(int fd, int offset, int whence) {
    return syscall3(NR_lseek, fd, offset, whence);
}

int unlink(const char *path) {
    return syscall1(NR_unlink, (u32)path);
}

int create(const char *path) {
    return syscall1(NR_create, (u32)path);
}

int delete(const char *path) {
    return syscall1(NR_delete, (u32)path);
}

int opendir(const char *path) {
    return syscall1(NR_opendir, (u32)path);
}

int createdir(const char *path) {
    return syscall1(NR_createdir, (u32)path);
}

int deletedir(const char *path) {
    return syscall1(NR_deletedir, (u32)path);
}
