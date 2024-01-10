#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <atomic.h>

typedef struct spinlock_s {
    //! NOTE: keep &locked the same with &spinlock_t to make spinlock method
    //! compatible
    u32 locked;

    //! for debug use
    char *name;
    int   cpu;     //<! number of the cpu holding the lock
    u32   pcs[10]; //<! call stack that locked the lock
} spinlock_t;

typedef i32 rwlock_t;

enum rwlock_request {
    RWLOCK_RD,
    RWLOCK_WR,
};

void init_spinlock(spinlock_t *lock, char *name);

void init_rwlock(rwlock_t *lock);
void rwlock_wait_rd(rwlock_t *lock);
void rwlock_wait_wr(rwlock_t *lock);
void rwlock_wait_rd_or(rwlock_t *lock, void (*callback)());
void rwlock_wait_wr_or(rwlock_t *lock, void (*callback)());
bool rwlock_try_lock_wr(rwlock_t *lock);
bool rwlock_try_lock_rd(rwlock_t *lock);
void rwlock_leave(rwlock_t *lock);
