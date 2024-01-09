#include <unios/proc.h>
#include <unios/memory.h>
#include <unios/page.h>
#include <unios/assert.h>
#include <unios/spinlock.h>
#include <stddef.h>
#include <atomic.h>

void *do_malloc(int size) {
    pcb_t *pcb = &p_proc_current->pcb;

    size_t real_size = size + sizeof(size_t);
    lock_or_schedule(&pcb->heap_lock);
    void *ptr = mballoc_alloc(pcb->allocator, real_size);
    release(&pcb->heap_lock);

    if (ptr == NULL) { return NULL; }
    bool need_refresh = !pg_addr_pte_exist(pcb->cr3, (u32)ptr);
    bool ok           = pg_map_laddr_range(
        pcb->cr3,
        (u32)ptr,
        (u32)(ptr + real_size),
        PG_P | PG_U | PG_RWX,
        PG_P | PG_U | PG_RWX);
    assert(ok);
    if (need_refresh) { pg_refresh(); }

    *(size_t *)ptr = real_size;
    pcb->memmap.heap_lin_limit =
        MAX(pcb->memmap.heap_lin_limit, (u32)(ptr + real_size));

    return ptr + sizeof(size_t);
}

void do_free(void *ptr) {
    pcb_t *pcb = &p_proc_current->pcb;

    void *real_ptr = ptr - sizeof(size_t);
    if (!pg_pde_exist(pcb->cr3, (u32)real_ptr)) { return; }
    u32 pde = pg_pde(pcb->cr3, (u32)real_ptr);
    if (!pg_pte_exist(pde, (u32)real_ptr)) { return; }

    size_t size = *(size_t *)real_ptr;
    lock_or_schedule(&pcb->heap_lock);
    int resp = mballoc_free(pcb->allocator, real_ptr, size);
    release(&pcb->heap_lock);
}
