/**********************************************************
*	spinlock.h       //added by mingxuan 2018-12-26
***********************************************************/

// Mutual exclusion lock.
#define uint unsigned
struct spinlock {
  uint locked;   // Is the lock held?
  
  // For debugging:
  char *name;    // Name of lock.
  int  cpu;      // The number of the cpu holding the lock.
  uint pcs[10];  // The call stack (an array of program counters)
                 // that locked the lock.
};

void initlock(struct spinlock *lock, char *name);
// Acquire the lock.
// Loops (spins) until the lock is acquired.
// (Because contention is handled by spinning, must not
// go to sleep holding any locks.)
void acquire(struct spinlock *lock);
// Release the lock.
void release(struct spinlock *lock);





