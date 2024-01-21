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
#include <unios/tracing.h>
#include <unios/page.h>
#include <unios/layout.h>
#include <unios/interrupt.h>
#include <arch/x86.h>
#include <assert.h>
#include <string.h>

extern void initial();

void clear_kernel_pagepte_low() {
    u32 page_num = *(u32 *)PageTblNumAddr;
    u32 phyaddr  = KernelPageTblAddr;
    memset(K_PHY2LIN(phyaddr), 0, sizeof(u32) * page_num);
    memset(K_PHY2LIN(phyaddr + NUM_4K), 0, NUM_4K * page_num);
    pg_refresh();
}

void init_startup_proc() {
    int total_init_pcbs = NR_TASKS + 1;
    for (int i = 0; i < total_init_pcbs; ++i) {
        process_t *proc = kmalloc(sizeof(process_t));
        assert(proc != NULL);
        memset(proc, 0, sizeof(process_t));
        proc->pcb.stat = IDLE;
        proc_table[i]  = proc;
    }

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
    kdebug("clean-up stuffs from loader");

    kdebug("init kernel");
    kstate_on_init = true;

    init_memory();
    kdebug("init memory done");

    init_startup_proc();
    kdebug("init startup proc done");

    //! record nest level of only interruption
    kstate_reenter_cntr = 0;

    //! WARNING: important assignment! do not remove this!
    p_proc_current = cpu_table;

    init_sysclk();   //<! system clock
    init_keyboard(); //<! keyboard service
    init_hd();       //<! hd rdwt service
    kdebug("init device done");

    vfs_setup_and_init();
    kdebug("init vfs done");

    //! FIXME: `schedule` and `restart_restore` in the `kernel_main` stage is a
    //! risky op, but they are always executed from the interrupt return. while
    //! the current disk io depends on both cascade and AT winchester disk IRQs,
    //! a better solution is expected in future versions

    enable_int();
    hd_open(PRIMARY_MASTER);
    kdebug("init hd done");
    init_fs();
    kdebug("init fs done");
    disable_int();

    p_proc_current = proc_table[0];
    assert(p_proc_current != NULL);
    assert(p_proc_current->pcb.stat == READY);
    kstate_on_init = false;

    kdebug("init kernel done");

    enable_irq(CLOCK_IRQ);

    restart_initial();
    unreachable();
}
