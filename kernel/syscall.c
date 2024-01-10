#include <unios/syscall.h>
#include <unios/assert.h>
#include <unios/proc.h>
#include <sys/types.h>
#include <arch/x86.h>
#include <stdint.h>

#define SYSCALL_ENTRY(name) [NR_##name] = sys_##name

#define SYSCALL_ARGS1(t1) (t1) get_syscall_argument(0)
#define SYSCALL_ARGS2(t1, t2) \
 (t1) get_syscall_argument(0), (t2)get_syscall_argument(1)
#define SYSCALL_ARGS3(t1, t2, t3)                           \
 (t1) get_syscall_argument(0), (t2)get_syscall_argument(1), \
     (t3)get_syscall_argument(2)
#define SYSCALL_ARGS4(t1, t2, t3, t4)                       \
 (t1) get_syscall_argument(0), (t2)get_syscall_argument(1), \
     (t3)get_syscall_argument(2), (t4)get_syscall_argument(3)
#define SYSCALL_ARGS5(t1, t2, t3, t4, t5)                      \
 (t1) get_syscall_argument(0), (t2)get_syscall_argument(1),    \
     (t3)get_syscall_argument(2), (t4)get_syscall_argument(3), \
     (t5)get_syscall_argument(4)
#define SYSCALL_ARGS6(t1, t2, t3, t4, t5, t6)                  \
 (t1) get_syscall_argument(0), (t2)get_syscall_argument(1),    \
     (t3)get_syscall_argument(2), (t4)get_syscall_argument(3), \
     (t5)get_syscall_argument(4), (t6)get_syscall_argument(5)

static u32 get_syscall_argument(int index) {
    //! FIXME: p_proc_current may not from the caller proc?
    u32 *frame = (u32 *)p_proc_current->pcb.esp_save_syscall;
    switch (index) {
        case 0: {
            return frame[NR_EBXREG];
        } break;
        case 1: {
            return frame[NR_ECXREG];
        } break;
        case 2: {
            return frame[NR_EDXREG];
        } break;
        case 3: {
            return frame[NR_ESIREG];
        } break;
        case 4: {
            return frame[NR_EDIREG];
        } break;
        case 5: {
            return frame[NR_EBPREG];
        } break;
    }
    panic("syscall argument out of range");
}

static u32 sys_get_ticks() {
    return do_get_ticks();
}

static u32 sys_pthread_create() {
    return do_pthread_create(SYSCALL_ARGS1(void *));
}

static u32 sys_execve() {
    return do_execve(SYSCALL_ARGS3(const char *, char *const *, char *const *));
}

static u32 sys_fork() {
    return do_fork();
}

static u32 sys_exit() {
    do_exit(SYSCALL_ARGS1(int));
    return 0;
}

static u32 sys_killerabbit() {
    return do_killerabbit(SYSCALL_ARGS1(int));
}

static u32 sys_wait() {
    return do_wait(SYSCALL_ARGS1(int *));
}

static u32 sys_get_pid() {
    return do_get_pid();
}

static u32 sys_yield() {
    do_yield();
    return 0;
}

static u32 sys_sleep() {
    do_sleep(SYSCALL_ARGS1(int));
    return 0;
}

static u32 sys_wakeup() {
    do_wakeup(SYSCALL_ARGS1(void *));
    return 0;
}

static u32 sys_malloc(int size) {
    return (u32)do_malloc(SYSCALL_ARGS1(int));
}

static u32 sys_free(void *ptr) {
    do_free(SYSCALL_ARGS1(void *));
    return 0;
}

static u32 sys_open() {
    return do_open(SYSCALL_ARGS2(const char *, int));
}

static u32 sys_close() {
    return do_close(SYSCALL_ARGS1(int));
}

static u32 sys_read() {
    return do_read(SYSCALL_ARGS3(int, char *, int));
}

static u32 sys_write() {
    return do_write(SYSCALL_ARGS3(int, const char *, int));
}

static u32 sys_lseek() {
    return do_lseek(SYSCALL_ARGS3(int, int, int));
}

static u32 sys_unlink() {
    return do_unlink(SYSCALL_ARGS1(const char *));
}

static u32 sys_create() {
    return do_create(SYSCALL_ARGS1(const char *));
}

static u32 sys_delete() {
    return do_delete(SYSCALL_ARGS1(const char *));
}

static u32 sys_opendir() {
    return do_opendir(SYSCALL_ARGS1(const char *));
}

static u32 sys_createdir() {
    return do_createdir(SYSCALL_ARGS1(const char *));
}

static u32 sys_deletedir() {
    return do_deletedir(SYSCALL_ARGS1(const char *));
}

static u32 sys_environ() {
    return do_environ(SYSCALL_ARGS2(int, char *const **));
}

syscall_t syscall_table[NR_SYSCALLS] = {
    SYSCALL_ENTRY(get_ticks),   SYSCALL_ENTRY(get_pid),
    SYSCALL_ENTRY(fork),        SYSCALL_ENTRY(execve),
    SYSCALL_ENTRY(yield),       SYSCALL_ENTRY(sleep),
    SYSCALL_ENTRY(wakeup),      SYSCALL_ENTRY(malloc),
    SYSCALL_ENTRY(free),        SYSCALL_ENTRY(open),
    SYSCALL_ENTRY(close),       SYSCALL_ENTRY(read),
    SYSCALL_ENTRY(write),       SYSCALL_ENTRY(lseek),
    SYSCALL_ENTRY(unlink),      SYSCALL_ENTRY(create),
    SYSCALL_ENTRY(delete),      SYSCALL_ENTRY(opendir),
    SYSCALL_ENTRY(createdir),   SYSCALL_ENTRY(deletedir),
    SYSCALL_ENTRY(wait),        SYSCALL_ENTRY(exit),
    SYSCALL_ENTRY(killerabbit), SYSCALL_ENTRY(pthread_create),
    SYSCALL_ENTRY(environ),
};
