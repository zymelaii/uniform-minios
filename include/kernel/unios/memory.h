#pragma once

#include <unios/layout.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>

//! WARNING: phy mem must be larger then 32 MB!
//! TODO: replace hard-coded mem assignment with dynamic allocation

//! kernel, normal use ~ 2 MB / laddr ~ phy [4, 6) MB
#define KMEM_BASE  K_PHY2LIN(NUM_4M)
#define KMEM_LIMIT (KMEM_BASE + 2 * NUM_1M)

//! kernel, page alloc ~ 2 MB / phyaddr ~ phy [6, 8) MB
#define KPAGE_BASE  ((void *)K_LIN2PHY(KMEM_LIMIT))
#define KPAGE_LIMIT (KPAGE_BASE + 2 * NUM_1M)

//! user, page alloc ~ 24 MB / phyaddr ~ phy [8, 32) MB
//! NOTE: scheduled by upage_allocator, further allocation strategy of normal
//! use for user space should be decided by user-defined memory allocator
#define UPAGE_BASE  KPAGE_LIMIT
#define UPAGE_LIMIT (UPAGE_BASE + 6 * NUM_4M)

typedef void *(*fn_malloc_t)(size_t);

typedef struct memblk_s {
    void  *addr;
    size_t size;
} memblk_t;

typedef struct memblk_allocator_s {
    void    *memblk_base;
    void    *memblk_limit;
    size_t   total_free_slots;
    size_t   nr_frees;
    memblk_t free_slots[0];
} memblk_allocator_t;

enum mballoc_free_state {
    MBALLOC_OK,
    MBALLOC_NOSLOTS,
    MBALLOC_INVALID,
};

void  mballoc_reset_unsafe(memblk_allocator_t *allocator);
void *mballoc_alloc(memblk_allocator_t *allocator, size_t size);
int   mballoc_free(memblk_allocator_t *allocator, void *addr, size_t size);
memblk_allocator_t *mballoc_create(
    fn_malloc_t malloc, size_t total_free_slots, void *base, void *limit);
memblk_allocator_t *mballoc_create_by(
    memblk_allocator_t *allocator,
    size_t              total_free_slots,
    void               *base,
    void               *limit);

void init_memory();

void     *kmalloc(size_t size);
void      kfree(void *ptr);
phyaddr_t kmalloc_phypage();
phyaddr_t malloc_phypage();
void      free_phypage(phyaddr_t phyaddr);
