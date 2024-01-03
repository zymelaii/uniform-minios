#include <unios/const.h>
#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/global.h>
#include <unios/proto.h>
#include <string.h>

/*======================================================================*
                              schedule
 *======================================================================*/
void schedule() {
    PROCESS* p;
    int      greatest_ticks = 0;

    // Added by xw, 18/4/21
    if (p_proc_current->pcb.stat == READY && p_proc_current->pcb.ticks > 0) {
        p_proc_next = p_proc_current; // added by xw, 18/4/26
        return;
    }

    while (!greatest_ticks) {
        for (p = proc_table; p < proc_table + NR_PCBS;
             p++) // edit by visual 2016.4.5
        {
            if (p->pcb.stat == READY
                && p->pcb.ticks > greatest_ticks) // edit by visual 2016.4.5
            {
                greatest_ticks = p->pcb.ticks;
                // p_proc_current = p;
                p_proc_next = p; // modified by xw, 18/4/26
            }
        }

        if (!greatest_ticks) {
            for (p = proc_table; p < proc_table + NR_PCBS;
                 p++) // edit by visual 2016.4.5
            {
                p->pcb.ticks = p->pcb.priority;
            }
        }
    }
}

/*======================================================================*
                           alloc_pcb  add by visual 2016.4.8
 *======================================================================*/
PROCESS* alloc_pcb() { // 分配PCB表
    PROCESS* p;
    int      i;
    p = proc_table + NR_K_PCBS; // 跳过前NR_K_PCBS个
    for (i = NR_K_PCBS; i < NR_PCBS; i++) {
        if (p->pcb.stat == IDLE) break;
        p++;
    }
    if (i >= NR_PCBS)
        return 0; // NULL
    else
        return p;
}

void free_pcb(PROCESS* p) {
    p->pcb.stat = IDLE;
}

void do_yield() {
    p_proc_current->pcb.ticks = 0;
    sched();
}

void do_sleep(int n) {
    int ticks0;

    ticks0                      = ticks;
    p_proc_current->pcb.channel = &ticks;

    while (ticks - ticks0 < n) {
        p_proc_current->pcb.stat = SLEEPING;
        //		save_context();
        sched(); // Modified by xw, 18/4/19
    }
}

int do_get_pid() {
    return p_proc_current->pcb.pid;
}

/*invoked by clock-interrupt handler to wakeup
 *processes sleeping on ticks.
 */
void do_wakeup(void* channel) {
    PROCESS* p;

    for (p = proc_table; p < proc_table + NR_PCBS; p++) {
        if (p->pcb.stat == SLEEPING && p->pcb.channel == channel) {
            p->pcb.stat = READY;
        }
    }
}

// added by zcr
int ldt_seg_linear(PROCESS* p, int idx) {
    struct s_descriptor* d = &p->pcb.ldts[idx];
    return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}

void* va2la(int pid, void* va) {
    if (kernel_initial == 1) { return va; }

    PROCESS* p        = &proc_table[pid];
    u32      seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
    u32      la       = seg_base + (u32)va;

    return (void*)la;
}

PROCESS* pid2proc(int pid) {
    return &proc_table[pid];
}

int proc2pid(PROCESS* proc) {
    return proc - proc_table;
}
