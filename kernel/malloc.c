#include <unios/malloc.h>
#include <string.h>
#include <const.h>
#include <stdio.h>
#include <assert.h>
#include <spinlock.h>

//! memory bounds
#define MEMSTART 0x00400000
#define KWALL    0x00600000
#define WALL     0x00800000
#define UWALL    0x01000000
#define MEMEND   0x02000000

memman_t memman;

static bool memm_is_alloc4k_memblk(u32 addr) {
    bool is_alloc_4k_mem  = addr >= UWALL && addr < MEMEND;
    bool is_kalloc_4k_mem = addr >= MEMSTART && addr < KWALL;
    return is_alloc_4k_mem || is_kalloc_4k_mem;
}

static bool memm_is_free_in_order(memman_t *man) {
    assert(man != NULL);
    for (int i = 1; i < man->frees; ++i) {
        if (man->free[i - 1].addr + man->free[i - 1].size > man->free[i].addr) {
            return false;
        }
    }
    return true;
}

static u32
    memm_range_based_alloc(memman_t *man, u32 size, u32 base, u32 limit) {
    u32 addr = 0;

    //! NOTE: reserve u32 to save size of non-4k memblk
    bool has_size_info = !memm_is_alloc4k_memblk(base);
    u32  real_size     = has_size_info ? size + 4 : size;

    for (int i = 0; i < man->frees; i++) {
        if (!(man->free[i].addr >= base && man->free[i].addr < limit)) {
            continue;
        }
        if (man->free[i].size < real_size) { continue; }
        addr              = man->free[i].addr;
        man->free[i].addr += real_size;
        man->free[i].size -= real_size;
        if (man->free[i].size == 0) {
            --man->frees;
            while (i < man->frees) {
                man->free[i] = man->free[i + 1];
                ++i;
            }
        }
        break;
    }

    if (has_size_info) {
        //! FIXME: awful code, expect better solution
        *(u32 *)K_PHY2LIN(addr) = size;
        addr                    += 4;
    }

    return addr;
}

static void memm_init(memman_t *man) {
    man->frees    = 0;
    man->maxfrees = 0;
    man->lostsize = 0;
    man->losts    = 0;
    return;
}

static u32 memm_sized_free(memman_t *man, u32 addr, u32 size) {
    if (size == 0) { return 0; }

    int index = 0;
    //! NOTE: free addr is sorted in asc
    //! ... | free[index - 1] | memblk | free[index] | ...
    for (index = 0; index < man->frees; ++index) {
        if (man->free[index].addr > addr) { break; }
    }
    assert(index == man->frees || man->free[index].addr >= addr + size);

    int merged = 0;

    mman_free_info_t *before = (mman_free_info_t *)man->free + index - 1;
    mman_free_info_t *after  = (mman_free_info_t *)man->free + index;

    //! merge backward
    if (index > 0) {
        assert(before->addr + before->size <= addr);
        if (before->addr + before->size == addr) {
            before->size += size;
            ++merged;
        }
    }

    //! merge forward
    if (index < man->frees) {
        assert(addr + size <= after->addr);
        if (addr + size == after->addr) {
            after->addr = addr;
            after->size += size;
            ++merged;
        }
    }

    if (merged == 2) {
        before->size += after->addr + after->size - before->addr;
        --man->frees;
        while (index < man->frees) {
            man->free[index] = man->free[index + 1];
            ++index;
        }
    }

    if (merged > 0) { return 0; }

    //! no enough space to insert new free blk, failed to free
    if (man->frees >= MEMMAN_FREES) {
        ++man->losts;
        man->lostsize += size;
        return -1;
    }

    for (int j = man->frees; j > index; --j) {
        man->free[j] = man->free[j - 1];
    }
    ++man->frees;
    man->maxfrees         = max(man->maxfrees, man->frees);
    man->free[index].addr = addr;
    man->free[index].size = size;
    return 0;
}

static u32 memm_free(memman_t *man, u32 addr) {
    if (addr < MEMSTART && addr >= MEMEND) { return 0; }
    u32 size = 0;
    if (memm_is_alloc4k_memblk(addr)) {
        size = 0x1000;
    } else {
        //! FIXME: awful impl, see memm_range_based_alloc also
        addr -= 4;
        size = *(u32 *)K_PHY2LIN(addr);
    }
    return memm_sized_free(man, addr, size);
}

static u32 memm_alloc(memman_t *man, u32 size) {
    return memm_range_based_alloc(man, size, WALL, UWALL);
}

static u32 memm_kalloc(memman_t *man, u32 size) {
    return memm_range_based_alloc(man, size, KWALL, WALL);
}

static u32 memm_alloc_4k(memman_t *man) {
    return memm_range_based_alloc(man, 0x1000, UWALL, MEMEND);
}

static u32 memm_kalloc_4k(memman_t *man) {
    return memm_range_based_alloc(man, 0x1000, MEMSTART, KWALL);
}

void *do_malloc(u32 size) {
    return (void *)memm_alloc(&memman, size);
}

void *do_kmalloc(u32 size) {
    return (void *)memm_kalloc(&memman, size);
}

void *do_malloc_4k() {
    return (void *)memm_alloc_4k(&memman);
}

void *do_kmalloc_4k() {
    return (void *)memm_kalloc_4k(&memman);
}

void do_free(void *addr) {
    memm_free(&memman, (u32)addr);
}

//! TODO: better solution
void init_mem() {
    memm_init(&memman);

    memman.frees        = 4;
    memman.maxfrees     = 4;
    memman.free[0].addr = MEMSTART;
    memman.free[0].size = KWALL - MEMSTART;
    memman.free[1].addr = KWALL;
    memman.free[1].size = WALL - KWALL;
    memman.free[2].addr = WALL;
    memman.free[2].size = UWALL - WALL;
    memman.free[3].addr = UWALL;
    memman.free[3].size = MEMEND - UWALL;

    assert(memm_is_free_in_order(&memman));
}
