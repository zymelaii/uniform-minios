
#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/page.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int pthread_pcb_cpy(process_t *p_child, process_t *p_parent);
static int pthread_update_info(process_t *p_child, process_t *p_parent);
static int pthread_stack_init(process_t *p_child, process_t *p_parent);
static int pthread_heap_init(process_t *p_child, process_t *p_parent);

int do_pthread_create(void *entry) {
    process_t *p_child;

    char *p_reg;
    p_child = alloc_pcb();
    if (0 == p_child) {
        klog("PCB NULL,pthread faild!");
        return -1;
    } else {
        process_t *p_parent;
        if (p_proc_current->pcb.tree_info.type == TYPE_THREAD) { // 线程
            p_parent = pid2pcb(p_proc_current->pcb.tree_info.real_ppid);
            p_parent =
                &(proc_table[p_proc_current->pcb.tree_info.ppid]); // 父进程
        } else {                                                   // 进程
            p_parent = p_proc_current; // 父进程就是父线程
        }
        pthread_pcb_cpy(p_child, p_parent);
        pthread_stack_init(p_child, p_parent);
        pthread_heap_init(p_child, p_parent);

        /********************设置线程的执行入口**********************************************/
        p_child->pcb.regs.eip                   = (u32)entry;
        p_reg                                   = (char *)(p_child + 1);
        *((u32 *)(p_reg + EIPREG - P_STACKTOP)) = p_child->pcb.regs.eip;

        pthread_update_info(p_child, p_parent);

        strcpy(p_child->pcb.name, "pthread"); // 所有的子进程都叫pthread

        p_child->pcb.regs.eax                   = 0; // return child with 0
        p_reg                                   = (char *)(p_child + 1);
        *((u32 *)(p_reg + EAXREG - P_STACKTOP)) = p_child->pcb.regs.eax;

        p_child->pcb.stat = READY;
    }
    return p_child->pcb.pid;
}

static int pthread_pcb_cpy(process_t *p_child, process_t *p_parent) {
    int   pid;
    u32   eflags, selector_ldt;
    char *p_reg; // point to a register in the new kernel stack
    char *esp_save_int, *esp_save_context; // use to save corresponding field in
                                           // child's PCB

    // 暂存标识信息
    pid          = p_child->pcb.pid;
    p_reg        = (char *)(p_child + 1);
    eflags       = *((u32 *)(p_reg + EFLAGSREG - P_STACKTOP));
    selector_ldt = p_child->pcb.ldt_sel;

    // esp_save_int and esp_save_context must be saved, because the child and
    // the parent use different kernel stack! And these two are importent to the
    // child's initial running. Added by xw, 18/4/21
    esp_save_int     = p_child->pcb.esp_save_int;
    esp_save_context = p_child->pcb.esp_save_context;
    p_child->pcb     = p_parent->pcb;
    // note that syscalls can be interrupted now! the state of child can only be
    // setted READY when anything else is well prepared. if an interruption
    // happens right here, an error will still occur.
    p_child->pcb.stat = IDLE;
    p_child->pcb.esp_save_int =
        esp_save_int; // esp_save_int of child must be restored!!
    p_child->pcb.esp_save_context = esp_save_context; // same above
    memcpy(
        ((char *)(p_child + 1) - P_STACKTOP),
        ((char *)(p_parent + 1) - P_STACKTOP),
        18 * 4);
    // modified end

    // 恢复标识信息
    p_child->pcb.pid = pid;

    // p_child->pcb.regs.eflags = eflags;
    p_reg                                      = (char *)(p_child + 1);
    *((u32 *)(p_reg + EFLAGSREG - P_STACKTOP)) = eflags;
    p_child->pcb.ldt_sel                       = selector_ldt;
    return 0;
}

static int pthread_update_info(process_t *p_child, process_t *p_parent) {
    /************更新父进程的info***************/ // 注意 父进程 父进程 父进程
    if (p_parent != p_proc_current) { // 只有在线程创建线程的时候才会执行
                                      // ，p_parent事实上是父进程
        p_parent->pcb.tree_info.child_t_num += 1; // 子线程数量
        p_parent->pcb.tree_info
            .child_thread[p_parent->pcb.tree_info.child_t_num - 1] =
            p_child->pcb.pid; // 子线程列表
    }
    p_proc_current->pcb.tree_info.child_t_num += 1; // 子线程数量
    p_proc_current->pcb.tree_info
        .child_thread[p_proc_current->pcb.tree_info.child_t_num - 1] =
        p_child->pcb.pid;
    p_child->pcb.tree_info.type        = TYPE_THREAD; // 这是一个线程
    p_child->pcb.tree_info.real_ppid   = p_proc_current->pcb.pid;
    p_child->pcb.tree_info.ppid        = p_parent->pcb.pid;
    p_child->pcb.tree_info.child_p_num = 0;
    p_child->pcb.tree_info.child_t_num = 0;
    p_child->pcb.tree_info.text_hold   = 0;
    p_child->pcb.tree_info.data_hold   = 0;

    return 0;
}

static int pthread_stack_init(process_t *p_child, process_t *p_parent) {
    int   addr_lin;
    char *p_reg; // point to a register in the new kernel stack, added by xw,

    p_child->pcb.memmap.stack_lin_limit =
        p_parent->pcb.memmap.stack_child_limit;       // 子线程的栈界
    p_parent->pcb.memmap.stack_child_limit += 0x4000; // 分配16K
    p_child->pcb.memmap.stack_lin_base =
        p_parent->pcb.memmap.stack_child_limit; // 子线程的基址
    pg_map_laddr_range(
        p_child->pcb.cr3,
        p_child->pcb.memmap.stack_lin_limit,
        p_child->pcb.memmap.stack_lin_base,
        PG_P | PG_U | PG_RWX,
        PG_P | PG_U | PG_RWX);

    p_child->pcb.regs.esp = p_child->pcb.memmap.stack_lin_base; // 调整esp
    p_reg                 = (char *)(p_child + 1);
    *((u32 *)(p_reg + ESPREG - P_STACKTOP)) = p_child->pcb.regs.esp;

    return 0;
}

static int pthread_heap_init(process_t *p_child, process_t *p_parent) {
    p_child->pcb.memmap.heap_lin_base =
        (u32) & (p_parent->pcb.memmap.heap_lin_base);
    p_child->pcb.memmap.heap_lin_limit =
        (u32) & (p_parent->pcb.memmap.heap_lin_limit);
    return 0;
}
