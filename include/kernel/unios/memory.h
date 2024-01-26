#pragma once

#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>

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

enum memory_bound_type {
    KernelSpace,
    KernelMemory,
    KernelPage,
    UserPage,
};

void mballoc_reset_unsafe(memblk_allocator_t *allocator);

void *mballoc_alloc(memblk_allocator_t *allocator, size_t size);
int   mballoc_free(memblk_allocator_t *allocator, void *addr, size_t size);
memblk_allocator_t *mballoc_create(
    fn_malloc_t malloc, size_t total_free_slots, void *base, void *limit);
memblk_allocator_t *mballoc_create_by(
    memblk_allocator_t *allocator,
    size_t              total_free_slots,
    void               *base,
    void               *limit);

void     *kmalloc(size_t size);
void      kfree(void *ptr);
phyaddr_t kmalloc_phypage();
phyaddr_t malloc_phypage();
void      free_phypage(phyaddr_t phyaddr);

bool get_phymem_bound(int type, phyaddr_t *base, phyaddr_t *limit);
void init_memory();
