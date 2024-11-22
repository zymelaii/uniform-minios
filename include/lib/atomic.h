#pragma once

#include <stdbool.h>

int exchange(int *ptr, int value);

int compare_exchange_strong(int *value, int expected, int desired);
int compare_exchange_weak(int *value, int expected, int desired);

//! NOTE: void* is far more compatible than int*, and the caller only needs to
//! ensure that the data bit length at the pointer is at least 32, i.e. int size

void fetch_add(void *atomic, int value);
void fetch_sub(void *atomic, int value);

void acquire(void *lock);
void release(void *lock);
bool try_lock(void *lock);
void lock_or(void *lock, void (*callback)());
