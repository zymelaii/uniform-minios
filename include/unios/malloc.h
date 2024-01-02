#pragma once

#include "../type.h"

typedef struct mman_free_info_s {
    u32 addr;
    u32 size;
} mman_free_info_t;

//! NOTE: make memman_t 32 KB
#define MEMMAN_FREES ((32 * 1024 - 16) / sizeof(mman_free_info_t))

typedef struct memman_s {
    u32              frees;
    u32              maxfrees;
    u32              lostsize;
    u32              losts;
    mman_free_info_t free[MEMMAN_FREES];
} memman_t;

void init_mem();
