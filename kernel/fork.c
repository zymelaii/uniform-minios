#include <unios/syscall.h>
#include <unios/malloc.h>
#include <unios/page.h>
#include <unios/proc.h>
#include <unios/assert.h>
#include <arch/x86.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

static void fork_clone_part_rwx(u32 ppid, u32 pid, u32 base, u32 limit) {
    //! FIXME: risky action, this is relevant to current cr3, but ppid may be
    //! not the expected one
    //! FIXME: pid allocation method may changes
    u32 cr3_ppid = ((pcb_t*)pid2pcb(ppid))->cr3;
    u32 cr3_pid  = ((pcb_t*)pid2pcb(pid))->cr3;

    u32 attr        = PG_P | PG_U | PG_RWX;
    u32 laddr_share = SharePageBase;
    assert(laddr_share == pg_frame_phyaddr(laddr_share));

    u32  laddr = base;
    bool ok    = false;
    while (laddr < limit) {
        assert(!pg_pte_exist(
            pg_pte(pg_pde(cr3_ppid, laddr_share), laddr_share), laddr_share));
        bool ok = pg_map_laddr(cr3_ppid, laddr_share, PG_INVALID, attr, attr);
        pg_refresh();
        memcpy((void*)laddr_share, (void*)pg_frame_phyaddr(laddr), NUM_4K);
        u32 phyaddr = pg_laddr_phyaddr(cr3_ppid, laddr_share);
        ok          = pg_map_laddr(cr3_pid, laddr, phyaddr, attr, attr);
        assert(ok);
        laddr = pg_frame_phyaddr(laddr) + NUM_4K;
        ok    = pg_unmap_laddr(cr3_ppid, laddr_share, false);
        assert(ok);
    }
}

static int fork_update_proc_info(process_t* p_child) {
    //! update parent
    int child_index = p_proc_current->pcb.tree_info.child_p_num;
    ++p_proc_current->pcb.tree_info.child_p_num;
    p_proc_current->pcb.tree_info.child_process[child_index] = p_child->pcb.pid;

    //! update child
    p_child->pcb.tree_info.type = p_proc_current->pcb.tree_info.type;
    //! parent creator (may be outdated)
    p_child->pcb.tree_info.real_ppid   = p_proc_current->pcb.pid;
    p_child->pcb.tree_info.ppid        = p_proc_current->pcb.pid;
    p_child->pcb.tree_info.child_p_num = 0;
    p_child->pcb.tree_info.child_t_num = 0;
    p_child->pcb.tree_info.text_hold   = false;
    p_child->pcb.tree_info.data_hold   = true;

    return 0;
}

static int fork_memory_clone(u32 ppid, u32 pid) {
    lin_memmap_t* memmap = &p_proc_current->pcb.memmap;
    ph_info_t*    ph_ptr = memmap->ph_info;
    //! clone elf part
    while (ph_ptr != NULL) {
        fork_clone_part_rwx(ppid, pid, ph_ptr->base, ph_ptr->limit);
        ph_ptr = ph_ptr->next;
    }

    //! FIXME: laddr range might be dynamic according to the impl, simply clone
    //! from base to limit might lost some pages differing from the parent's,
    //! cases are like that, e.g. heap limit reduction, thread clone, etc.

    //! clone vpage
    fork_clone_part_rwx(
        ppid, pid, memmap->vpage_lin_base, memmap->vpage_lin_limit);

    //! clone heap
    fork_clone_part_rwx(
        ppid, pid, memmap->heap_lin_base, memmap->heap_lin_limit);

    //! clone stack
    //! NOTE: inverse grow
    fork_clone_part_rwx(
        ppid, pid, memmap->stack_lin_limit, memmap->stack_lin_base);
    //! clone args
    fork_clone_part_rwx(ppid, pid, memmap->arg_lin_base, memmap->arg_lin_limit);

    return 0;
}

static int fork_pcb_clone(process_t* p_child) {
    //! first frame point in stack
    u32* frame        = (void*)(p_child + 1) - P_STACKTOP;
    u32* parent_frame = (void*)(p_proc_current + 1) - P_STACKTOP;

    //! save status
    int   pid              = p_child->pcb.pid;
    u32   eflags           = frame[NR_EFLAGSREG];
    u32   selector_ldt     = p_child->pcb.ldt_sel;
    u32   cr3_child        = p_child->pcb.cr3;
    char* esp_save_int     = p_child->pcb.esp_save_int;
    char* esp_save_context = p_child->pcb.esp_save_context;

    //! FIXME: risky action, here child proc come into READY unexpectedly
    p_child->pcb        = p_proc_current->pcb;
    p_child->pcb.p_lock = 0;
    //! FIXME: IDLE may not represents the complete status
    p_child->pcb.stat = IDLE;
    memcpy(frame, parent_frame, P_STACKTOP);

    lin_memmap_t* memmap = &p_child->pcb.memmap;
    ph_info_t*    ph_ptr = p_proc_current->pcb.memmap.ph_info;
    memmap->ph_info      = NULL;
    while (ph_ptr != NULL) {
        ph_info_t* new_ph_info =
            (ph_info_t*)K_PHY2LIN(do_kmalloc(sizeof(ph_info_t)));
        new_ph_info->base  = ph_ptr->base;
        new_ph_info->limit = ph_ptr->limit;
        if (memmap->ph_info == NULL) {
            new_ph_info->next   = NULL;
            new_ph_info->before = NULL;
            memmap->ph_info     = new_ph_info;
        } else {
            memmap->ph_info->before = new_ph_info;
            new_ph_info->next       = memmap->ph_info;
            new_ph_info->before     = NULL;
            memmap->ph_info         = new_ph_info;
        }
        ph_ptr = ph_ptr->next;
    }

    // restore status
    p_child->pcb.pid              = pid;
    frame[NR_EFLAGSREG]           = eflags;
    p_child->pcb.ldt_sel          = selector_ldt;
    p_child->pcb.cr3              = cr3_child;
    p_child->pcb.esp_save_int     = esp_save_int;
    p_child->pcb.esp_save_context = esp_save_context;
    p_child->pcb.tree_info.type   = TYPE_PROCESS;
    return 0;
}

int do_fork() {
    //! FIXME: use lock instead
    disable_int();
    process_t* p_child = alloc_pcb();
    if (p_child == NULL) {
        klog(
            "fork failed from pid=%d: pcb res is not available\n",
            p_proc_current->pcb.pid);
        return -1;
    }

    p_child->pcb.cr3 = pg_create_and_init();
    fork_pcb_clone(p_child);
    fork_memory_clone(p_proc_current->pcb.pid, p_child->pcb.pid);
    fork_update_proc_info(p_child);

    //! TODO: modify forked proc name
    strcpy(p_child->pcb.name, "fork");

    //! first frame point in child stack
    u32* frame            = (void*)(p_child + 1) - P_STACKTOP;
    p_child->pcb.regs.eax = 0;
    //! update retval address in stack to be safe
    frame[NR_EAXREG]  = p_child->pcb.regs.eax;
    p_child->pcb.stat = READY;
    //! FIXME: use lock instead (see above)
    enable_int();
    return p_child->pcb.pid;
}
