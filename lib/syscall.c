#include <unios/syscall.h>
#include <unios/environ.h>
#include <unios/sync.h>
#include <sys/types.h>
#include <compiler.h>
#include <stdint.h>
#include <stddef.h>

static int syscall0(uint32_t NR_syscall) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall)
                 : "cc", "memory");
    return ret;
}

static int syscall1(uint32_t NR_syscall, uint32_t p1) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall), "b"(p1)
                 : "cc", "memory");
    return ret;
}

static int syscall2(uint32_t NR_syscall, uint32_t p1, uint32_t p2) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall), "b"(p1), "c"(p2)
                 : "cc", "memory");
    return ret;
}

static int
    syscall3(uint32_t NR_syscall, uint32_t p1, uint32_t p2, uint32_t p3) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall), "b"(p1), "c"(p2), "d"(p3)
                 : "cc", "memory");
    return ret;
}

static int syscall4(
    uint32_t NR_syscall, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4) {
    int ret = 0;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(NR_syscall), "b"(p1), "c"(p2), "d"(p3), "S"(p4)
                 : "cc", "memory");
    return ret;
}

static int syscall5(
    uint32_t NR_syscall,
    uint32_t p1,
    uint32_t p2,
    uint32_t p3,
    uint32_t p4,
    uint32_t p5) {
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
    return syscall3(NR_execve, (uint32_t)path, (uint32_t)argv, (uint32_t)envp);
}

int fork() {
    return syscall0(NR_fork);
}

int wait(int *wstatus) {
    return syscall1(NR_wait, (uint32_t)wstatus);
}

handle_t krnlobj_lookup(int user_id) {
    return (handle_t)syscall2(
        NR_krnlobj_request, KRNLOBJ_LOOKUP, (uint32_t)user_id);
}

handle_t krnlobj_create(int user_id) {
    return (handle_t)syscall2(
        NR_krnlobj_request, KRNLOBJ_CREATE, (uint32_t)user_id);
}

void krnlobj_destroy(handle_t handle) {
    syscall2(NR_krnlobj_request, KRNLOBJ_DESTROY, (uint32_t)handle);
}

void krnlobj_lock(int user_id) {
    syscall2(NR_krnlobj_request, KRNLOBJ_LOCK, (uint32_t)user_id);
}

void krnlobj_unlock(int user_id) {
    syscall2(NR_krnlobj_request, KRNLOBJ_UNLOCK, (uint32_t)user_id);
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
    syscall1(NR_free, (uint32_t)ptr);
}

int open(const char *path, int flags) {
    return syscall2(NR_open, (uint32_t)path, flags);
}

int close(int fd) {
    return syscall1(NR_close, fd);
}

int read(int fd, void *buf, int count) {
    return syscall3(NR_read, fd, (uint32_t)buf, count);
}

int write(int fd, const void *buf, int count) {
    return syscall3(NR_write, fd, (uint32_t)buf, count);
}

int lseek(int fd, int offset, int whence) {
    return syscall3(NR_lseek, fd, offset, whence);
}

int unlink(const char *path) {
    return syscall1(NR_unlink, (uint32_t)path);
}

int create(const char *path) {
    return syscall1(NR_create, (uint32_t)path);
}

int delete(const char *path) {
    return syscall1(NR_delete, (uint32_t)path);
}

int opendir(const char *path) {
    return syscall1(NR_opendir, (uint32_t)path);
}

int createdir(const char *path) {
    return syscall1(NR_createdir, (uint32_t)path);
}

int deletedir(const char *path) {
    return syscall1(NR_deletedir, (uint32_t)path);
}

bool putenv(char *const *envp) {
    bool ok = syscall2(NR_environ, ENVIRON_PUT, (uint32_t)&envp);
    return ok;
}

char *const *getenv() {
    char **envp = NULL;
    bool   ok   = syscall2(NR_environ, ENVIRON_GET, (uint32_t)&envp);
    return envp;
}
