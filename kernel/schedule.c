#include <unios/schedule.h>
#include <unios/proc.h>
#include <stddef.h>

phyaddr_t cr3_ready;

void switch_pde() {
    cr3_ready = p_proc_current->pcb.cr3;
}

void cherry_pick_next_ready_proc() {
    process_t* proc = p_proc_current;
    while (true) {
        ++proc;
        if (proc >= proc_table + NR_PCBS) { proc = proc_table; }
        if (proc->pcb.stat == READY && proc->pcb.live_ticks > 0) {
            p_proc_next = proc;
            break;
        }
        if (proc == p_proc_current && p_proc_current) {
            for (proc = proc_table; proc < proc_table + NR_PCBS; ++proc) {
                proc->pcb.live_ticks = proc->pcb.priority;
            }
        }
    }
}
