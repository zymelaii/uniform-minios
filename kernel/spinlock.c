#include <unios/spinlock.h>
#include <unios/schedule.h>
#include <stdlib.h>
#include <atomic.h>

void initlock(spinlock_t *lock, char *name) {
    lock->name   = name;
    lock->locked = 0;
    lock->cpu    = 0xffffffff;
}

void lock_or_schedule(void *lock) {
    while (compare_exchange_strong((int *)lock, 0, 1) != 0) { schedule(); }
}
