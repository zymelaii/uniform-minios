
#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/global.h>
#include <unios/proto.h>
#include <string.h>

tss_t    tss;
PROCESS* p_proc_current;
PROCESS* p_proc_next;

PROCESS cpu_table[NR_CPUS];
PROCESS proc_table[NR_PCBS];

#define TASK_ENTRY(handler) \
 { handler, STACK_SIZE_TASK, #handler }

TASK task_table[NR_TASKS] = {
    TASK_ENTRY(hd_service),
    TASK_ENTRY(task_tty),
    TASK_ENTRY(sweeper),
};

void schedule() {
    PROCESS* p;
    int      greatest_ticks = 0;

    if (p_proc_current->pcb.stat == READY
        && p_proc_current->pcb.live_ticks > 0) {
        p_proc_next = p_proc_current;
        return;
    }

    while (!greatest_ticks) {
        for (p = proc_table; p < proc_table + NR_PCBS; p++) {
            if (p->pcb.stat == READY && p->pcb.live_ticks > greatest_ticks) {
                greatest_ticks = p->pcb.live_ticks;
                p_proc_next    = p;
            }
        }

        if (!greatest_ticks) {
            for (p = proc_table; p < proc_table + NR_PCBS; p++) {
                p->pcb.live_ticks = p->pcb.priority;
            }
        }
    }
}

PROCESS* alloc_pcb() {
    PROCESS* p = proc_table + NR_K_PCBS;
    for (int i = NR_K_PCBS; i < NR_PCBS; i++) {
        if (p->pcb.stat == IDLE) { return p; }
        p++;
    }
    return NULL;
}

void free_pcb(PROCESS* p) {
    p->pcb.stat = IDLE;
}

void do_yield() {
    p_proc_current->pcb.live_ticks = 0;
    sched();
}

void do_sleep(int n) {
    int ticks0                  = ticks;
    p_proc_current->pcb.channel = &ticks;
    while (ticks - ticks0 < n) {
        p_proc_current->pcb.stat = SLEEPING;
        sched();
    }
}

int do_get_pid() {
    return p_proc_current->pcb.pid;
}

void do_wakeup(void* channel) {
    for (PROCESS* p = proc_table; p < proc_table + NR_PCBS; p++) {
        if (p->pcb.stat == SLEEPING && p->pcb.channel == channel) {
            p->pcb.stat = READY;
        }
    }
}

int ldt_seg_linear(PROCESS* p, int idx) {
    descriptor_t* d = &p->pcb.ldts[idx];
    return d->base2 << 24 | d->base1 << 16 | d->base0;
}

void* va2la(int pid, void* va) {
    if (kernel_initial == 1) { return va; }
    PROCESS* p        = &proc_table[pid];
    u32      seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
    u32      la       = seg_base + (u32)va;
    return (void*)la;
}

PROCESS* pid2pcb(int pid) {
    return &proc_table[pid];
}

int proc2pid(PROCESS* proc) {
    return proc - proc_table;
}
