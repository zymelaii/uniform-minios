#include <unios/proc.h>
#include <unios/vga.h>
#include <unios/kstate.h>
#include <unios/memory.h>
#include <unios/clock.h>
#include <unios/keyboard.h>
#include <unios/hd.h>
#include <unios/schedule.h>
#include <unios/vfs.h>
#include <unios/fs.h>
#include <arch/x86.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern void clear_kernel_pagepte_low();
extern void initial();

void init_startup_proc() {
    int total_init_pcbs = NR_TASKS + 1;
    rwlock_wait_wr(&proc_table_rwlock);
    for (int i = 0; i < total_init_pcbs; ++i) {
        process_t *proc = kmalloc(sizeof(process_t));
        assert(proc != NULL);
        memset(proc, 0, sizeof(process_t));
        proc->pcb.stat = IDLE;
        proc_table[i]  = proc;
    }
    rwlock_leave(&proc_table_rwlock);

    int index = 0;

    //! init tasks
    while (index < NR_TASKS) {
        bool ok = init_proc_pcb(
            proc_table[index],
            task_table[index].name,
            task_table[index].initial_eip,
            RPL_TASK);
        assert(ok);
        ++index;
    }

    //! init initial program
    bool ok = init_proc_pcb(proc_table[index], "initial", initial, RPL_TASK);
    assert(ok);

    //! NOTE: clock interrupt might come immediately as soon as the proc runs,
    //! that may lead to a unexpected schedule to the first proc, so add some
    //! more ticks to ensure the first proc run its user code firstly
    assert(proc_table[0] != NULL);
    ++proc_table[0]->pcb.live_ticks;
}

int kernel_main() {
    vga_clear_screen();

    clear_kernel_pagepte_low();
    klog("-----Kernel Initialization Begins-----");
    kstate_on_init = true;

    init_memory();
    klog("-----mem module init done-----");

    init_startup_proc();

    //! record nest level of only interruption
    kstate_reenter_cntr = 0;

    //! WARNING: important assignment! do not remove this!
    p_proc_current = cpu_table;

    init_sysclk();   //<! system clock
    init_keyboard(); //<! keyboard service
    init_hd();       //<! hd rdwt service

    //! enable ints to allow retrive infos from devices
    enable_int();
    hd_open(PRIMARY_MASTER);
    vfs_setup_and_init();
    init_fs();
    init_fs_fat();
    disable_int();

    klog("-----Processes Begin-----");
    p_proc_current = proc_table[0];
    assert(p_proc_current != NULL);
    assert(p_proc_current->pcb.stat == READY);
    kstate_on_init = false;

    restart_initial();
    unreachable();
}
