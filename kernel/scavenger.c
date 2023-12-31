#include <unios/scavenger.h>
#include <unios/page.h>
#include <unios/assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

void recycle_memory_part(phyaddr_t cr3, void* base, void* limit) {
    bool ok = pg_unmap_laddr_range(cr3, (u32)base, (u32)limit, true);
    assert(ok);
}

void recycle_proc_memory(process_t* proc) {
    assert(proc != NULL);

    //! WARNING: no concurrency guarantee in this impl, please ensure
    //! concurrency security at the caller place

    //! TODO: decide whether to recycle by pcb->hold_* flag
    //! TODO: distinguish between processes and threads

    pcb_t*        pcb    = &proc->pcb;
    lin_memmap_t* memmap = &pcb->memmap;
    ph_info_t*    ph_ptr = memmap->ph_info;
    phyaddr_t     cr3    = pcb->cr3;

    while (ph_ptr != NULL) {
        recycle_memory_part(cr3, (void*)ph_ptr->base, (void*)ph_ptr->limit);
        ph_info_t* old_ph_ptr = ph_ptr;
        ph_ptr                = ph_ptr->next;
        kfree(old_ph_ptr);
    }
    memmap->ph_info = NULL;

    recycle_memory_part(
        cr3, (void*)memmap->vpage_lin_base, (void*)memmap->vpage_lin_limit);
    recycle_memory_part(
        cr3, (void*)memmap->heap_lin_base, (void*)memmap->heap_lin_limit);
    recycle_memory_part(
        cr3, (void*)memmap->arg_lin_base, (void*)memmap->arg_lin_limit);
    recycle_memory_part(
        cr3, (void*)memmap->stack_lin_limit, (void*)memmap->stack_lin_base);

    //! NOTE: phypages of kernel memory is stable and shared, not from any
    //! allocator, under no circumstances can they be released
    bool ok = pg_unmap_laddr_range(
        cr3, memmap->kernel_lin_base, memmap->kernel_lin_limit, false);
    assert(ok);

    ok = pg_unmap_pte(pcb->cr3, true);
    assert(ok);
    ok = pg_free_pde(pcb->cr3);
    assert(ok);

    kfree(pcb->allocator);
    pcb->allocator = NULL;
}

void scavenger() {
    while (true) {
        int pid = wait(NULL);
        if (pid == -1) {
            p_proc_current->pcb.stat = SLEEPING;
            yield();
        } else {
            klog("---recycled orphan! pid[%d]---\n", pid);
        }
    }
}
