#include <unios/syscall.h>
#include <unios/malloc.h>
#include <unios/page.h>
#include <unios/const.h>
#include <unios/proc.h>
#include <unios/global.h>
#include <unios/proto.h>
#include <unios/assert.h>
#include <arch/x86.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

static void fork_clone_part_rwx(u32 ppid, u32 pid, u32 base, u32 limit) {
    //! FIXME: risky action, this is relevant to current cr3, but ppid may be
    //! not the expected one
    //! FIXME: pid allocation method may changes
    u32 cr3_ppid = proc_table[ppid].pcb.cr3;
    u32 cr3_pid  = proc_table[pid].pcb.cr3;

    u32 attr        = PG_P | PG_U | PG_RWX;
    u32 laddr_share = SharePageBase;
    assert(laddr_share == pg_frame_phyaddr(laddr_share));

    u32 laddr = base;
    while (laddr < limit) {
        bool ok = pg_unmap_laddr(cr3_ppid, laddr_share, false);
        assert(ok);
        ok = pg_map_laddr(cr3_ppid, laddr_share, 0, attr, attr);
        pg_refresh();
        memcpy((void*)laddr_share, (void*)pg_frame_phyaddr(laddr), num_4K);
        u32 phyaddr = pg_laddr_phyaddr(cr3_ppid, laddr_share);
        ok          = pg_map_laddr(cr3_pid, laddr, phyaddr, attr, attr);
        assert(ok);
        laddr = pg_frame_phyaddr(laddr) + num_4K;
    }
}

static int fork_update_proc_info(PROCESS* p_child) {
    //! update parent
    int child_index = p_proc_current->pcb.info.child_p_num;
    ++p_proc_current->pcb.info.child_p_num;
    p_proc_current->pcb.info.child_process[child_index] = p_child->pcb.pid;

    //! update child
    p_child->pcb.info.type = p_proc_current->pcb.info.type;
    //! parent creator (may be outdated)
    p_child->pcb.info.real_ppid   = p_proc_current->pcb.pid;
    p_child->pcb.info.ppid        = p_proc_current->pcb.pid;
    p_child->pcb.info.child_p_num = 0;
    p_child->pcb.info.child_t_num = 0;
    p_child->pcb.info.text_hold   = false;
    p_child->pcb.info.data_hold   = true;

    return 0;
}

static int fork_memory_clone(u32 ppid, u32 pid) {
    LIN_MEMMAP* memmap = &p_proc_current->pcb.memmap;
    PH_INFO*    ph_ptr = memmap->ph_info;

    //! clone elf part
    while (ph_ptr != NULL) {
        fork_clone_part_rwx(
            ppid, pid, ph_ptr->lin_addr_base, ph_ptr->lin_addr_limit);
        ph_ptr = ph_ptr->next;
    }

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
    fork_clone_part_rwx(ppid, pid, memmap->arg_lin_limit, memmap->arg_lin_base);

    return 0;
}

static int fork_pcb_clone(PROCESS* p_child) {
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
    p_child->pcb      = p_proc_current->pcb;
    p_child->pcb.stat = IDLE;

    memcpy(frame, parent_frame, P_STACKTOP);

    LIN_MEMMAP* memmap = &p_child->pcb.memmap;
    PH_INFO*    ph_ptr = p_proc_current->pcb.memmap.ph_info;
    memmap->ph_info    = NULL;
    while (ph_ptr != NULL) {
        PH_INFO* new_ph_info = (PH_INFO*)K_PHY2LIN(do_kmalloc(sizeof(PH_INFO)));
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

    memmap->vpage_lin_base  = p_proc_current->pcb.memmap.vpage_lin_base;
    memmap->vpage_lin_limit = p_proc_current->pcb.memmap.vpage_lin_limit;
    memmap->heap_lin_base   = p_proc_current->pcb.memmap.heap_lin_base;
    memmap->heap_lin_limit  = p_proc_current->pcb.memmap.heap_lin_limit;
    memmap->stack_lin_limit = p_proc_current->pcb.memmap.stack_lin_limit;
    memmap->stack_lin_base  = p_proc_current->pcb.memmap.stack_lin_base;
    memmap->arg_lin_limit   = p_proc_current->pcb.memmap.arg_lin_limit;
    memmap->arg_lin_base    = p_proc_current->pcb.memmap.arg_lin_base;

    // restore status
    p_child->pcb.pid              = pid;
    frame[NR_EFLAGSREG]           = eflags;
    p_child->pcb.ldt_sel          = selector_ldt;
    p_child->pcb.cr3              = cr3_child;
    p_child->pcb.esp_save_int     = esp_save_int;
    p_child->pcb.esp_save_context = esp_save_context;

    return 0;
}

int do_fork() {
    //! FIXME: use lock instead
    PROCESS* p_child = alloc_pcb();
    if (p_child == NULL) {
        trace_logging(
            "fork failed from pid=%d: pcb res is not available",
            p_proc_current->pcb.pid);
        return -1;
    }

    p_child->pcb.cr3 = pg_create_and_init();
    fork_pcb_clone(p_child);
    fork_memory_clone(p_proc_current->pcb.pid, p_child->pcb.pid);
    fork_update_proc_info(p_child);

    //! TODO: modify forked proc name
    strcpy(p_child->pcb.p_name, "fork");

    //! first frame point in child stack
    u32* frame            = (void*)(p_child + 1) - P_STACKTOP;
    p_child->pcb.regs.eax = 0;
    //! update retval address in stack to be safe
    frame[NR_EAXREG] = p_child->pcb.regs.eax;

    //! fork done
    ++u_proc_sum;
    p_child->pcb.stat = READY;

    //! FIXME: use lock instead (see above)
    return p_child->pcb.pid;
}
