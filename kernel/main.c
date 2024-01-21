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

int kernel_main() {
    //! ATTENTION: ints is disabled through the whole `kernel_main`

    vga_clear_screen();
    clear_kernel_pagepte_low();
    kdebug("clean-up stuffs from loader");

    kstate_on_init = true;
    kdebug("init kernel");

    init_memory();
    kdebug("init memory done");

    process_t *proc = try_lock_free_pcb();
    assert(proc != NULL);
    bool ok = init_locked_pcb(proc, "init", initial, RPL_TASK);
    assert(ok);
    for (int i = 0; i < NR_TASKS; ++i) {
        process_t *proc = try_lock_free_pcb();
        assert(proc != NULL);
        bool ok = init_locked_pcb(
            proc, task_table[i].name, task_table[i].initial_eip, RPL_TASK);
        assert(ok);
        //! NOTE: mark as pre-inited, enable it in the `init` proc later
        proc->pcb.stat = PREINITED;
    }
    kdebug("init startup proc done");

    init_sysclk();   //<! system clock
    init_keyboard(); //<! keyboard service
    init_hd();       //<! hd rdwt service
    kdebug("init device done");

    vfs_setup_and_init();
    kdebug("init vfs done");

    kstate_reenter_cntr = 0;
    kstate_on_init      = false;
    kdebug("init kernel done");

    p_proc_current = proc_table[0];
    restart_initial();
    unreachable();
}
