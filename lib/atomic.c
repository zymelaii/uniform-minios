#include <stdlib.h>
#include <atomic.h>
#include <stdbool.h>

int compare_exchange_strong(int *value, int expected, int desired) {
    //! impl with GCC builtin __atomic_compare_exchange_n
    int tmp = expected;
    __atomic_compare_exchange_n(
        value, &tmp, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return tmp;
}

int compare_exchange_weak(int *value, int expected, int desired) {
    //! impl with GCC builtin __atomic_compare_exchange_n
    int tmp = expected;
    __atomic_compare_exchange_n(
        value, &tmp, desired, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return tmp;
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

void release(void *lock) {
    *(int *)lock = 0;
}
