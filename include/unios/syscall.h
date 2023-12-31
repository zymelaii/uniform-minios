#pragma once

enum {
    NR_get_ticks = 0,
    NR_get_pid,
    NR_malloc,
    NR_malloc_4k,
    NR_free,
    NR_fork,
    NR_pthread,
    NR_exec,
    NR_yield,
    NR_sleep,
    NR_wakeup,
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
    NR_exit,

    //! total syscalls
    NR_SYSCALLS = NR_exit + 1,
};

void syscall_handler();

int   do_get_ticks();                               //<! from clock.c
void *do_kmalloc(int size);                         //<! from malloc.c -->
void *do_kmalloc_4k();                              //<!
void *do_malloc(int size);                          //<!
void *do_malloc_4k();                               //<!
int   do_free(void *ptr);                           //<! <-- END
int   do_pthread(void *arg);                        //<! from pthread.c
int   do_exec(char *path);                          //<! from exec.c
int   do_fork();                                    //<! from fork.c
void  do_exit(int exit_code);                       //<! from exit.c
int   do_get_pid();                                 //<! from proc.c -->
void  do_yield();                                   //<!
void  do_sleep(int n);                              //<!
void  do_wakeup(void *channel);                     //<! <-- END
int   do_open(const char *path, int flags);         //<! from vfs.c -->
int   do_close(int fd);                             //<!
int   do_read(int fd, char *buf, int count);        //<!
int   do_write(int fd, const char *buf, int count); //<!
int   do_lseek(int fd, int offset, int whence);     //<!
int   do_unlink(const char *path);                  //<!
int   do_create(const char *path);                  //<!
int   do_delete(const char *path);                  //<!
int   do_opendir(const char *path);                 //<!
int   do_createdir(const char *path);               //<!
int   do_deletedir(const char *path);               //<! <-- END
