#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/vga.h>
#include <unios/kstate.h>
#include <unios/malloc.h>
#include <unios/clock.h>
#include <unios/keyboard.h>
#include <unios/hd.h>
#include <unios/schedule.h>
#include <unios/vfs.h>
#include <unios/fs.h>
#include <arch/x86.h>
#include <stdio.h>
#include <string.h>

extern void clear_kernel_pagepte_low();
extern void initial();

void init_startup_proc() {
    for (int i = 0; i < NR_PCBS; ++i) {
        memset(&proc_table[i], 0, sizeof(process_t));
        proc_table[i].pcb.stat = IDLE;
    }

    int index = 0;

    //! init tasks
    while (index < NR_TASKS) {
        bool ok = init_proc_pcb(
            &proc_table[index],
            task_table[index].name,
            task_table[index].initial_eip,
            RPL_TASK);
        assert(ok);
        ++index;
    }

    //! init initial program
    bool ok = init_proc_pcb(&proc_table[index], "initial", initial, RPL_TASK);
    assert(ok);

    //! NOTE: clock interrupt might come immediately as soon as the proc runs,
    //! that may lead to a unexpected schedule to the first proc, so add some
    //! more ticks to ensure the first proc run its user code firstly
    ++proc_table[0].pcb.live_ticks;
}

int kernel_main() {
    //! clear screen
    vga_set_disppos(0);
    for (int i = 0; i < SCR_HEIGHT; ++i) { vga_flush_blankline(i); }
    vga_set_disppos(0);

    int error;
    clear_kernel_pagepte_low();
    klog("-----Kernel Initialization Begins-----\n");
    kstate_on_init = true;

    init_mem();
    klog("-----mem module init done-----\n");

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

    klog("-----Processes Begin-----\n");
    p_proc_current = proc_table;
    kstate_on_init = false;

    restart_initial();
    panic("unreachable");
}
