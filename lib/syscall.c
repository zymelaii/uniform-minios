#include <unios/syscall.h>
#include <unios/environ.h>
#include <unios/sync.h>
#include <sys/types.h>
#include <compiler.h>
#include <stdint.h>
#include <stddef.h>

static int syscall0(u32 NR_syscall) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall)
                 : "cc", "memory");
    return ret;
}

static int syscall1(u32 NR_syscall, u32 p1) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall), "b"(p1)
                 : "cc", "memory");
    return ret;
}

static int syscall2(u32 NR_syscall, u32 p1, u32 p2) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall), "b"(p1), "c"(p2)
                 : "cc", "memory");
    return ret;
}

static int syscall3(u32 NR_syscall, u32 p1, u32 p2, u32 p3) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall), "b"(p1), "c"(p2), "d"(p3)
                 : "cc", "memory");
    return ret;
}

static int syscall4(u32 NR_syscall, u32 p1, u32 p2, u32 p3, u32 p4) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall), "b"(p1), "c"(p2), "d"(p3), "S"(p4)
                 : "cc", "memory");
    return ret;
}

static int syscall5(u32 NR_syscall, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall), "b"(p1), "c"(p2), "d"(p3), "S"(p4), "D"(p5)
                 : "cc", "memory");
    return ret;
}

int get_ticks() {
    return syscall0(NR_get_ticks);
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

handle_t krnlobj_lookup(int user_id) {
    return (handle_t)syscall2(NR_krnlobj_request, KRNLOBJ_LOOKUP, (u32)user_id);
}

handle_t krnlobj_create(int user_id) {
    return (handle_t)syscall2(NR_krnlobj_request, KRNLOBJ_CREATE, (u32)user_id);
}

void krnlobj_destroy(handle_t handle) {
    syscall2(NR_krnlobj_request, KRNLOBJ_DESTROY, (u32)handle);
}

void krnlobj_lock(int user_id) {
    syscall2(NR_krnlobj_request, KRNLOBJ_LOCK, (u32)user_id);
}

void krnlobj_unlock(int user_id) {
    syscall2(NR_krnlobj_request, KRNLOBJ_UNLOCK, (u32)user_id);
}

void exit(int exit_code) {
    syscall1(NR_exit, exit_code);
}

int killerabbit(int pid) {
    return syscall1(NR_killerabbit, pid);
}

int get_pid() {
    return syscall0(NR_get_pid);
}

int get_ppid() {
    return syscall0(NR_get_ppid);
}

void yield() {
    syscall0(NR_yield);
}

void sleep(int n) {
    syscall1(NR_sleep, n);
}

void *malloc(int size) {
    return size <= 0 ? NULL : (void *)syscall1(NR_malloc, size);
}

void free(void *ptr) {
    syscall1(NR_free, (u32)ptr);
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

bool putenv(char *const *envp) {
    bool ok = syscall2(NR_environ, ENVIRON_PUT, (u32)&envp);
    return ok;
}

char *const *getenv() {
    char **envp = NULL;
    bool   ok   = syscall2(NR_environ, ENVIRON_GET, (u32)&envp);
    return envp;
}
