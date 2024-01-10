#pragma once

#include <sys/types.h>

#define INVALID_HANDLE ((handle_t)-1)

handle_t krnlobj_create(int user_id);
void     krnlobj_destroy(handle_t handle);
void     krnlobj_lock(int user_id);
void     krnlobj_unlock(int user_id);
