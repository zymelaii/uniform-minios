
#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/clock.h>
#include <unios/tty.h>
#include <unios/hd.h>
#include <unios/sweeper.h>
#include <unios/scedule.h>
#include <unios/kstate.h>
#include <string.h>

tss_t      tss;
process_t* p_proc_current;
process_t* p_proc_next;
process_t  cpu_table[NR_CPUS];
process_t  proc_table[NR_PCBS];

#define TASK_ENTRY(handler) \
 { handler, STACK_SIZE_TASK, #handler }

task_t task_table[NR_TASKS] = {
    TASK_ENTRY(hd_service),
    TASK_ENTRY(task_tty),
    TASK_ENTRY(task_sweeper),
};

process_t* alloc_pcb() {
    process_t* p = proc_table + NR_K_PCBS;
    for (int i = NR_K_PCBS; i < NR_PCBS; i++) {
        if (p->pcb.stat == IDLE) { return p; }
        p++;
    }
    return NULL;
}

void free_pcb(process_t* p) {
    p->pcb.stat = IDLE;
}

void do_yield() {
    p_proc_current->pcb.live_ticks = 0;
    schedule();
}

void do_sleep(int n) {
    int ticks0                  = system_ticks;
    p_proc_current->pcb.channel = &system_ticks;
    while (system_ticks - ticks0 < n) {
        p_proc_current->pcb.stat = SLEEPING;
        schedule();
    }
}

int do_get_pid() {
    return p_proc_current->pcb.pid;
}

void do_wakeup(void* channel) {
    for (process_t* p = proc_table; p < proc_table + NR_PCBS; p++) {
        if (p->pcb.stat == SLEEPING && p->pcb.channel == channel) {
            p->pcb.stat = READY;
        }
    }
}

int ldt_seg_linear(process_t* p, int idx) {
    descriptor_t* d = &p->pcb.ldts[idx];
    return d->base2 << 24 | d->base1 << 16 | d->base0;
}

void* va2la(int pid, void* va) {
    if (kstate_on_init) { return va; }
    process_t* p        = &proc_table[pid];
    u32        seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
    u32        la       = seg_base + (u32)va;
    return (void*)la;
}

process_t* pid2pcb(int pid) {
    return &proc_table[pid];
}

int proc2pid(process_t* proc) {
    return proc - proc_table;
}
