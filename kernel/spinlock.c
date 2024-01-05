#include <unios/proto.h>
#include <unios/spinlock.h>
#include <stdlib.h>
#include <atomic.h>

void initlock(spinlock_t *lock, char *name) {
    lock->name   = name;
    lock->locked = 0;
    lock->cpu    = 0xffffffff;
}

bool try_lock(void *lock) {
    return (compare_exchange_strong((int *)lock, 0, 1) == 0);
}

void acquire(void *lock) {
    while (compare_exchange_strong((int *)lock, 0, 1) != 0) {}
}

void lock_or_yield(void *lock) {
    while (compare_exchange_strong((int *)lock, 0, 1) != 0) { yield(); }
}

void lock_or_schedule(void *lock) {
    while (compare_exchange_strong((int *)lock, 0, 1) != 0) { sched(); }
}

void release(void *lock) {
    *(u32 *)lock = 0;
}
