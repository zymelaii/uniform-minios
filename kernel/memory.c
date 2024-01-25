#include <unios/memory.h>
#include <unios/assert.h>
#include <unios/page.h>
#include <unios/sync.h>
#include <unios/schedule.h>
#include <string.h>

memblk_allocator_t* kmem_allocator  = NULL;
memblk_allocator_t* kpage_allocator = NULL;
memblk_allocator_t* upage_allocator = NULL;

spinlock_t kmem_lock;

void* unsafe_kmalloc(size_t size) {
    static void* base = KMEM_BASE;
    if (KMEM_LIMIT - base < size) { return NULL; }
    void* ptr = base;
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
            after->addr = addr;
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

void init_memory() {
    //! NOTE: symbol `_end` is provided by GCC and is addressed at the end of
    //! the elf, perform bound check to ensure the runtime mem ops do not
    //! destroy the kernel
    extern int end;
    assert((void*)&end <= KMEM_BASE);

    kmem_allocator =
        mballoc_create(unsafe_kmalloc, NUM_1K, KMEM_BASE, KMEM_LIMIT);
    assert(kmem_allocator != NULL);

    //! NOTE: remove `kmem_allocator` itself from the kmem segment, and now
    //! nothing except a violent mem direct write could destroy kmem_allocator
    size_t kmem_alloc_size =
        sizeof(memblk_allocator_t)
        + kmem_allocator->total_free_slots * sizeof(memblk_t);
    kmem_allocator->memblk_base = (void*)kmem_allocator + kmem_alloc_size;
    mballoc_reset_unsafe(kmem_allocator);

    //! NOTE: ok, now `unsafe_kmalloc` has finished its task and was completely
    //! replaced by `kmem_allocator`, it should never be used since then,
    //! neither should we free the memory allocated by it

    //! NOTE: kpage_allocator & upage_allocator is expected always to allocate
    //! and free page size, so the free slots should never excceed half of total
    //! pages

    kpage_allocator = mballoc_create_by(
        kmem_allocator,
        ((KPAGE_LIMIT - KPAGE_BASE + NUM_4K - 1) / NUM_4K + 1) / 2,
        KPAGE_BASE,
        KPAGE_LIMIT);
    assert(kpage_allocator != NULL);

    upage_allocator = mballoc_create_by(
        kmem_allocator,
        ((UPAGE_LIMIT - UPAGE_BASE + NUM_4K - 1) / NUM_4K + 1) / 2,
        UPAGE_BASE,
        UPAGE_LIMIT);
    assert(upage_allocator != NULL);
}

void* kmalloc(size_t size) {
    size_t real_size = size + sizeof(size_t);
    lock_or(&kmem_lock, schedule);
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
    lock_or(&kmem_lock, schedule);
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
