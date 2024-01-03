#include <unios/syscall.h>
#include <unios/global.h>

void clock_handler(int irq) {
    ++ticks;

    /* There is two stages - in kernel intializing or in process running.
     * Some operation shouldn't be valid in kernel intializing stage.
     * added by xw, 18/6/1
     */
    if (kernel_initial == 1) { return; }
    irq = 0;
    --p_proc_current->pcb.ticks;
    do_wakeup(&ticks);
}

int do_get_ticks() {
    return ticks;
}
