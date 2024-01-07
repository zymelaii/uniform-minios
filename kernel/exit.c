#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/spinlock.h>
#include <unios/page.h>
#include <unios/schedule.h>
#include <arch/x86.h>
#include <stdlib.h>
#include <stdio.h>

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

static void exit_handle_child_thread_proc(u32 pid) {
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
            exit_handle_child_thread_proc(child_pcb->tree_info.child_thread[i]);
        }
        lock_or_schedule(&recy_pcb->p_lock);
        transfer_child_proc(child_pcb->pid, NR_RECY_PROC);
        release(&recy_pcb->p_lock);
        child_pcb->stat = IDLE;
    }
    pcb->tree_info.child_t_num = 0;
    return;
}

void do_exit(int exit_code) {
    pcb_t* exit_pcb = NULL;
    pcb_t* fa_pcb   = NULL;
    pcb_t* recy_pcb = (pcb_t*)pid2pcb(NR_RECY_PROC);

    while (true) {
        exit_pcb = (pcb_t*)pid2pcb(p_proc_current->pcb.pid);
        if (!try_lock(&exit_pcb->p_lock)) { schedule(); }
        fa_pcb = (pcb_t*)pid2pcb(exit_pcb->tree_info.ppid);
        if (try_lock(&fa_pcb->p_lock)) { break; }
        release(&exit_pcb->p_lock);
        schedule();
    }

    assert(exit_pcb->tree_info.ppid >= 0 && exit_pcb->tree_info.ppid < NR_PCBS);
    assert(fa_pcb->stat == READY || fa_pcb->stat == SLEEPING);
    exit_handle_child_thread_proc(exit_pcb->pid);
    //! NOTE: disable int to reduce op complexity
    if (fa_pcb->pid == NR_RECY_PROC) {
        //! case 0: father is recy and already locked
        transfer_child_proc(exit_pcb->pid, NR_RECY_PROC);
    } else {
        //! case 1: father isn't recy so need to lock
        lock_or_schedule(&recy_pcb->p_lock);
        transfer_child_proc(exit_pcb->pid, NR_RECY_PROC);
        release(&recy_pcb->p_lock);
    }
    fa_pcb->stat                   = READY;
    exit_pcb->stat                 = ZOMBIE;
    p_proc_current->pcb.p_exitcode = exit_code;
    release(&exit_pcb->p_lock);
    release(&fa_pcb->p_lock);
    schedule();
}
