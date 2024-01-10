#include <unios/sync.h>
#include <unios/assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <atomic.h>
#include <stddef.h>

void init_spinlock(spinlock_t *lock, char *name) {
    assert(lock != NULL);
    lock->name   = name;
    lock->locked = 0;
    lock->cpu    = 0xffffffff;
}

void init_rwlock(rwlock_t *lock) {
    assert(lock != NULL);
    *(int *)lock = 0;
}

void rwlock_wait_rd(rwlock_t *lock) {
    assert(lock != NULL);
    while (!rwlock_try_lock_rd(lock)) {}
}

void rwlock_wait_wr(rwlock_t *lock) {
    assert(lock != NULL);
    while (!rwlock_try_lock_wr(lock)) {}
}

void rwlock_wait_rd_or(rwlock_t *lock, void (*callback)()) {
    assert(lock != NULL);
    while (!rwlock_try_lock_rd(lock)) {
        if (callback != NULL) { callback(); }
    }
}

void rwlock_wait_wr_or(rwlock_t *lock, void (*callback)()) {
    assert(lock != NULL);
    while (!rwlock_try_lock_wr(lock)) {
        if (callback != NULL) { callback(); }
    }
}

bool rwlock_try_lock_wr(rwlock_t *lock) {
    assert(lock != NULL);
    return compare_exchange_strong(lock, 0, -1) == 0;
}

bool rwlock_try_lock_rd(rwlock_t *lock) {
    assert(lock != NULL);
    int old = compare_exchange_strong(lock, 0, 1);
    if (old == 0) { return true; }
    if (old < 0) { return false; }
    return compare_exchange_strong(lock, old, old + 1) == old;
}

void rwlock_leave(rwlock_t *lock) {
    assert(lock != NULL);
    if (compare_exchange_strong(lock, 0, 0) <= 0) {
        //! hold wr lock
        int old = compare_exchange_strong(lock, -1, 0);
        assert(old == -1);
    } else {
        //! hold rd lock
        fetch_sub(lock, 1);
    }
}
