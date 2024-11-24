#pragma once

#include <stdbool.h>

enum {
    NR_get_ticks = 0,
    NR_get_pid,
    NR_get_ppid,
    NR_fork,
    NR_execve,
    NR_yield,
    NR_sleep,
    NR_malloc,
    NR_free,
    NR_open,
    NR_close,
    NR_read,
    NR_write,
    NR_lseek,
    NR_unlink,
    NR_create,
    NR_delete,
    NR_opendir,
    NR_createdir,
    NR_deletedir,
    NR_wait,
    NR_killerabbit,
    NR_environ,
    NR_krnlobj_request,
    NR_exit,

    //! total syscalls
    NR_SYSCALLS = NR_exit + 1,
};

//! from exec.c
int do_execve(const char *path, char *const *argv, char *const *envp);

//! from clock.c
int do_get_ticks();

//! from fork.c
int do_fork();

//! from exit.c
void do_exit(int exit_code);

//! from wait.c
int do_wait(int *wstatus);

//! from malloc.c
void *do_malloc(int size);
void  do_free(void *ptr);

//! from proc.c
int  do_get_pid();
int  do_get_ppid();
void do_yield();
void do_sleep(int n);

//! from vfs.c
int do_open(const char *path, int flags);
int do_close(int fd);
int do_read(int fd, void *buf, int count);
int do_write(int fd, const void *buf, int count);
int do_lseek(int fd, int offset, int whence);
int do_unlink(const char *path);
int do_create(const char *path);
int do_delete(const char *path);
int do_opendir(const char *path);
int do_createdir(const char *path);
int do_deletedir(const char *path);

//! from killerabbit.c
int do_killerabbit(int pid);

//! from environ.c
bool do_environ(int op, char *const **p_envp);

//! from sync.c
int do_krnlobj_request(int req, void *arg);
