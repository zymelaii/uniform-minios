#include <unios/syscall.h>
#include <unios/malloc.h>
#include <unios/page.h>
#include <unios/proc.h>
#include <unios/spinlock.h>
#include <unios/assert.h>
#include <unios/schedule.h>
#include <arch/x86.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

static void fork_clone_part_rwx(u32 ppid, u32 pid, u32 base, u32 limit) {
    //! FIXME: risky action, this is relevant to current cr3, but ppid may be
    //! not the expected one
    //! FIXME: pid allocation method may changes
    u32 cr3_ppid = ((pcb_t*)pid2proc(ppid))->cr3;
    u32 cr3_pid  = ((pcb_t*)pid2proc(pid))->cr3;

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
    pcb_t* fa = &p_proc_current->pcb;
    pcb_t* ch = &p_child->pcb;

    assert(fa->tree_info.child_p_num < NR_CHILD_MAX);
    fa->tree_info.child_process[fa->tree_info.child_p_num++] = ch->pid;

    ch->tree_info.type        = fa->tree_info.type;
    ch->tree_info.real_ppid   = fa->pid;
    ch->tree_info.ppid        = fa->pid;
    ch->tree_info.child_p_num = 0;
    ch->tree_info.child_t_num = 0;
    ch->tree_info.child_k_num = 0;
    ch->tree_info.text_hold   = false;
    ch->tree_info.data_hold   = true;

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
    pcb_t* fa = &p_proc_current->pcb;
    pcb_t* ch = &p_child->pcb;

    u32* ch_frame = (void*)(p_child + 1) - P_STACKTOP;
    u32* fa_frame = (void*)(p_proc_current + 1) - P_STACKTOP;

    //! shared part
    ch->regs             = fa->regs;
    ch->esp_save_syscall = fa->esp_save_syscall;
    ch->channel          = fa->channel;
    //! FIXME: see `fork_memory_clone`
    ch->memmap     = fa->memmap;
    ch->live_ticks = fa->live_ticks;
    ch->priority   = fa->priority;
    ch->exit_code  = fa->exit_code;
    assert(ch->exit_code == 0);
    strcpy(ch->name, fa->name);
    memcpy(ch->ldts, fa->ldts, sizeof(fa->ldts));
    memcpy(ch->filp, fa->filp, sizeof(fa->filp));
    memcpy(ch_frame, fa_frame, P_STACKTOP);

    //! unique part
    assert(ch->cr3 != 0 && ch->cr3 != fa->cr3);
    ch->memmap.ph_info = clone_ph_info(fa->memmap.ph_info);

    //! TODO: better pid assignment method
    ch->pid = proc2pid(p_child);

    //! TODO: better ldt selector assignment method
    ch->ldt_sel = SELECTOR_LDT_FIRST + (ch->pid << 3);

    //! NOTE: forked child proc should start at user space, see `init_proc_pcb`
    //! for more details
    ch->esp_save_int     = (void*)ch_frame;
    ch->esp_save_context = (void*)(ch_frame - 10);
    memset(ch->esp_save_context, 0, sizeof(u32) * 10);
    ch_frame[-1] = (u32)restart_restore;
    ch_frame[-2] = ch_frame[NR_EFLAGSREG];

    //! NOTE: proc family tree will be maintained later

    return 0;
}

int do_fork() {
    process_t* fa = p_proc_current;
    lock_or_schedule(&fa->pcb.lock);

    process_t* ch = try_lock_free_pcb();
    if (ch == NULL) {
        klog("fork %d: pcb res is not available\n", fa->pcb.pid);
        release(&fa->pcb.lock);
        return -1;
    }

    ch->pcb.cr3 = pg_create_and_init();
    fork_pcb_clone(ch);
    fork_memory_clone(fa->pcb.pid, ch->pcb.pid);
    fork_update_proc_info(ch);

    u32* frame       = (void*)(ch + 1) - P_STACKTOP;
    ch->pcb.regs.eax = 0;
    //! update retval address in stack to be safe
    frame[NR_EAXREG] = ch->pcb.regs.eax;

    //! done
    ch->pcb.stat = READY;
    release(&ch->pcb.lock);
    release(&fa->pcb.lock);
    return ch->pcb.pid;
}
