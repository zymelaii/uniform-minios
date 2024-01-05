#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/proto.h>
#include <unios/global.h>
#include <unios/spinlock.h>
#include <arch/x86.h>
#include <stdlib.h>
#include <stdio.h>

static void exit_transfer_child_proc(u32 src_pid, u32 dst_pid) {
    assert(src_pid != dst_pid);
    pcb_t* src_pcb = (pcb_t*)pid2pcb(src_pid);
    pcb_t* dst_pcb = (pcb_t*)pid2pcb(dst_pid);
    for (int i = 0, j = dst_pcb->info.child_p_num;
         i < src_pcb->info.child_p_num;
         ++i, ++j) {
        dst_pcb->info.child_process[j] = src_pcb->info.child_process[i];
        pcb_t* son_pcb     = (pcb_t*)pid2pcb(src_pcb->info.child_process[i]);
        son_pcb->info.ppid = dst_pid;
        ++dst_pcb->info.child_p_num;
    }
    src_pcb->info.child_p_num = 0;
}

static void exit_force_kill_child_thread(u32 pid) {
    //! FIXME: PTSD! not fully implemented!
    pcb_t* pcb = (pcb_t*)pid2pcb(pid);
    for (int i = 0; i < pcb->info.child_t_num; ++i) {
        //! FIXME: assume that threads have no child process, actually need
        //! recursive killing
        pcb_t* child_pcb = (pcb_t*)pid2pcb(pcb->info.child_thread[i]);
        child_pcb->stat  = IDLE;
    }
    pcb->info.child_t_num = 0;
    return;
}

void do_exit(int exit_code) {
    pcb_t* exit_task = NULL;
    pcb_t* fa_task   = NULL;
    while (1) {
        exit_task = (pcb_t*)pid2pcb(p_proc_current->pcb.pid);
        if (try_lock(&exit_task->p_lock) == true) {
            fa_task = (pcb_t*)pid2pcb(exit_task->info.ppid);
            if (try_lock(&fa_task->p_lock) == true) {
                break;
            } else {
                release(&exit_task->p_lock);
                sched();
                continue;
            }
        } else {
            sched();
        }
    }

    assert(exit_task->info.ppid >= 0 && exit_task->info.ppid < NR_PCBS);
    assert(fa_task->stat == READY || fa_task->stat == SLEEPING);
    exit_force_kill_child_thread(exit_task->pid);
    //! NOTE: disable int to reduce op complexity
    disable_int();
    exit_transfer_child_proc(exit_task->pid, NR_RECY_PROC);
    enable_int();
    fa_task->stat                  = READY;
    exit_task->stat                = ZOMBIE;
    p_proc_current->pcb.p_exitcode = exit_code;
    release(&exit_task->p_lock);
    release(&fa_task->p_lock);
    sched();
}
