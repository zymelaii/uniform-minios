#include <spinlock.h>

static inline u32 cmpxchg(u32 oldval, u32 newval, volatile u32 *lock_addr) {
    u32 result;
    __asm__ volatile("lock; cmpxchg %0, %2"
                     : "+m"(*lock_addr), "=a"(result)
                     : "r"(newval), "1"(oldval)
                     : "cc");
    return result;
}

void initlock(struct spinlock *lock, char *name) {
    lock->name   = name;
    lock->locked = 0;
    lock->cpu    = 0xffffffff;
}

void acquire(struct spinlock *lock) {
    while (cmpxchg(0, 1, &lock->locked) == 1) {}
}

void release(struct spinlock *lock) {
    lock->locked = 0;
}
