#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/protect.h>
#include <unios/spinlock.h>
#include <unios/page.h>
#include <unios/schedule.h>
#include <unios/syscall.h>
#include <arch/x86.h>
#include <sys/defs.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void transfer_child_proc(u32 src_pid, u32 dst_pid) {
    assert(src_pid != dst_pid);
    pcb_t* src_pcb = (pcb_t*)pid2pcb(src_pid);
    pcb_t* dst_pcb = (pcb_t*)pid2pcb(dst_pid);
    for (int i = 0, j = dst_pcb->tree_info.child_p_num;
         i < src_pcb->tree_info.child_p_num;
         ++i, ++j) {
        dst_pcb->tree_info.child_process[j] =
            src_pcb->tree_info.child_process[i];
        pcb_t* son_pcb = (pcb_t*)pid2pcb(src_pcb->tree_info.child_process[i]);
        lock_or_schedule(&son_pcb->p_lock);
        son_pcb->tree_info.ppid = dst_pid;
        ++dst_pcb->tree_info.child_p_num;
        release(&son_pcb->p_lock);
    }
    src_pcb->tree_info.child_p_num = 0;
}

static void killerabbit_handle_child_thread_proc(u32 pid) {
    //! NOTE:fixed, now recursive delete
    pcb_t* pcb      = (pcb_t*)pid2pcb(pid);
    pcb_t* recy_pcb = (pcb_t*)pid2pcb(NR_RECY_PROC);
    for (int i = 0; i < pcb->tree_info.child_t_num; ++i) {
        pcb_t* child_pcb = (pcb_t*)pid2pcb(pcb->tree_info.child_thread[i]);
        u32    cr3       = ((pcb_t*)pid2pcb(pid))->cr3;
        u32    laddr     = child_pcb->memmap.stack_lin_limit;
        u32    limit     = child_pcb->memmap.stack_lin_base;
        while (laddr < limit) {
            bool ok = pg_unmap_laddr(cr3, laddr, true);
            assert(ok);
            laddr = pg_frame_phyaddr(laddr) + NUM_4K;
        }
        for (int i = 0; i < child_pcb->tree_info.child_t_num; ++i) {
            killerabbit_handle_child_thread_proc(
                child_pcb->tree_info.child_thread[i]);
        }
        lock_or_schedule(&recy_pcb->p_lock);
        transfer_child_proc(child_pcb->pid, NR_RECY_PROC);
        release(&recy_pcb->p_lock);
        child_pcb->stat = IDLE;
    }
    pcb->tree_info.child_t_num = 0;
    return;
}

static void remove_killed_child(u32 ppid, u32 pid) {
    pcb_t* fa_pcb = (pcb_t*)pid2pcb(ppid);
    bool   cpyflg = false;
    for (int i = 0; i < fa_pcb->tree_info.child_p_num; ++i) {
        pcb_t* exit_child = (pcb_t*)pid2pcb(fa_pcb->tree_info.child_process[i]);
        assert(!(cpyflg && exit_child->pid == pid));
        if (exit_child->pid == pid) { cpyflg = true; }
        if (cpyflg && i < fa_pcb->tree_info.child_p_num - 1) {
            fa_pcb->tree_info.child_process[i] =
                fa_pcb->tree_info.child_process[i + 1];
        }
    }
    assert(cpyflg);
    --fa_pcb->tree_info.child_p_num;
    return;
}

static void killerabbit_recycle_part(u32 pid, u32 base, u32 limit, bool free) {
    u32 cr3   = ((pcb_t*)pid2pcb(pid))->cr3;
    u32 laddr = base;
    u32 parts = 0;
    while (laddr < limit) {
        parts++;
        bool ok = pg_unmap_laddr(cr3, laddr, free);
        assert(ok);
        laddr = pg_frame_phyaddr(laddr) + NUM_4K;
    }
}

static void killerabbit_recycle_memory(u32 recy_pid) {
    assert(recy_pid != p_proc_current->pcb.pid);
    pcb_t*        recy_pcb = (pcb_t*)pid2pcb(recy_pid);
    lin_memmap_t* memmap   = &recy_pcb->memmap;
    ph_info_t*    ph_ptr   = memmap->ph_info;
    disable_int();
    while (ph_ptr != NULL) {
        killerabbit_recycle_part(recy_pid, ph_ptr->base, ph_ptr->limit, true);
        ph_info_t* old_ph_ptr = ph_ptr;
        ph_ptr                = ph_ptr->next;
        do_free((void*)K_LIN2PHY(old_ph_ptr));
    }
    memmap->ph_info = NULL;
    killerabbit_recycle_part(
        recy_pid, memmap->vpage_lin_base, memmap->vpage_lin_limit, true);
    killerabbit_recycle_part(
        recy_pid, memmap->heap_lin_base, memmap->heap_lin_limit, true);
    killerabbit_recycle_part(
        recy_pid, memmap->arg_lin_base, memmap->arg_lin_limit, true);
    killerabbit_recycle_part(
        recy_pid, memmap->kernel_lin_base, memmap->kernel_lin_limit, false);
    killerabbit_recycle_part(
        recy_pid, memmap->stack_lin_limit, memmap->stack_lin_base, true);
    pg_unmap_pte(recy_pcb->cr3, true);
    pg_free_pde(recy_pcb->cr3);
    enable_int();
}

