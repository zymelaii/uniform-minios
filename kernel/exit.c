#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/page.h>
#include <unios/schedule.h>
#include <arch/x86.h>
#include <atomic.h>
#include <stdlib.h>
#include <stdio.h>

static void exit_handle_child_killed_proc(u32 pid) {
    pcb_t* pcb                 = (pcb_t*)pid2proc(pid);
    pcb->tree_info.child_k_num = 0;
}

static int transfer_child_proc(u32 src_pid, u32 dst_pid) {
    int number = 0;
    assert(src_pid != dst_pid);
    pcb_t* src_pcb = (pcb_t*)pid2proc(src_pid);
    pcb_t* dst_pcb = (pcb_t*)pid2proc(dst_pid);
    for (int i = 0, j = dst_pcb->tree_info.child_p_num;
         i < src_pcb->tree_info.child_p_num;
         ++i, ++j) {
        dst_pcb->tree_info.child_process[j] =
            src_pcb->tree_info.child_process[i];
        pcb_t* son_pcb = (pcb_t*)pid2proc(src_pcb->tree_info.child_process[i]);
        lock_or(&son_pcb->lock, schedule);
        son_pcb->tree_info.ppid = dst_pid;
        son_pcb->stat           = ZOMBIE;
        ++dst_pcb->tree_info.child_p_num;
        release(&son_pcb->lock);
    }
    number                         = src_pcb->tree_info.child_p_num;
    src_pcb->tree_info.child_p_num = 0;
    return number;
}

static void exit_handle_child_thread_proc(u32 pid, bool lock_recy) {
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
            exit_handle_child_thread_proc(
                child_pcb->tree_info.child_thread[i], lock_recy);
        }
        if (lock_recy) {
            lock_or(&recy_pcb->lock, schedule);
            transfer_child_proc(child_pcb->pid, NR_RECY_PROC);
            release(&recy_pcb->lock);
        } else {
            transfer_child_proc(child_pcb->pid, NR_RECY_PROC);
        }

        child_pcb->stat = IDLE;
    }
    pcb->tree_info.child_t_num = 0;
    return;
} // 8049143

void do_exit(int exit_code) {
    pcb_t* exit_pcb = NULL;
    pcb_t* fa_pcb   = NULL;
    pcb_t* recy_pcb = (pcb_t*)pid2proc(NR_RECY_PROC);
    while (true) {
        exit_pcb = (pcb_t*)pid2proc(p_proc_current->pcb.pid);
        if (!try_lock(&exit_pcb->lock)) { schedule(); }
        fa_pcb = (pcb_t*)pid2proc(exit_pcb->tree_info.ppid);
        if (try_lock(&fa_pcb->lock)) { break; }
        release(&exit_pcb->lock);
        schedule();
    }

    assert(exit_pcb->tree_info.ppid >= 0 && exit_pcb->tree_info.ppid < NR_PCBS);
    assert(fa_pcb->stat == READY || fa_pcb->stat == SLEEPING);
    exit_handle_child_killed_proc(exit_pcb->pid);
    //! NOTE: disable int to reduce op complexity
    if (fa_pcb->pid == NR_RECY_PROC) {
        //! case 0: father is recy and already locked
        exit_handle_child_thread_proc(exit_pcb->pid, false);
        transfer_child_proc(exit_pcb->pid, NR_RECY_PROC);
    } else {
        //! case 1: father isn't recy so need to lock
        exit_handle_child_thread_proc(exit_pcb->pid, true);
        lock_or(&recy_pcb->lock, schedule);
        if (transfer_child_proc(exit_pcb->pid, NR_RECY_PROC) != 0) {
            recy_pcb->stat = READY;
        }
        release(&recy_pcb->lock);
    }
    disable_int();
    fa_pcb->stat        = READY;
    exit_pcb->stat      = ZOMBIE;
    exit_pcb->exit_code = exit_code;
    assert(exit_pcb->lock);
    assert(fa_pcb->lock);
    release(&exit_pcb->lock);
    release(&fa_pcb->lock);
    schedule();
}
