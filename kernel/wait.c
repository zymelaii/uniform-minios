#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/proto.h>
#include <unios/global.h>
#include <unios/spinlock.h>
#include <unios/page.h>
#include <unios/syscall.h>
#include <arch/x86.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static pcb_t* try_get_zombie_child() {
    pcb_t* pcb = &p_proc_current->pcb;
    for (int i = 0; i < pcb->info.child_p_num; ++i) {
        pcb_t* exit_child = (pcb_t*)pid2pcb(pcb->info.child_process[i]);
        if (exit_child->stat == ZOMBIE) { return exit_child; }
    }
    return NULL;
}

static void remove_zombie_child(u32 pid) {
    pcb_t* pcb    = &p_proc_current->pcb;
    bool   cpyflg = false;
    for (int i = 0; i < pcb->info.child_p_num; ++i) {
        pcb_t* exit_child = (pcb_t*)pid2pcb(pcb->info.child_process[i]);
        assert(!(cpyflg && exit_child->pid == pid));
        if (exit_child->pid == pid) { cpyflg = true; }
        if (cpyflg == true && i < pcb->info.child_p_num - 1) {
            pcb->info.child_process[i] = pcb->info.child_process[i + 1];
        }
    }
    assert(cpyflg);
    --pcb->info.child_p_num;
    return;
}

static void wait_recycle_part(u32 pid, u32 base, u32 limit, bool free) {
    u32 cr3   = ((pcb_t*)pid2pcb(pid))->cr3;
    u32 laddr = base;
    u32 parts = 0;
    while (laddr < limit) {
        parts++;
        bool ok = pg_unmap_laddr(cr3, laddr, free);
        assert(ok);
        laddr = pg_frame_phyaddr(laddr) + num_4K;
    }
}

static void wait_recycle_memory(u32 recy_pid) {
    assert(recy_pid != p_proc_current->pcb.pid);
    pcb_t*      recy_pcb = (pcb_t*)pid2pcb(recy_pid);
    LIN_MEMMAP* memmap   = &recy_pcb->memmap;
    PH_INFO*    ph_ptr   = memmap->ph_info;
    disable_int();
    while (ph_ptr != NULL) {
        wait_recycle_part(
            recy_pid, ph_ptr->lin_addr_base, ph_ptr->lin_addr_limit, true);
        PH_INFO* old_ph_ptr = ph_ptr;
        ph_ptr              = ph_ptr->next;
        do_free((void*)K_LIN2PHY((u32)old_ph_ptr));
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

static void wait_reset_child(u32 pid) {
    pcb_t* recy_pcb = (pcb_t*)pid2pcb(pid);
    disable_int();
    strcpy(recy_pcb->p_name, "USER");
    recy_pcb->pid    = pid;
    recy_pcb->p_lock = 0;
    memcpy(
        &recy_pcb->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
    recy_pcb->ldts[0].attr1 = DA_C | PRIVILEGE_USER << 5;
    memcpy(
        &recy_pcb->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
    recy_pcb->ldts[1].attr1 = DA_DRW | PRIVILEGE_USER << 5;
    recy_pcb->regs.cs =
        ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
    recy_pcb->regs.ds =
        ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
    recy_pcb->regs.es =
        ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
    recy_pcb->regs.fs =
        ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
    recy_pcb->regs.ss =
        ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
    recy_pcb->regs.gs     = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_USER;
    recy_pcb->regs.eflags = 0x0202;

    //! FIXME: rewrite antihuman expresssions as below
    char* p_regs = (char*)((PROCESS*)recy_pcb + 1);
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

int do_wait(int* wstatus) {
    pcb_t* fa_pcb = &p_proc_current->pcb;
    while (1) {
        lock_or_schedule(&p_proc_current->pcb.p_lock);
        if (fa_pcb->info.child_p_num == 0) {
            if (wstatus != NULL) { *wstatus = 0; }
            p_proc_current->pcb.p_lock = 0;
            return -1;
        }
        pcb_t* exit_pcb = try_get_zombie_child();
        if (exit_pcb == NULL) {
            fa_pcb->stat = SLEEPING;
            release(&fa_pcb->p_lock);
            sched();
            continue;
        }
        lock_or_schedule(&exit_pcb->p_lock);
        remove_zombie_child(exit_pcb->pid);
        if (wstatus != NULL) { *wstatus = exit_pcb->p_exitcode; }
        //! FIXME: no thread release here
        wait_recycle_memory(exit_pcb->pid);
        wait_reset_child(exit_pcb->pid);
        int pid = exit_pcb->pid;
        --u_proc_sum;
        exit_pcb->p_lock = 0;
        fa_pcb->p_lock   = 0;
        return pid;
    }
}
