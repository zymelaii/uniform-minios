#include <assert.h>
#include <proc.h>
#include <stdlib.h>
#include <x86.h>
#include <proto.h>
#include <global.h>
#include <spinlock.h>
#include <stdio.h>

static void transfer_child_proc(u32 src_pid, u32 dst_pid) {
    assert(src_pid != dst_pid);
    PCB_t* src_pcb = (PCB_t*)pid2proc(src_pid);
    PCB_t* dst_pcb = (PCB_t*)pid2proc(dst_pid);
    while (cmpxchg(0, 1, &dst_pcb->p_lock) == 1) { yield(); }
    for (int i = 0, j = dst_pcb->info.child_p_num;
         i < src_pcb->info.child_p_num;
         ++i, ++j) {
        dst_pcb->info.child_process[j] = src_pcb->info.child_process[i];
        PCB_t* son_pcb     = (PCB_t*)pid2proc(src_pcb->info.child_process[i]);
        son_pcb->info.ppid = dst_pid;
    }
    src_pcb->info.child_p_num = 0;
    dst_pcb->p_lock           = 0;
}

static void kill_child_thread(u32 pid) {
    //! PTSD: not fully implemented!
    PCB_t* pcb = (PCB_t*)pid2proc(pid);
    for (int i = 0; i < pcb->info.child_t_num; ++i) {
        //! FIXME: assume that threads have no child process.
        //         actually need recursive killing
        PCB_t* child_pcb = (PCB_t*)pid2proc(pcb->info.child_thread[i]);
        child_pcb->stat  = IDLE;
    }
    pcb->info.child_t_num = 0;
    return;
}

void do_exit(int exit_code) {
    while (cmpxchg(0, 1, &p_proc_current->pcb.p_lock) == 1) { yield(); }
    //! FIXME: this way of locking son then father proc may cause deadlock
    //! according to lab-6 exit.c
    PCB_t* exit_task = (PCB_t*)pid2proc(p_proc_current->pcb.pid);
    PCB_t* fa_task   = (PCB_t*)pid2proc(exit_task->info.ppid);
    // kprintf("\nexit %d",exit_code);
    assert(exit_task->info.ppid >= 0 && exit_task->info.ppid < NR_PCBS);
    assert(fa_task->stat == READY || fa_task->stat == SLEEPING);
    kill_child_thread(exit_task->pid);
    transfer_child_proc(exit_task->pid, fa_task->pid);
    fa_task->stat                  = READY;
    exit_task->stat                = ZOMBIE;
    p_proc_current->pcb.p_exitcode = exit_code;
    p_proc_current->pcb.p_lock     = 0;
    sched();
}
