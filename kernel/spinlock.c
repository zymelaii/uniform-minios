/**********************************************************
*	spinlock.c       //added by mingxuan 2018-12-26
***********************************************************/
// Mutual exclusion spin locks.

//#include "types.h"
//#include "defs.h"
//#include "x86.h"
//#include "mmu.h"
//#include "param.h"
//#include "proc.h"
#include "spinlock.h"

//extern int use_console_lock;

static inline uint
cmpxchg(uint oldval, uint newval, volatile uint* lock_addr)
{
  uint result;
  asm volatile("lock; cmpxchg %0, %2" :
               "+m" (*lock_addr), "=a" (result) :
               "r"(newval), "1"(oldval) :
               "cc");
  return result;
}

void
initlock(struct spinlock *lock, char *name)
{
  lock->name = name;
  lock->locked = 0;
  lock->cpu = 0xffffffff;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// (Because contention is handled by spinning, must not
// go to sleep holding any locks.)
void
acquire(struct spinlock *lock)
{

  while(cmpxchg(0, 1, &lock->locked) == 1)
    ;
}

// Release the lock.
void
release(struct spinlock *lock)
{

  lock->locked = 0;
}



