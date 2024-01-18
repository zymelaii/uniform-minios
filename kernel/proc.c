#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/clock.h>
#include <unios/tty.h>
#include <unios/hd.h>
#include <unios/scavenger.h>
#include <unios/schedule.h>
#include <unios/kstate.h>
#include <unios/memory.h>
#include <unios/layout.h>
#include <unios/syscall.h>
#include <unios/assert.h>
#include <unios/page.h>
#include <string.h>
#include <atomic.h>

tss_t      tss;
process_t* p_proc_current;
process_t* p_proc_next;
process_t  cpu_table[NR_CPUS];
process_t* proc_table[NR_PCBS];
rwlock_t   proc_table_rwlock;

#define TASK_ENTRY(handler) \
 { handler, STACK_SIZE_TASK, #handler }

task_t task_table[NR_TASKS] = {
    TASK_ENTRY(tty_handler),
    TASK_ENTRY(scavenger),
};

process_t* try_lock_free_pcb() {
    bool have_free_slot = false;
    rwlock_wait_rd(&proc_table_rwlock);
    for (int i = 0; i < NR_PCBS; ++i) {
        if (proc_table[i] == NULL) {
            have_free_slot = true;
            continue;
        }
        pcb_t* pcb = &proc_table[i]->pcb;
        if (!try_lock(&pcb->lock)) { continue; }
        if (pcb->stat != IDLE) {
            release(&pcb->lock);
            continue;
        }
        rwlock_leave(&proc_table_rwlock);
        return (process_t*)pcb;
    }
    rwlock_leave(&proc_table_rwlock);
    if (!have_free_slot) { return NULL; }
    rwlock_wait_wr(&proc_table_rwlock);
    process_t* proc = NULL;
    for (int i = 0; i < NR_PCBS; ++i) {
        if (proc_table[i] == NULL) {
            process_t* proc = kmalloc(sizeof(process_t));
            assert(proc != NULL);
            memset(proc, 0, sizeof(process_t));
            proc->pcb.stat = IDLE;
            proc_table[i]  = proc;
            rwlock_leave(&proc_table_rwlock);
            return proc;
        }
    }
    rwlock_leave(&proc_table_rwlock);
    return NULL;
}

ph_info_t* clone_ph_info(ph_info_t* src) {
    ph_info_t* dst = NULL;
    while (src != NULL) {
        ph_info_t* node = kmalloc(sizeof(ph_info_t));
        assert(node != NULL);
        node->base  = src->base;
        node->limit = src->limit;
        if (dst == NULL) {
            node->next   = NULL;
            node->before = NULL;
            dst          = node;
        } else {
            dst->before  = node;
            node->next   = dst;
            node->before = NULL;
            dst          = node;
        }
        src = src->next;
    }
    return dst;
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

int do_get_ppid() {
    return p_proc_current->pcb.tree_info.ppid;
}

void do_wakeup(void* channel) {
    rwlock_wait_rd(&proc_table_rwlock);
    for (int i = 0; i < NR_PCBS; ++i) {
        process_t* proc = proc_table[i];
        if (proc == NULL) { continue; }
        if (proc->pcb.stat == SLEEPING && proc->pcb.channel == channel) {
            proc->pcb.stat = READY;
        }
    }
    rwlock_leave(&proc_table_rwlock);
}

int ldt_seg_linear(process_t* p, int idx) {
    descriptor_t* d = &p->pcb.ldts[idx];
    return d->base2 << 24 | d->base1 << 16 | d->base0;
}

void* va2la(int pid, void* va) {
    if (kstate_on_init) { return va; }
    process_t* proc = proc_table[pid];
    assert(proc != NULL);
    u32 seg_base = ldt_seg_linear(proc, INDEX_LDT_RW);
    u32 la       = seg_base + (u32)va;
    return (void*)la;
}

process_t* pid2proc(int pid) {
    if (pid == -1) { return cpu_table; }
    return proc_table[pid];
}

int proc2pid(process_t* proc) {
    assert(proc != NULL);
    rwlock_wait_rd(&proc_table_rwlock);
    for (int i = 0; i < NR_PCBS; ++i) {
        if (proc == proc_table[i]) {
            rwlock_leave(&proc_table_rwlock);
            return i;
        }
    }
    rwlock_leave(&proc_table_rwlock);
    if (proc == cpu_table) { return -1; }
    unreachable();
}

bool init_proc_pcb(
    process_t* proc, const char* name, void* entry_point, u32 rpl) {
    assert(proc != NULL);
    assert(name != NULL);
    assert(proc->pcb.stat == IDLE);
    assert(!proc->pcb.lock);

    pcb_t*        pcb  = &proc->pcb;
    lin_memmap_t* mmap = &pcb->memmap;
    if (!try_lock(&pcb->lock)) { return false; }

    //! FIXME: better pid & ldt sel assignment method
    int index = proc2pid(proc);

    //! basic info
    strcpy(pcb->name, name);
    pcb->exit_code  = 0;
    pcb->pid        = index;
    pcb->priority   = 4;
    pcb->live_ticks = pcb->priority;

    //! ldt selector
    pcb->ldt_sel = SELECTOR_LDT_FIRST + (index << 3);
    memcpy(&pcb->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(descriptor_t));
    memcpy(&pcb->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(descriptor_t));
    pcb->ldts[0].attr0 = DA_C | (rpl << 5);
    pcb->ldts[1].attr0 = DA_DRW | (rpl << 5);
    init_descriptor(
        &gdt[pcb->ldt_sel >> 3],
        vir2phys(seg2phys(SELECTOR_KERNEL_DS), pcb->ldts),
        LDT_SIZE * sizeof(descriptor_t) - 1,
        DA_LDT);

    //! memory
    bool ok = pg_create_and_init(&pcb->cr3);
    if (!ok) { return false; }
    mmap->kernel_lin_base   = KernelLinBase;
    mmap->kernel_lin_limit  = KernelLinBase + KernelSize;
    mmap->arg_lin_base      = ArgLinBase;
    mmap->arg_lin_limit     = ArgLinLimitMAX;
    mmap->stack_lin_base    = StackLinBase;
    mmap->stack_lin_limit   = StackLinBase - 0x4000;
    mmap->stack_child_limit = StackLinLimitMAX;
    mmap->heap_lin_base     = HeapLinBase;
    mmap->heap_lin_limit    = HeapLinBase;
    ok                      = pg_map_laddr_range(
        pcb->cr3,
        mmap->stack_lin_limit,
        mmap->stack_lin_base,
        PG_P | PG_U | PG_RWX,
        PG_P | PG_U | PG_RWX);
    assert(ok);
    //! FIXME: better heap manager
    pcb->allocator = mballoc_create(
        kmalloc, NUM_4K, (void*)mmap->heap_lin_base, (void*)HeapLinLimitMAX);
    pcb->heap_lock = 0;

    //! user space context
    memset(&pcb->regs, 0, P_STACKTOP);
    pcb->regs.cs  = ((8 * 0) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | rpl;
    pcb->regs.ds  = ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | rpl;
    pcb->regs.es  = ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | rpl;
    pcb->regs.fs  = ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | rpl;
    pcb->regs.ss  = ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | rpl;
    pcb->regs.gs  = (SELECTOR_KERNEL_GS & SA_MASK_RPL) | rpl;
    pcb->regs.esp = mmap->stack_lin_base;
    pcb->regs.eip = (u32)entry_point;

    //! NOTE: assign task proc iopl=1, user's iopl=0
    pcb->regs.eflags =
        EFLAGS_RESERVED | EFLAGS_IF | EFLAGS_IOPL(rpl == RPL_USER ? 0 : 1);

    //! kernel space stack frame
    u32* stack = (void*)(proc + 1);
    u32* frame = (void*)stack - P_STACKTOP;
    memcpy(frame, &pcb->regs, P_STACKTOP);

    pcb->esp_save_context = (void*)(frame - 10); //<! popad + popfd + ret
    memset(pcb->esp_save_context, 0, sizeof(u32) * 10);
    frame[-1] = (u32)restart_restore; //<! ret -> retaddr
    //! NOTE: proc inited here always start from kernel space, so allow iopl=1
    //! here
    frame[-2] =
        EFLAGS_RESERVED | EFLAGS_IF | EFLAGS_IOPL(1); //<! popfd -> eflags

    pcb->esp_save_int     = (void*)frame;
    pcb->esp_save_syscall = (void*)stack;

    //! family tree
    pcb->tree_info.type = TYPE_PROCESS;
    //! FIXME: semantics of pid=-1 are unclear
    pcb->tree_info.real_ppid   = -1;
    pcb->tree_info.ppid        = -1;
    pcb->tree_info.child_p_num = 0;
    pcb->tree_info.child_t_num = 0;
    pcb->tree_info.child_k_num = 0;
    pcb->tree_info.text_hold   = true;
    pcb->tree_info.data_hold   = true;

    //! done
    pcb->stat = READY;
    release(&pcb->lock);
    return true;
}
