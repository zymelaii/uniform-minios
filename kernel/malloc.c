#include <unios/malloc.h>
#include <string.h>
#include <const.h>
#include <stdio.h>
#include <assert.h>
#include <spinlock.h>

// memman 存档地址
#define MEMMAN_ADDR 0x01ff0000

// loader 中 getFreeMemInfo 返回值存放起始地址
#define FMIBuff 0x007ff000

//! malloc 内存分配划分
#define MEMSTART 0x00400000
#define KWALL    0x00600000
#define WALL     0x00800000
#define UWALL    0x01000000
#define MEMEND   0x02000000

// 存放 FMIBuff 后 1 KB 内容
u32 MemInfo[256] = {0};

memman_t        s_memman;
memman_t       *memman = &s_memman;
struct spinlock memmlock;

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
    acquire(&memmlock);
    u32 addr = -1;

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

    assert(memm_is_free_in_order(man));
    release(&memmlock);
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
    acquire(&memmlock);
    if (size == 0) {
        release(&memmlock);
        return 0;
    }

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

    if (merged > 0) {
        assert(memm_is_free_in_order(man));
        release(&memmlock);
        return 0;
    }

    //! no enough space to insert new free blk, failed to free
    if (man->frees >= MEMMAN_FREES) {
        ++man->losts;
        man->lostsize += size;
        assert(memm_is_free_in_order(man));
        release(&memmlock);
        return -1;
    }

    for (int j = man->frees; j > index; --j) {
        man->free[j] = man->free[j - 1];
    }
    ++man->frees;
    man->maxfrees         = max(man->maxfrees, man->frees);
    man->free[index].addr = addr;
    man->free[index].size = size;
    assert(memm_is_free_in_order(man));
    release(&memmlock);
    return 0;
}

static u32 memm_free(memman_t *man, u32 addr) {
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
    return (void *)memm_alloc(memman, size);
}

void *do_kmalloc(u32 size) {
    return (void *)memm_kalloc(memman, size);
}

void *do_malloc_4k() {
    return (void *)memm_alloc_4k(memman);
}

void *do_kmalloc_4k() {
    return (void *)memm_kalloc_4k(memman);
}

int do_free(void *addr) {
    return memm_free(memman, (u32)addr);
}

//! FIXME: awful impl
void init_mem() {
    initlock(&memmlock, "memmlock");

    memm_init(memman);

    memman->frees        = 4;
    memman->maxfrees     = 4;
    memman->free[0].addr = MEMSTART;
    memman->free[0].size = KWALL - MEMSTART;
    memman->free[1].addr = KWALL;
    memman->free[1].size = WALL - KWALL;
    memman->free[2].addr = WALL;
    memman->free[2].size = UWALL - WALL;
    memman->free[3].addr = UWALL;
    memman->free[3].size = MEMEND - UWALL;

    assert(memm_is_free_in_order(memman));

    return;

    // 4M 开始初始化
    u32 memstart = MEMSTART;

    memcpy(MemInfo, (void *)K_PHY2LIN(FMIBuff), 1024);

    for (int i = 1; i <= MemInfo[0]; i++) {
        // 4M 之后开始 free
        if (MemInfo[i] < memstart) { continue; }
        // free 每一段可用内存
        memm_sized_free(memman, memstart, MemInfo[i] - memstart);
        // memtest_sub(start, end) 中每 4KB 检测一次
        memstart = MemInfo[i] + 0x1000;
    }

    for (int i = 0; i < memman->frees; i++) {
        // 6M 处分开，4～6M 为 kmalloc_4k 使用，6～8M 为 kmalloc 使用
        if ((memman->free[i].addr <= KWALL)
            && (memman->free[i].addr + memman->free[i].size > KWALL)) {
            if (memman->free[i].addr == KWALL) { break; }
            // i 之后向后一位
            for (int j = memman->frees; j > i + 1; j--) {
                memman->free[j] = memman->free[j - 1];
            }
            memman->frees++;
            if (memman->maxfrees < memman->frees) { // 更新 man->maxfrees
                memman->maxfrees = memman->frees;
            }
            memman->free[i + 1].addr = KWALL;
            memman->free[i + 1].size =
                memman->free[i].addr + memman->free[i].size - KWALL;
            memman->free[i].size = KWALL - 0x1000 - memman->free[i].addr;
            break;
        }
    }

    for (int i = 0; i < memman->frees; i++) {
        // 8M 处分开，4～8M 为 kmalloc 使用，8～32M 为 malloc 使用
        if ((memman->free[i].addr <= WALL)
            && (memman->free[i].addr + memman->free[i].size > WALL)) {
            if (memman->free[i].addr == WALL) { break; }
            // i 之后向后一位
            for (int j = memman->frees; j > i + 1; j--) {
                memman->free[j] = memman->free[j - 1];
            }
            memman->frees++;
            if (memman->maxfrees < memman->frees) {
                memman->maxfrees = memman->frees;
            }
            memman->free[i + 1].addr = WALL;
            memman->free[i + 1].size =
                memman->free[i].addr + memman->free[i].size - WALL;
            memman->free[i].size = WALL - 0x1000 - memman->free[i].addr;
            break;
        }
    }

    for (int i = 0; i < memman->frees; i++) {
        // 16M处分开，8～16M为malloc使用，16～32M为malloc_4k使用
        if ((memman->free[i].addr <= UWALL)
            && (memman->free[i].addr + memman->free[i].size > UWALL)) {
            if (memman->free[i].addr == UWALL) { break; }
            for (int j = memman->frees; j > i + 1; j--) { // i之后向后一位
                memman->free[j] = memman->free[j - 1];
            }
            memman->frees++;
            if (memman->maxfrees < memman->frees) { // 更新man->maxfrees
                memman->maxfrees = memman->frees;
            }
            memman->free[i + 1].addr = UWALL;
            memman->free[i + 1].size =
                memman->free[i].addr + memman->free[i].size - UWALL;
            memman->free[i].size = UWALL - 0x1000 - memman->free[i].addr;
            break;
        }
    }
}
