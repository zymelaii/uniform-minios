#include <unios/schedule.h>
#include <unios/proc.h>
#include <stddef.h>

phyaddr_t cr3_ready;

void switch_pde() {
    cr3_ready = p_proc_current->pcb.cr3;
}

void cherry_pick_next_ready_proc() {
    //! FIXME: index=-1 means proc comes from a temporary pcb
    int index = proc2pid(p_proc_current);
    while (true) {
        index           = (index + 1) % NR_PCBS;
        process_t *proc = proc_table[index];
        if (proc == NULL) { continue; }
        if (proc->pcb.stat == READY && proc->pcb.live_ticks > 0) {
            p_proc_next = proc;
            break;
        }
        if (proc != p_proc_current) { continue; }
        for (int i = 0; i < NR_PCBS; ++i) {
            if (proc_table[i] == NULL) { continue; }
            proc_table[i]->pcb.live_ticks = proc_table[i]->pcb.priority;
        }
    }
}
