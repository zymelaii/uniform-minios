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
#include <unios/assert.h>

extern void init();

void kernel_main() {
    //! ATTENTION: ints is disabled through the whole `kernel_main`

    //! TODO: maybe we can print our logo here?
    vga_set_cursor_visible_unsafe(false);

    kstate_on_init = true;
    kdebug("init kernel");

    init_memory();
    kinfo("init memory done");

    process_t *proc = try_lock_free_pcb();
    assert(proc != NULL);
    bool ok = init_locked_pcb(proc, "init", init, RPL_TASK);
    assert(ok);
    for (int i = 0; i < NR_TASKS; ++i) {
        process_t *proc = try_lock_free_pcb();
        assert(proc != NULL);
        bool ok = init_locked_pcb(
            proc, task_table[i].name, task_table[i].entry_point, RPL_TASK);
        assert(ok);
        //! NOTE: mark as pre-inited, enable it in the `init` proc later
        proc->pcb.stat = PREINITED;
    }
    kinfo("init startup proc done");

    init_sysclk();   //<! system clock
    init_keyboard(); //<! keyboard service
    init_hd();       //<! hd rdwt service
    kinfo("init device done");

    vfs_setup_and_init();
    kinfo("init vfs done");

    kstate_reenter_cntr = 0;
    kstate_on_init      = false;
    kinfo("init kernel done");

    p_proc_current = proc_table[0];
    restart_initial();
    unreachable();
}
