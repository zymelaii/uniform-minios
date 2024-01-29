#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/page.h>
#include <unios/syscall.h>
#include <unios/schedule.h>
#include <unios/memory.h>
#include <unios/scavenger.h>
#include <arch/x86.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <atomic.h>
#include <string.h>

static pcb_t* try_get_zombie_child(u32 pid) {
    pcb_t* pcb = (pcb_t*)pid2proc(pid);
    for (int i = 0; i < pcb->tree_info.child_p_num; ++i) {
        pcb_t* exit_child = (pcb_t*)pid2proc(pcb->tree_info.child_process[i]);
        if (exit_child->stat == ZOMBIE) { return exit_child; }
    }
    return NULL;
}

static int try_remove_killed_child(u32 pid) {
    pcb_t* pcb = (pcb_t*)pid2proc(pid);
    if (pcb->tree_info.child_k_num == 0) { return -1; }
    for (int i = pcb->tree_info.child_k_num - 1; i >= 0; --i) {
        --pcb->tree_info.child_k_num;
        return (int)pcb->tree_info.child_killed[i];
    }
    unreachable();
}

static void remove_zombie_child(u32 pid) {
    pcb_t* pcb    = &p_proc_current->pcb;
    bool   cpyflg = false;
    for (int i = 0; i < pcb->tree_info.child_p_num; ++i) {
        pcb_t* exit_child = (pcb_t*)pid2proc(pcb->tree_info.child_process[i]);
        assert(!(cpyflg && exit_child->pid == pid));
        if (exit_child->pid == pid) { cpyflg = true; }
        if (cpyflg && i < pcb->tree_info.child_p_num - 1) {
            pcb->tree_info.child_process[i] =
                pcb->tree_info.child_process[i + 1];
        }
    }
    assert(cpyflg);
    --pcb->tree_info.child_p_num;
    return;
}

static void wait_recycle_memory(u32 recy_pid) {
    assert(recy_pid != p_proc_current->pcb.pid);
    disable_int();
    recycle_proc_memory(pid2proc(recy_pid));
    enable_int();
}

int do_wait(int* wstatus) {
    pcb_t* fa_pcb = &p_proc_current->pcb;
    while (true) {
        lock_or(&fa_pcb->lock, schedule);
        if (fa_pcb->tree_info.child_p_num == 0
            && fa_pcb->tree_info.child_k_num == 0) {
            if (wstatus != NULL) { *wstatus = 0; }
            release(&fa_pcb->lock);
            return -1;
        }
        pcb_t* exit_pcb = try_get_zombie_child(fa_pcb->pid);
        if (exit_pcb == NULL) {
            int pid = try_remove_killed_child(fa_pcb->pid);
            if (pid != -1) {
                release(&fa_pcb->lock);
                return pid;
            }
            disable_int();
            fa_pcb->stat = SLEEPING;
            release(&fa_pcb->lock);
            schedule();
            continue;
        }
        lock_or(&exit_pcb->lock, schedule);
        remove_zombie_child(exit_pcb->pid);
        if (wstatus != NULL) { *wstatus = exit_pcb->exit_code; }
        //! FIXME: no thread release here
        wait_recycle_memory(exit_pcb->pid);
        int pid = exit_pcb->pid;
        //! FIXME: lock also release here
        disable_int();
        memset(exit_pcb, 0, sizeof(process_t));
        assert(!exit_pcb->tree_info.child_k_num);
        assert(!exit_pcb->tree_info.child_p_num);
        assert(!exit_pcb->tree_info.child_t_num);
        assert(!exit_pcb->tree_info.ppid);
        assert(!exit_pcb->tree_info.real_ppid);
        exit_pcb->pid  = -1;
        exit_pcb->stat = IDLE;
        release(&exit_pcb->lock);
        release(&fa_pcb->lock);
        enable_int();
        return pid;
    }
}
