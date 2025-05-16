#include <unios/memory.h>
#include <unios/assert.h>
#include <unios/page.h>
#include <unios/sync.h>
#include <unios/schedule.h>
#include <unios/tracing.h>
#include <unios/host_device.h>
#include <string.h>
#include <stddef.h>
#include <config.h>
#include <math.h>

//! { kmem, kpage, upage, kernel space } ~ [ base, limit ]
static phyaddr_t memblk_bounds[4][2];

static memblk_allocator_t* kmem_allocator  = NULL;
static memblk_allocator_t* kpage_allocator = NULL;
static memblk_allocator_t* upage_allocator = NULL;

static spinlock_t kmem_lock;

static size_t get_total_memory() {
    size_t              total_memory = 0;
    host_device_info_t* device_info  = (void*)DEVICE_INFO_ADDR;
    //! FIXME: unios not support multi-seg memory currently
    for (int i = 0; i < device_info->ards_count; ++i) {
        ards_t ards = device_info->ards_buffer[i];
        if (ards.type == 1) {
            size_t limit = ards.base_addr_low + ards.length_low;
            total_memory = max(total_memory, limit);
        }
    }
    return total_memory;
}

static size_t get_critical_memsize() {
    //! NOTE: symbol `_end` is provided by GCC and is addressed at the end of
    //! the elf, addr from 0 to 4 KB aligned `_end` is marked critical and
    //! should never be access by anything except the kernel
    extern int end;
    return (size_t)K_LIN2PHY(round_up(&end, NUM_4K));
}

void* unsafe_kmalloc(size_t size) {
    static void* base = 0;
    if (base == 0) { base = K_PHY2LIN(get_critical_memsize()); }
    void* ptr  = base;
    base      += size;
    return ptr;
}

void mballoc_reset_unsafe(memblk_allocator_t* allocator) {
    assert(allocator != NULL);
    assert(allocator->memblk_base < allocator->memblk_limit);
    assert(allocator->total_free_slots > 1);

    size_t total_free_slots = allocator->total_free_slots;
    void*  base             = allocator->memblk_base;
    void*  limit            = allocator->memblk_limit;

    size_t size_memblks = total_free_slots * sizeof(memblk_t);
    memset(allocator->free_slots, 0, size_memblks);

    allocator->nr_frees           = 1;
    allocator->free_slots[0].addr = base;
    allocator->free_slots[0].size = limit - base;
}

void* mballoc_alloc(memblk_allocator_t* allocator, size_t size) {
    if (allocator == NULL) { return NULL; }
    if (size == 0) { return NULL; }

    int index = 0;
    for (; index < allocator->nr_frees; index++) {
        if (allocator->free_slots[index].size >= size) { break; }
    }
    if (index == allocator->nr_frees) { return NULL; }
    void* addr = allocator->free_slots[index].addr;

    allocator->free_slots[index].addr += size;
    allocator->free_slots[index].size -= size;
    if (allocator->free_slots[index].size > 0) { return addr; }

    --allocator->nr_frees;
    for (int i = index; i < allocator->nr_frees; ++i) {
        allocator->free_slots[i] = allocator->free_slots[i + 1];
    }
    return addr;
}

int mballoc_free(memblk_allocator_t* allocator, void* addr, size_t size) {
    if (allocator == NULL) { return MBALLOC_INVALID; }
    if (size == 0) { return MBALLOC_INVALID; }
    if (addr < allocator->memblk_base
        || addr + size >= allocator->memblk_limit) {
        return MBALLOC_INVALID;
    }

    int index = 0;
    for (index = 0; index < allocator->nr_frees; ++index) {
        if (allocator->free_slots[index].addr > addr) { break; }
    }
    if (index != allocator->nr_frees
        && allocator->free_slots[index].addr < addr + size) {
        return MBALLOC_INVALID;
    }

    int merged = 0;

    memblk_t* before = allocator->free_slots + index - 1;
    memblk_t* after  = allocator->free_slots + index;

    //! merge backward
    if (index > 0) {
        assert(before->addr + before->size <= addr);
        if (before->addr + before->size == addr) {
            before->size += size;
            ++merged;
        }
    }

    //! merge forward
    if (index < allocator->nr_frees) {
        assert(addr + size <= after->addr);
        if (addr + size == after->addr) {
            after->addr  = addr;
            after->size += size;
            ++merged;
        }
    }

    if (merged == 2) {
        before->size = after->addr + after->size - before->addr;
        --allocator->nr_frees;
        while (index < allocator->nr_frees) {
            allocator->free_slots[index] = allocator->free_slots[index + 1];
            ++index;
        }
    }

    if (merged > 0) { return MBALLOC_OK; }

    //! no enough space to insert new free blk, failed to free
    if (allocator->nr_frees >= allocator->total_free_slots) {
        //! TODO: resolve memory leak issues
        return MBALLOC_NOSLOTS;
    }

    for (int j = allocator->nr_frees; j > index; --j) {
        allocator->free_slots[j] = allocator->free_slots[j - 1];
    }
    ++allocator->nr_frees;
    allocator->free_slots[index].addr = addr;
    allocator->free_slots[index].size = size;
    return MBALLOC_OK;
}

