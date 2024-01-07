#include <unios/scedule.h>
#include <unios/proc.h>
#include <stddef.h>

phyaddr_t cr3_ready;

void switch_pde() {
    cr3_ready = p_proc_current->pcb.cr3;
}

void cherry_pick_next_ready_proc() {
    process_t* proc           = NULL;
    int        greatest_ticks = 0;

    if (p_proc_current->pcb.stat == READY
        && p_proc_current->pcb.live_ticks > 0) {
        p_proc_next = p_proc_current;
        return;
    }

    while (!greatest_ticks) {
        for (proc = proc_table; proc < proc_table + NR_PCBS; proc++) {
            if (proc->pcb.stat == READY
                && proc->pcb.live_ticks > greatest_ticks) {
                greatest_ticks = proc->pcb.live_ticks;
                p_proc_next    = proc;
            }
        }

        if (!greatest_ticks) {
            for (proc = proc_table; proc < proc_table + NR_PCBS; proc++) {
                proc->pcb.live_ticks = proc->pcb.priority;
            }
        }
    }
}
