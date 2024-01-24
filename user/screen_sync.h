#pragma once

#include <sys/sync.h>
#include <assert.h>

const int screen_lock_uid = 0xcafebabe;

#define init_exclusive_screen()                            \
 do {                                                      \
  if (krnlobj_lookup(screen_lock_uid) == INVALID_HANDLE) { \
   handle_t hd = krnlobj_create(screen_lock_uid);          \
   assert(hd != INVALID_HANDLE);                           \
  }                                                        \
 } while (0)

#define begin_exclusive_screen()                           \
 do {                                                      \
  if (krnlobj_lookup(screen_lock_uid) != INVALID_HANDLE) { \
   krnlobj_lock(screen_lock_uid);                          \
  }                                                        \
 } while (0)

#define end_exclusive_screen()                             \
 do {                                                      \
  if (krnlobj_lookup(screen_lock_uid) != INVALID_HANDLE) { \
   krnlobj_unlock(screen_lock_uid);                        \
  }                                                        \
 } while (0)
