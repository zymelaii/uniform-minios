#pragma once

#include <stdbool.h>

int compare_exchange_strong(int *value, int expected, int desired);
int compare_exchange_weak(int *value, int expected, int desired);

//! NOTE: void* is a much more compatible type than int*

bool try_lock(void *lock);
void acquire(void *lock);
void lock_or_yield(void *lock);
void release(void *lock);
