#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/protect.h>
#include <unios/spinlock.h>
#include <unios/page.h>
#include <unios/memory.h>
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
    pcb_t* src_pcb = (pcb_t*)pid2proc(src_pid);
    pcb_t* dst_pcb = (pcb_t*)pid2proc(dst_pid);
    for (int i = 0, j = dst_pcb->tree_info.child_p_num;
         i < src_pcb->tree_info.child_p_num;
         ++i, ++j) {
        dst_pcb->tree_info.child_process[j] =
            src_pcb->tree_info.child_process[i];
        pcb_t* son_pcb = (pcb_t*)pid2proc(src_pcb->tree_info.child_process[i]);
        lock_or_schedule(&son_pcb->lock);
        son_pcb->tree_info.ppid = dst_pid;
        ++dst_pcb->tree_info.child_p_num;
        release(&son_pcb->lock);
    }
    src_pcb->tree_info.child_p_num = 0;
}

static void killerabbit_handle_child_thread_proc(u32 pid) {
    //! NOTE:fixed, now recursive delete
    pcb_t* pcb      = (pcb_t*)pid2proc(pid);
    pcb_t* recy_pcb = (pcb_t*)pid2proc(NR_RECY_PROC);
    for (int i = 0; i < pcb->tree_info.child_t_num; ++i) {
        pcb_t* child_pcb = (pcb_t*)pid2proc(pcb->tree_info.child_thread[i]);
        u32    cr3       = ((pcb_t*)pid2proc(pid))->cr3;
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
        lock_or_schedule(&recy_pcb->lock);
        transfer_child_proc(child_pcb->pid, NR_RECY_PROC);
        release(&recy_pcb->lock);
        child_pcb->stat = IDLE;
    }
    pcb->tree_info.child_t_num = 0;
    return;
}

static void remove_killed_child(u32 ppid, u32 pid) {
    pcb_t* fa_pcb  = (pcb_t*)pid2proc(ppid);
    bool   cpyflg  = false;
    bool   killflg = false;
    for (int i = 0; i < fa_pcb->tree_info.child_p_num; ++i) {
        pcb_t* exit_child =
            (pcb_t*)pid2proc(fa_pcb->tree_info.child_process[i]);
        assert(!(cpyflg && exit_child->pid == pid));
        if (exit_child->pid == pid) { cpyflg = true; }
        if (cpyflg && i < fa_pcb->tree_info.child_p_num - 1) {
            fa_pcb->tree_info.child_process[i] =
                fa_pcb->tree_info.child_process[i + 1];
        }
    }
    for (int i = 0; i < fa_pcb->tree_info.child_k_num; ++i) {
        //! case 0: forward didn't clean, no need to add
        if (fa_pcb->tree_info.child_killed[i] == pid) { killflg = true; }
    }
    //! case 1: forward cleaned / doesn't have, just add
    if (killflg == false) {
        fa_pcb->tree_info.child_killed[fa_pcb->tree_info.child_k_num] = pid;
        ++fa_pcb->tree_info.child_k_num;
        killflg = true;
    }
    assert(cpyflg);
    assert(killflg);
    --fa_pcb->tree_info.child_p_num;
    return;
}

static void killerabbit_recycle_part(u32 pid, u32 base, u32 limit, bool free) {
    u32 cr3   = ((pcb_t*)pid2proc(pid))->cr3;
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
    pcb_t*        recy_pcb = (pcb_t*)pid2proc(recy_pid);
    lin_memmap_t* memmap   = &recy_pcb->memmap;
    ph_info_t*    ph_ptr   = memmap->ph_info;

    disable_int();

    while (ph_ptr != NULL) {
        killerabbit_recycle_part(recy_pid, ph_ptr->base, ph_ptr->limit, true);
        ph_info_t* old_ph_ptr = ph_ptr;
        ph_ptr                = ph_ptr->next;
        kfree(old_ph_ptr);
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

    kfree(recy_pcb->allocator);
    recy_pcb->allocator = NULL;

    enable_int();
}

/*!
 * \brief kill a thread/process by given pid and wakeup its father
 *
 * \param pid pid == x (x >= 0): to kill the process/thread which pid == x,
 * its child will be transferred to RECY_PROC (kernel task, pid == 2)
 */
int do_killerabbit(int pid) {
    // can't kill shell or kernel task or itself (cr3 will boom!)
    //! TODO: bad solution
    if (pid < NR_TASKS + NR_CONSOLES) { return -1; }

    if (pid == p_proc_current->pcb.pid) { return -1; }
    pcb_t* kill_pcb = NULL;
    pcb_t* fa_pcb   = NULL;
    pcb_t* recy_pcb = (pcb_t*)pid2proc(NR_RECY_PROC);

    while (1) {
        kill_pcb = (pcb_t*)pid2proc(pid);
        //! TODO: implement thread killing!
        if (kill_pcb->tree_info.type == TYPE_THREAD) { return -1; }
        if (kill_pcb->stat != SLEEPING && kill_pcb->stat != READY) {
            return -1;
        }

        if (try_lock(&kill_pcb->lock)) {
            fa_pcb = (pcb_t*)pid2proc(kill_pcb->tree_info.ppid);
            if (try_lock(&fa_pcb->lock)) {
                break;
            } else {
                release(&kill_pcb->lock);
                schedule();
                continue;
            }
        } else {
            schedule();
        }
    }
    klog("-----here to kill [%2d]-----\n", pid);
    assert(fa_pcb->stat == READY || fa_pcb->stat == SLEEPING);
    remove_killed_child(fa_pcb->pid, pid);
    killerabbit_handle_child_thread_proc(pid);
    if (fa_pcb->pid == NR_RECY_PROC) {
        //! case 0: father is recy and already locked
        transfer_child_proc(pid, NR_RECY_PROC);
    } else {
        //! case 1: father isn't recy so need to lock
        lock_or_schedule(&recy_pcb->lock);
        transfer_child_proc(pid, NR_RECY_PROC);
        release(&recy_pcb->lock);
    }
    killerabbit_recycle_memory(pid);
    memset(kill_pcb, 0, sizeof(process_t));
    kill_pcb->pid  = -1;
    kill_pcb->lock = 0;
    kill_pcb->stat = IDLE;
    fa_pcb->stat   = READY;
    release(&kill_pcb->lock);
    release(&fa_pcb->lock);
    return 0;
}
