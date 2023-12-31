#include <unios/syscall.h>
#include <x86.h>
#include <unios/malloc.h>
#include <type.h>
#include <const.h>
#include <proc.h>
#include <global.h>
#include <proto.h>
#include <string.h>
#include <stdio.h>

static void fork_clone_part_rww(u32 ppid, u32 pid, u32 base, u32 limit) {
    for (u32 laddr = base; laddr < limit; laddr += num_4K) {
        //! clear phy page mapping
        lin_mapping_phy(SharePageBase, 0, ppid, PG_P | PG_USU | PG_RWW, 0);
        //! alloc shared page for clone, also the phy page mapping from laddr
        lin_mapping_phy(
            SharePageBase,
            MAX_UNSIGNED_INT,
            ppid,
            PG_P | PG_USU | PG_RWW,
            PG_P | PG_USU | PG_RWW);
        memcpy((void*)SharePageBase, (void*)(laddr & 0xfffff000), num_4K);
        lin_mapping_phy(
            laddr,
            get_page_phy_addr(ppid, SharePageBase),
            pid,
            PG_P | PG_USU | PG_RWW,
            PG_P | PG_USU | PG_RWW);
    }
}

static int fork_update_proc_info(PROCESS* p_child) {
    //! update parent
    int child_index = p_proc_current->task.info.child_p_num;
    ++p_proc_current->task.info.child_p_num;
    p_proc_current->task.info.child_process[child_index] = p_child->task.pid;

    //! update child
    p_child->task.info.type = p_proc_current->task.info.type;
    //! parent creator (may be outdated)
    p_child->task.info.real_ppid   = p_proc_current->task.pid;
    p_child->task.info.ppid        = p_proc_current->task.pid;
    p_child->task.info.child_p_num = 0;
    p_child->task.info.child_t_num = 0;
    p_child->task.info.text_hold   = false;
    p_child->task.info.data_hold   = true;

    return 0;
}

static int fork_memory_clone(u32 ppid, u32 pid) {
    LIN_MEMMAP* memmap = &p_proc_current->task.memmap;
    PH_INFO*    ph_ptr = memmap->ph_info;

    //! clone elf part
    while (ph_ptr != NULL) {
        fork_clone_part_rww(
            ppid, pid, ph_ptr->lin_addr_base, ph_ptr->lin_addr_limit);
        ph_ptr = ph_ptr->next;
    }

    //! clone vpage
    fork_clone_part_rww(
        ppid, pid, memmap->vpage_lin_base, memmap->vpage_lin_limit);

    //! clone heap
    fork_clone_part_rww(
        ppid, pid, memmap->heap_lin_base, memmap->heap_lin_limit);

    //! clone stack
    //! NOTE: inverse grow
    fork_clone_part_rww(
        ppid, pid, memmap->stack_lin_limit, memmap->stack_lin_base);

    //! clone args
    fork_clone_part_rww(ppid, pid, memmap->arg_lin_limit, memmap->arg_lin_base);

    return 0;
}

static int fork_pcb_clone(PROCESS* p_child) {
    //! first frame point in stack
    u32* frame        = (u32*)((void*)(p_child + 1) - P_STACKTOP);
    u32* parent_frame = (u32*)((void*)(p_proc_current + 1) - P_STACKTOP);

    //! save status
    int   pid              = p_child->task.pid;
    u32   eflags           = frame[NR_EFLAGSREG];
    u32   selector_ldt     = p_child->task.ldt_sel;
    u32   cr3_child        = p_child->task.cr3;
    char* esp_save_int     = p_child->task.esp_save_int;
    char* esp_save_context = p_child->task.esp_save_context;

    //! FIXME: risky action, here child proc come into READY unexpectedly
    p_child->task      = p_proc_current->task;
    p_child->task.stat = IDLE;

    memcpy(frame, parent_frame, P_STACKTOP);

    LIN_MEMMAP* memmap = &p_child->task.memmap;
    PH_INFO*    ph_ptr = p_proc_current->task.memmap.ph_info;
    memmap->ph_info    = NULL;
    while (ph_ptr != NULL) {
        PH_INFO* new_ph_info        = (PH_INFO*)do_kmalloc(sizeof(PH_INFO));
        new_ph_info->lin_addr_base  = ph_ptr->lin_addr_base;
        new_ph_info->lin_addr_limit = ph_ptr->lin_addr_limit;
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
    p_child->task.pid              = pid;
    frame[NR_EFLAGSREG]            = eflags;
    p_child->task.ldt_sel          = selector_ldt;
    p_child->task.cr3              = cr3_child;
    p_child->task.esp_save_int     = esp_save_int;
    p_child->task.esp_save_context = esp_save_context;

    return 0;
}

int do_fork() {
    //! FIXME: use lock instead
    disable_int();

    PROCESS* p_child = alloc_pcb();
    if (p_child == NULL) {
        trace_logging(
            "fork failed from pid=%d: pcb res is not available",
            p_proc_current->task.pid);
        return -1;
    }

    init_page_pte(p_child->task.pid);
    fork_pcb_clone(p_child);
    fork_memory_clone(p_proc_current->task.pid, p_child->task.pid);
    fork_update_proc_info(p_child);

    //! TODO: modify forked proc name
    strcpy(p_child->task.p_name, "fork");

    //! first frame point in child stack
    u32* frame             = (u32*)((void*)(p_child + 1) - P_STACKTOP);
    p_child->task.regs.eax = 0;
    //! update retval address in stack to be safe
    frame[NR_EAXREG] = p_child->task.regs.eax;

    //! fork done
    ++u_proc_sum;
    p_child->task.stat = READY;

    //! FIXME: use lock instead (see above)
    enable_int();
    return p_child->task.pid;
}