memblk_allocator_t* mballoc_create(
    fn_malloc_t malloc, size_t total_free_slots, void* base, void* limit) {
    if (total_free_slots < 1) { return NULL; }
    if (base >= limit) { return NULL; }
    size_t size =
        sizeof(memblk_allocator_t) + total_free_slots * sizeof(memblk_t);
    memblk_allocator_t* obj = malloc(size);
    if (obj == NULL) { return NULL; }
    obj->total_free_slots = total_free_slots;
    obj->memblk_base      = base;
    obj->memblk_limit     = limit;
    mballoc_reset_unsafe(obj);
    return obj;
}

memblk_allocator_t* mballoc_create_by(
    memblk_allocator_t* allocator,
    size_t              total_free_slots,
    void*               base,
    void*               limit) {
    if (total_free_slots < 1) { return NULL; }
    if (base >= limit) { return NULL; }
    size_t size =
        sizeof(memblk_allocator_t) + total_free_slots * sizeof(memblk_t);
    memblk_allocator_t* obj = mballoc_alloc(allocator, size);
    if (obj == NULL) { return NULL; }
    obj->total_free_slots = total_free_slots;
    obj->memblk_base      = base;
    obj->memblk_limit     = limit;
    mballoc_reset_unsafe(obj);
    return obj;
}

void* kmalloc(size_t size) {
    size_t real_size = size + sizeof(size_t);
    lock_or(&kmem_lock, sched);
    void* ptr = mballoc_alloc(kmem_allocator, real_size);
    release(&kmem_lock);
    if (ptr == NULL) { return NULL; }
    *(size_t*)ptr = real_size;
    return ptr + sizeof(size_t);
}

void kfree(void* ptr) {
    //! WARNING: the following explicit bound check is necessary, although the
    //! `mballoc_free` also contains this logic, we still need to access memory
    //! at `ptr` to retrive the memblk size, and that is exactly a potential
    //! risky operation since `ptr` may be invalid
    void* real_ptr = ptr - sizeof(size_t);
    assert(
        real_ptr >= kmem_allocator->memblk_base
        && ptr < kmem_allocator->memblk_limit);

    size_t size = *(size_t*)real_ptr;
    lock_or(&kmem_lock, sched);
    int resp = mballoc_free(kmem_allocator, real_ptr, size);
    release(&kmem_lock);

    //! NOTE: we exactly not that care about memory leak :-)
    assert(resp == MBALLOC_OK || resp == MBALLOC_NOSLOTS);

    //! TODO: deal with memory leak
}

phyaddr_t kmalloc_phypage() {
    return (phyaddr_t)mballoc_alloc(kpage_allocator, NUM_4K);
}

phyaddr_t malloc_phypage() {
    return (phyaddr_t)mballoc_alloc(upage_allocator, NUM_4K);
}

void free_phypage(phyaddr_t phyaddr) {
    assert(phyaddr == pg_frame_phyaddr(phyaddr));
    void* addr = (void*)phyaddr;
    if (addr >= kpage_allocator->memblk_base
        && addr + NUM_4K < kpage_allocator->memblk_limit) {
        int resp = mballoc_free(kpage_allocator, addr, NUM_4K);
        assert(resp == MBALLOC_OK);
        return;
    }
    if (addr >= upage_allocator->memblk_base
        && addr + NUM_4K < upage_allocator->memblk_limit) {
        int resp = mballoc_free(upage_allocator, addr, NUM_4K);
        assert(resp == MBALLOC_OK);
        return;
    }
    unreachable();
}

bool get_phymem_bound(int type, phyaddr_t* base, phyaddr_t* limit) {
    int index = -1;
    if (type == KernelMemory) {
        index = 0;
    } else if (type == KernelPage) {
        index = 1;
    } else if (type == UserPage) {
        index = 2;
    } else if (type == KernelSpace) {
        index = 3;
    }
    if (index == -1) { return false; }
    if (base != NULL) { *base = memblk_bounds[index][0]; }
    if (limit != NULL) { *limit = memblk_bounds[index][1]; }
    return true;
}

