#pragma once

#include <stdint.h>

typedef struct spinlock_s {
    //! NOTE: keep &locked the same with &spinlock_t to make spinlock method
    //! compatible
    u32 locked;

    //! for debug use
    char *name;
    int   cpu;     //<! number of the cpu holding the lock
    u32   pcs[10]; //<! call stack that locked the lock
} spinlock_t;

void initlock(spinlock_t *lock, char *name);

//! NOTE: use void* instead of spinlock_t*, so the spinlock method is also
//! usable for other compatible types

void lock_or_schedule(void *lock);
