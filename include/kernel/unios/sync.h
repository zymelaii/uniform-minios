#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <atomic.h>

typedef int32_t spinlock_t;
typedef int32_t rwlock_t;

enum krnlobj_req_type {
    KRNLOBJ_CREATE,
    KRNLOBJ_LOOKUP,
    KRNLOBJ_DESTROY,
    KRNLOBJ_LOCK,
    KRNLOBJ_UNLOCK,
};

void rwlock_wait_rd(rwlock_t *lock);
void rwlock_wait_wr(rwlock_t *lock);
void rwlock_wait_rd_or(rwlock_t *lock, void (*callback)());
void rwlock_wait_wr_or(rwlock_t *lock, void (*callback)());
bool rwlock_try_lock_wr(rwlock_t *lock);
bool rwlock_try_lock_rd(rwlock_t *lock);
void rwlock_leave(rwlock_t *lock);
