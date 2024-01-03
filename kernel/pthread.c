#include <unios/const.h>
#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/global.h>
#include <unios/proto.h>
#include <unios/page.h>
#include <string.h>
#include <stdint.h>

static int pthread_pcb_cpy(PROCESS *p_child, PROCESS *p_parent);
static int pthread_update_info(PROCESS *p_child, PROCESS *p_parent);
static int pthread_stack_init(PROCESS *p_child, PROCESS *p_parent);
static int pthread_heap_init(PROCESS *p_child, PROCESS *p_parent);

int do_pthread(void *entry) {
    PROCESS *p_child;

    char *p_reg;
    p_child = alloc_pcb();
    if (0 == p_child) {
        vga_write_str_color("PCB NULL,pthread faild!", 0x74);
        return -1;
    } else {
        PROCESS *p_parent;
        if (p_proc_current->pcb.info.type == TYPE_THREAD) {          // 线程
            p_parent = &(proc_table[p_proc_current->pcb.info.ppid]); // 父进程
        } else {                                                     // 进程
            p_parent = p_proc_current; // 父进程就是父线程
        }
        /************复制父进程的PCB部分内容（保留了自己的标识信息,但cr3使用的是父进程的）**************/
        pthread_pcb_cpy(p_child, p_parent);

        /************在父进程的栈中分配子线程的栈（从进程栈的低地址分配8M,注意方向）**********************/
        pthread_stack_init(p_child, p_parent);

        /**************初始化子线程的堆（此时的这两个变量已经变成了指针）***********************/
        pthread_heap_init(p_child, p_parent);

        /********************设置线程的执行入口**********************************************/
        p_child->pcb.regs.eip                   = (u32)entry;
        p_reg                                   = (char *)(p_child + 1);
        *((u32 *)(p_reg + EIPREG - P_STACKTOP)) = p_child->pcb.regs.eip;

        pthread_update_info(p_child, p_parent);

        strcpy(p_child->pcb.p_name, "pthread"); // 所有的子进程都叫pthread

        p_child->pcb.regs.eax                   = 0; // return child with 0
        p_reg                                   = (char *)(p_child + 1);
        *((u32 *)(p_reg + EAXREG - P_STACKTOP)) = p_child->pcb.regs.eax;

        /****************用户进程数+1****************************/
        u_proc_sum += 1;

        vga_write_str_color("[pthread success:", 0x72);
        vga_write_str_color(p_proc_current->pcb.p_name, 0x72);
        vga_write_str_color("]", 0x72);

        // anything child need is prepared now, set its state to ready. added by
        // xw, 17/12/11
        p_child->pcb.stat = READY;
    }
    return p_child->pcb.pid;
}

/**********************************************************
 *		pthread_pcb_cpy			//add by visual 2016.5.26
 *复制父进程PCB表，但是又马上恢复了子进程的标识信息
 *************************************************************/
static int pthread_pcb_cpy(PROCESS *p_child, PROCESS *p_parent) {
    int   pid;
    u32   eflags, selector_ldt;
    char *p_reg; // point to a register in the new kernel stack, added by xw,
                 // 17/12/11
    char *esp_save_int, *esp_save_context; // use to save corresponding field in
                                           // child's PCB, xw, 18/4/21

    // 暂存标识信息
    pid = p_child->pcb.pid;

    // eflags = p_child->pcb.regs.eflags; //deleted by xw, 17/12/11
    p_reg  = (char *)(p_child + 1);
    eflags = *((u32 *)(p_reg + EFLAGSREG - P_STACKTOP)); // added by xw,
                                                         // 17/12/11

    selector_ldt = p_child->pcb.ldt_sel;

    // 复制PCB内容
    // modified by xw, 17/12/11
    // modified begin
    //*p_child = *p_parent;

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
    *((u32 *)(p_reg + EFLAGSREG - P_STACKTOP)) = eflags; // added by xw,
                                                         // 17/12/11

    p_child->pcb.ldt_sel = selector_ldt;
    return 0;
}

