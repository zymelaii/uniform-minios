#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/spinlock.h>
#include <unios/page.h>
#include <unios/syscall.h>
#include <unios/schedule.h>
#include <unios/memory.h>
#include <arch/x86.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
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
    panic("unreachable");
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

static void wait_recycle_part(u32 pid, u32 base, u32 limit, bool free) {
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

static void wait_recycle_memory(u32 recy_pid) {
    assert(recy_pid != p_proc_current->pcb.pid);
    pcb_t*        recy_pcb = (pcb_t*)pid2proc(recy_pid);
    lin_memmap_t* memmap   = &recy_pcb->memmap;
    ph_info_t*    ph_ptr   = memmap->ph_info;
    disable_int();
    while (ph_ptr != NULL) {
        wait_recycle_part(recy_pid, ph_ptr->base, ph_ptr->limit, true);
        ph_info_t* old_ph_ptr = ph_ptr;
        ph_ptr                = ph_ptr->next;
        kfree(old_ph_ptr);
    }
    memmap->ph_info = NULL;
    wait_recycle_part(
        recy_pid, memmap->vpage_lin_base, memmap->vpage_lin_limit, true);
    wait_recycle_part(
        recy_pid, memmap->heap_lin_base, memmap->heap_lin_limit, true);
    wait_recycle_part(
        recy_pid, memmap->arg_lin_base, memmap->arg_lin_limit, true);
    wait_recycle_part(
        recy_pid, memmap->kernel_lin_base, memmap->kernel_lin_limit, false);
    wait_recycle_part(
        recy_pid, memmap->stack_lin_limit, memmap->stack_lin_base, true);
    pg_unmap_pte(recy_pcb->cr3, true);
    pg_free_pde(recy_pcb->cr3);
    enable_int();
}

int do_wait(int* wstatus) {
    pcb_t* fa_pcb = &p_proc_current->pcb;
    while (true) {
        lock_or_schedule(&p_proc_current->pcb.lock);
        if (fa_pcb->tree_info.child_p_num == 0
            && fa_pcb->tree_info.child_k_num == 0) {
            if (wstatus != NULL) { *wstatus = 0; }
            p_proc_current->pcb.lock = 0;
            return -1;
        }
        pcb_t* exit_pcb = try_get_zombie_child(fa_pcb->pid);
        if (exit_pcb == NULL) {
            int pid = try_remove_killed_child(fa_pcb->pid);
            if (pid != -1) {
                release(&fa_pcb->lock);
                return pid;
            }
            fa_pcb->stat = SLEEPING;
            release(&fa_pcb->lock);
            schedule();
            continue;
        }
        lock_or_schedule(&exit_pcb->lock);
        remove_zombie_child(exit_pcb->pid);
        if (wstatus != NULL) { *wstatus = exit_pcb->exit_code; }
        //! FIXME: no thread release here
        wait_recycle_memory(exit_pcb->pid);
        int pid = exit_pcb->pid;
        //! FIXME: lock also release here
        memset(exit_pcb, 0, sizeof(process_t));
        exit_pcb->pid  = -1;
        exit_pcb->stat = IDLE;
        exit_pcb->lock = 0;
        release(&exit_pcb->lock);
        release(&fa_pcb->lock);
        return pid;
    }
}