void init_memory() {
    size_t total_mem = get_total_memory();
    kinfo("total memory: %.3f MB", total_mem * 1.0 / NUM_1M);

    size_t total_crit = get_critical_memsize();
    kdebug("total critical memory: %.3f KB", total_crit * 1.0 / NUM_1K);

    const int    kmem_slots = 1000;
    const size_t kmem_alloc_size =
        sizeof(memblk_allocator_t) + kmem_slots * sizeof(memblk_t);

    //! layout: critical | kmem_allocator | kmem | kpage | upage

    const ssize_t min_kmem_size  = 2 * NUM_1M;
    const ssize_t min_kpage_size = 2 * NUM_1M;
    const ssize_t min_upage_size = 8 * NUM_1M;
    const ssize_t min_free_mem =
        min_kmem_size + min_kpage_size + min_upage_size;

    const ssize_t kmem_offset    = total_crit + kmem_alloc_size;
    const ssize_t total_free_mem = (ssize_t)total_mem - kmem_offset;
    if (total_free_mem < min_free_mem) {
        size_t min_req = idiv_ceil(min_free_mem + kmem_offset, NUM_1M);
        kerror("physical memory too small, expect >= %d MB", min_req);
        kfatal("init memory failed");
    }

    //! current assignment: kmem 0.2, kpage 0.1, upage 0.7
    int    slack     = total_free_mem % NUM_4K;
    size_t kmemsize  = round_down(total_free_mem * 0.2 - slack, NUM_4K) + slack;
    size_t kpagesize = round_up(total_free_mem * 0.1, NUM_4K);
    size_t upagesize = total_free_mem - kmemsize - kpagesize;
    assert(kpagesize == round_down(kpagesize, NUM_4K));
    assert(upagesize == round_down(upagesize, NUM_4K));
    assert(kmemsize + kpagesize + upagesize == total_free_mem);

    memblk_bounds[0][0] = kmem_offset;
    memblk_bounds[0][1] = memblk_bounds[0][0] + kmemsize;
    memblk_bounds[1][0] = memblk_bounds[0][1];
    memblk_bounds[1][1] = memblk_bounds[1][0] + kpagesize;
    memblk_bounds[2][0] = memblk_bounds[1][1];
    memblk_bounds[2][1] = memblk_bounds[2][0] + upagesize;
    memblk_bounds[3][0] = 0;
    memblk_bounds[3][1] = memblk_bounds[1][1];
    kdebug(
        "kmem: address at %#p, %d MB %d KB %d bytes",
        memblk_bounds[0][0],
        kmemsize / NUM_1M,
        kmemsize % NUM_1M / NUM_1K,
        kmemsize % NUM_1K);
    kdebug(
        "kpage: address at %#p, %d pages",
        memblk_bounds[1][0],
        kpagesize / NUM_4K);
    kdebug(
        "upage: address at %#p, %d pages",
        memblk_bounds[2][0],
        upagesize / NUM_4K);

    //! NOTE: separate `kmem_allocator` itself from the kmem segment, so nothing
    //! except a violent mem direct write could destroy kmem_allocator
    kmem_allocator = mballoc_create(
        unsafe_kmalloc,
        kmem_slots,
        K_PHY2LIN(memblk_bounds[0][0]),
        K_PHY2LIN(memblk_bounds[0][1]));

    //! NOTE: ok, now `unsafe_kmalloc` has finished its task and was completely
    //! replaced by `kmem_allocator`, it should never be used since then,
    //! neither should we free the memory allocated by it

    //! NOTE: kpage_allocator & upage_allocator is expected always to allocate
    //! and free page size, so the free slots should never excceed half of total
    //! pages

    kpage_allocator = mballoc_create_by(
        kmem_allocator,
        idiv_ceil(memblk_bounds[1][1] - memblk_bounds[1][0], NUM_4K * 2),
        (void*)memblk_bounds[1][0],
        (void*)memblk_bounds[1][1]);
    assert(kpage_allocator != NULL);

    upage_allocator = mballoc_create_by(
        kmem_allocator,
        idiv_ceil(memblk_bounds[2][1] - memblk_bounds[2][0], NUM_4K * 2),
        (void*)memblk_bounds[2][0],
        (void*)memblk_bounds[2][1]);
    assert(upage_allocator != NULL);
}
