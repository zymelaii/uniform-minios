#pragma once

#include <type.h>

struct spinlock {
    u32 locked;

    //! for debug use
    char *name;
    int   cpu;     //<! number of the cpu holding the lock
    u32   pcs[10]; //<! call stack that locked the lock
};

u32  cmpxchg(u32 oldval, u32 newval, volatile u32 *lock_addr);
void initlock(struct spinlock *lock, char *name);
void acquire(struct spinlock *lock);
void release(struct spinlock *lock);