static void killerabbit_reset_killed(u32 pid) {
    pcb_t* recy_pcb = (pcb_t*)pid2pcb(pid);
    disable_int();
    strcpy(recy_pcb->name, "USER");
    recy_pcb->pid    = pid;
    recy_pcb->p_lock = 0;
    memcpy(
        &recy_pcb->ldts[0],
        &gdt[SELECTOR_KERNEL_CS >> 3],
        sizeof(descriptor_t));
    recy_pcb->ldts[0].attr0 = DA_C | RPL_USER << 5;
    memcpy(
        &recy_pcb->ldts[1],
        &gdt[SELECTOR_KERNEL_DS >> 3],
        sizeof(descriptor_t));
    recy_pcb->ldts[1].attr0 = DA_DRW | RPL_USER << 5;
    recy_pcb->regs.cs =
        ((8 * 0) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
    recy_pcb->regs.ds =
        ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
    recy_pcb->regs.es =
        ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
    recy_pcb->regs.fs =
        ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
    recy_pcb->regs.ss =
        ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
    recy_pcb->regs.gs     = (SELECTOR_KERNEL_GS & SA_MASK_RPL) | RPL_USER;
    recy_pcb->regs.eflags = 0x0202;

    //! FIXME: rewrite antihuman expresssions as below
    char* p_regs = (char*)((process_t*)recy_pcb + 1);
    p_regs       -= P_STACKTOP;
    memcpy(p_regs, &recy_pcb->regs, P_STACKTOP);
    recy_pcb->esp_save_int     = p_regs;
    recy_pcb->esp_save_context = p_regs - 10 * 4;
    task_handler_t eip_context = restart_restore;
    *(u32*)(p_regs - 4)        = (u32)eip_context;
    *(u32*)(p_regs - 8)        = 0x1202;
    recy_pcb->stat             = IDLE;
    enable_int();
}

/*!
 * \brief kill a thread/process by given pid and wakeup its father
 *
 * \param pid pid == x (x >= 0): to kill the process/thread which pid == x,
 * its child will be transferred to RECY_PROC (kernel task, pid == 2)
 */
int do_killerabbit(int pid) {
    // can't kill shell or kernel task
    if (pid < NR_K_PCBS + NR_CONSOLES) { return -1; }
    pcb_t* kill_pcb = NULL;
    pcb_t* fa_pcb   = NULL;
    pcb_t* recy_pcb = (pcb_t*)pid2pcb(NR_RECY_PROC);

    while (1) {
        kill_pcb = (pcb_t*)pid2pcb(pid);
        //! TODO: implement thread killing!
        if (kill_pcb->tree_info.type == TYPE_THREAD) { return -1; }
        if (kill_pcb->stat != SLEEPING && kill_pcb->stat != READY) {
            return -1;
        }
        if (try_lock(&kill_pcb->p_lock)) {
            fa_pcb = (pcb_t*)pid2pcb(kill_pcb->tree_info.ppid);
            if (try_lock(&fa_pcb->p_lock)) {
                break;
            } else {
                release(&kill_pcb->p_lock);
                schedule();
                continue;
            }
        } else {
            schedule();
        }
    }
    assert(fa_pcb->stat == READY || fa_pcb->stat == SLEEPING);
    remove_killed_child(fa_pcb->pid, pid);
    killerabbit_handle_child_thread_proc(pid);
    if (fa_pcb->pid == NR_RECY_PROC) {
        //! case 0:father is recy and already locked
        transfer_child_proc(pid, NR_RECY_PROC);
    } else {
        //! case 1:father isn't recy so need to lock
        lock_or_schedule(&recy_pcb->p_lock);
        transfer_child_proc(pid, NR_RECY_PROC);
        release(&recy_pcb->p_lock);
    }
    killerabbit_recycle_memory(pid);
    killerabbit_reset_killed(pid);
    fa_pcb->stat = READY;
    release(&kill_pcb->p_lock);
    release(&fa_pcb->p_lock);
    return 0;
}