/**********************************************************
 *		pthread_update_info			//add by visual 2016.5.26
 *更新父进程和子线程程的进程树标识info
 *************************************************************/
static int pthread_update_info(PROCESS *p_child, PROCESS *p_parent) {
    /************更新父进程的info***************/ // 注意 父进程 父进程 父进程
    if (p_parent != p_proc_current) { // 只有在线程创建线程的时候才会执行
                                      // ，p_parent事实上是父进程
        p_parent->pcb.info.child_t_num += 1; // 子线程数量
        p_parent->pcb.info.child_thread[p_parent->pcb.info.child_t_num - 1] =
            p_child->pcb.pid; // 子线程列表
    }
    /************更新父线程的info**************/
    // p_proc_current->pcb.info.type;		//当前是进程还是线程
    // p_proc_current->pcb.info.real_ppid;  //亲父进程，创建它的那个进程
    // p_proc_current->pcb.info.ppid;		//当前父进程
    // p_proc_current->pcb.info.child_p_num += 1; //子进程数量
    // p_proc_current->pcb.info.child_process[p_proc_current->pcb.info.child_p_num-1]
    // = p_child->pcb.pid;//子进程列表
    p_proc_current->pcb.info.child_t_num += 1; // 子线程数量
    p_proc_current->pcb.info
        .child_thread[p_proc_current->pcb.info.child_t_num - 1] =
        p_child->pcb.pid; // 子线程列表
    // p_proc_current->pcb.text_hold;			//是否拥有代码
    // p_proc_current->pcb.data_hold;			//是否拥有数据

    /************更新子线程的info***************/
    p_child->pcb.info.type = TYPE_THREAD; // 这是一个线程
    p_child->pcb.info.real_ppid =
        p_proc_current->pcb
            .pid; // 亲父进程，创建它的那个线程，注意，这个是创建它的那个线程p_proc_current
    p_child->pcb.info.ppid        = p_parent->pcb.pid; // 当前父进程
    p_child->pcb.info.child_p_num = 0;                 // 子进程数量
    // p_child->pcb.info.child_process[NR_CHILD_MAX] = pid;//子进程列表
    p_child->pcb.info.child_t_num = 0; // 子线程数量
    // p_child->pcb.info.child_thread[NR_CHILD_MAX];//子线程列表
    p_child->pcb.info.text_hold = 0; // 是否拥有代码，子进程不拥有代码
    p_child->pcb.info.data_hold = 0; // 是否拥有数据，子进程拥有数据

    return 0;
}

/**********************************************************
 *		pthread_stack_init			//add by visual 2016.5.26
 *申请子线程的栈，并重置其esp
 *************************************************************/
static int pthread_stack_init(PROCESS *p_child, PROCESS *p_parent) {
    int   addr_lin;
    char *p_reg; // point to a register in the new kernel stack, added by xw,
                 // 17/12/11

    p_child->pcb.memmap.stack_lin_limit =
        p_parent->pcb.memmap.stack_child_limit;       // 子线程的栈界
    p_parent->pcb.memmap.stack_child_limit += 0x4000; // 分配16K
    p_child->pcb.memmap.stack_lin_base =
        p_parent->pcb.memmap.stack_child_limit - num_4B; // 子线程的基址

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

/**********************************************************
 *		pthread_stack_init			//add by visual 2016.5.26
 *子线程使用父进程的堆
 *************************************************************/
static int pthread_heap_init(PROCESS *p_child, PROCESS *p_parent) {
    p_child->pcb.memmap.heap_lin_base =
        (u32) & (p_parent->pcb.memmap.heap_lin_base);
    p_child->pcb.memmap.heap_lin_limit =
        (u32) & (p_parent->pcb.memmap.heap_lin_limit);
    return 0;
}
