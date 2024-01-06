#include <unios/vfs.h>
#include <unios/proto.h>
#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/page.h>
#include <unios/malloc.h>
#include <unios/hd.h>
#include <unios/global.h>
#include <unios/fs.h>
#include <unios/interrupt.h>
#include <unios/vga.h>
#include <unios/fs_const.h>
#include <unios/fat32.h>
#include <unios/keyboard.h>
#include <unios/console.h>
#include <unios/assert.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <arch/x86.h>

/*************************************************************************
return 0 if there is no error, or return -1.
added by xw, 18/6/2
***************************************************************************/
static int initialize_cpus() {
    // just use the fields of struct PCB in cpu_table, we needn't initialize
    // something at present.
    return 0;
}

/*************************************************************************
进程初始化部分
return 0 if there is no error, or return -1.
moved from kernel_main() by xw, 18/5/26
***************************************************************************/
static int initialize_processes() {
    TASK*    p_task       = task_table;
    PROCESS* p_proc       = proc_table;
    u16      selector_ldt = SELECTOR_LDT_FIRST;
    char*    p_regs; // point to registers in the new kernel stack, added by xw,
                     // 17/12/11
    task_handler_t eip_context; // a funtion pointer, added by xw, 18/4/18
    /*************************************************************************
     *进程初始化部分 	edit by visual 2016.5.4
     ***************************************************************************/
    int pid;
    u32 AddrLin, err_temp; // edit by visual 2016.5.9

    /* set common fields in PCB. added by xw, 18/5/25 */
    p_proc = proc_table;
    for (pid = 0; pid < NR_PCBS; pid++) {
        // some operations
        p_proc++;
    }

    p_proc = proc_table;
    for (pid = 0; pid < NR_TASKS;
         pid++) { // 1>对前NR_TASKS个PCB初始化,且状态为READY(生成的进程)
        /*************基本信息*********************************/
        strcpy(p_proc->pcb.name, p_task->name); // 名称
        p_proc->pcb.pid    = pid;               // pid
        p_proc->pcb.stat   = READY;             // 状态
        p_proc->pcb.p_lock = 0;
        /**************LDT*********************************/
        p_proc->pcb.ldt_sel = selector_ldt;
        memcpy(
            &p_proc->pcb.ldts[0],
            &gdt[SELECTOR_KERNEL_CS >> 3],
            sizeof(descriptor_t));
        p_proc->pcb.ldts[0].attr0 = DA_C | RPL_TASK << 5;
        memcpy(
            &p_proc->pcb.ldts[1],
            &gdt[SELECTOR_KERNEL_DS >> 3],
            sizeof(descriptor_t));
        p_proc->pcb.ldts[1].attr0 = DA_DRW | RPL_TASK << 5;

        /**************寄存器初值**********************************/
        p_proc->pcb.regs.cs =
            ((8 * 0) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.ds =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.es =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.fs =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.ss =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.gs     = (SELECTOR_KERNEL_GS & SA_MASK_RPL) | RPL_TASK;
        p_proc->pcb.regs.eflags = 0x1202; /* IF=1, IOPL=1 */
        // p_proc->pcb.cr3 在页表初始化中处理

        /**************线性地址布局初始化**********************************/ //	add by visual 2016.5.4
        /**************task的代码数据大小及位置暂时是不会用到的，所以没有初始化************************************/
        p_proc->pcb.memmap.heap_lin_base = HeapLinBase;
        p_proc->pcb.memmap.heap_lin_limit =
            HeapLinBase; // 堆的界限将会一直动态变化

        p_proc->pcb.memmap.stack_child_limit =
            StackLinLimitMAX; // add by visual 2016.5.27
        p_proc->pcb.memmap.stack_lin_base = StackLinBase;
        p_proc->pcb.memmap.stack_lin_limit =
            StackLinBase
            - 0x4000; // 栈的界限将会一直动态变化，目前赋值为16k，这个值会根据esp的位置进行调整，目前初始化为16K大小

        p_proc->pcb.memmap.kernel_lin_base = KernelLinBase;
        p_proc->pcb.memmap.kernel_lin_limit =
            KernelLinBase
            + KernelSize; // 内核大小初始化为8M		//add  by visual 2016.5.10

        /***************初始化PID进程页表*****************************/
        p_proc->pcb.cr3 = pg_create_and_init();

        //	pde_addr_phy_temp = get_pde_phy_addr(pid);//获取该进程页目录物理地址
        ////delete by visual 2016.5.19

        /****************代码数据*****************************/
        p_proc->pcb.regs.eip =
            (u32)p_task
                ->initial_eip; // 进程入口线性地址		edit by visual 2016.5.4

        /****************栈（此时堆、栈已经区分，以后实验会重新规划堆的位置）*****************************/
        p_proc->pcb.regs.esp = (u32)StackLinBase;

        bool ok = pg_map_laddr_range(
            p_proc->pcb.cr3,
            p_proc->pcb.memmap.stack_lin_limit,
            p_proc->pcb.memmap.stack_lin_base,
            PG_P | PG_U | PG_RWX,
            PG_P | PG_U | PG_RWX);
        assert(ok);

        /***************copy registers data to kernel
         * stack****************************/
        // copy registers data to the bottom of the new kernel stack
        // added by xw, 17/12/11
        p_regs = (char*)(p_proc + 1);
        p_regs -= P_STACKTOP;
        memcpy(p_regs, (char*)p_proc, 18 * 4);

        /***************some field about process
         * switch****************************/
        p_proc->pcb.esp_save_int =
            p_regs; // initialize esp_save_int, added by xw, 17/12/11
        // p_proc->pcb.save_type = 1;
        p_proc->pcb.esp_save_context =
            p_regs
            - 10 * 4; // when the process is chosen to run for the first time,
                      // sched() will fetch value from esp_save_context
        eip_context = restart_restore;
        *(u32*)(p_regs - 4) =
            (u32)eip_context; // initialize EIP in the context, so the process
                              // can start run. added by xw, 18/4/18
        *(u32*)(p_regs - 8) = 0x1202; // initialize EFLAGS in the context, IF=1,
                                      // IOPL=1. xw, 18/4/20

        /***************变量调整****************************/
        p_proc++;
        p_task++;
        selector_ldt += 1 << 3;
    }
    for (
        ; pid < NR_K_PCBS;
        pid++) { // 2>对中NR_TASKS~NR_K_PCBS的PCB表初始化,状态为IDLE,没有初始化esp(并没有生成,所以没有代码入口,只是留位置)
        /*************基本信息*********************************/
        strcpy(p_proc->pcb.name, "Task"); // 名称
        p_proc->pcb.pid    = pid;         // pid
        p_proc->pcb.p_lock = 0;
        p_proc->pcb.stat   = IDLE; // 状态

        /**************LDT*********************************/
        p_proc->pcb.ldt_sel = selector_ldt;
        memcpy(
            &p_proc->pcb.ldts[0],
            &gdt[SELECTOR_KERNEL_CS >> 3],
            sizeof(descriptor_t));
        p_proc->pcb.ldts[0].attr0 = DA_C | RPL_TASK << 5;
        memcpy(
            &p_proc->pcb.ldts[1],
            &gdt[SELECTOR_KERNEL_DS >> 3],
            sizeof(descriptor_t));
        p_proc->pcb.ldts[1].attr0 = DA_DRW | RPL_TASK << 5;

        /**************寄存器初值**********************************/
        p_proc->pcb.regs.cs =
            ((8 * 0) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.ds =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.es =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.fs =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.ss =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.gs     = (SELECTOR_KERNEL_GS & SA_MASK_RPL) | RPL_TASK;
        p_proc->pcb.regs.eflags = 0x1202; /* IF=1, IOPL=1 */

        /****************页表、代码数据、堆栈*****************************/
        // 无

        /***************copy registers data to kernel
         * stack****************************/
        // copy registers data to the bottom of the new kernel stack
        // added by xw, 17/12/11
        p_regs = (char*)(p_proc + 1);
        p_regs -= P_STACKTOP;
        memcpy(p_regs, (char*)p_proc, 18 * 4);

        /***************some field about process
         * switch****************************/
        p_proc->pcb.esp_save_int =
            p_regs; // initialize esp_save_int, added by xw, 17/12/11
        // p_proc->pcb.save_type = 1;
        p_proc->pcb.esp_save_context =
            p_regs
            - 10 * 4; // when the process is chosen to run for the first time,
                      // sched() will fetch value from esp_save_context
        eip_context = restart_restore;
        *(u32*)(p_regs - 4) =
            (u32)eip_context; // initialize EIP in the context, so the process
                              // can start run. added by xw, 18/4/18
        *(u32*)(p_regs - 8) = 0x1202; // initialize EFLAGS in the context, IF=1,
                                      // IOPL=1. xw, 18/4/20

        /***************变量调整****************************/
        p_proc++;
        selector_ldt += 1 << 3;
    }
    for (; pid < NR_K_PCBS + 1; pid++) { // initial 进程的初始化
                                         // //add by visual 2016.5.17
        /*************基本信息*********************************/
        strcpy(p_proc->pcb.name, "initial"); // 名称
        p_proc->pcb.pid    = pid;            // pid
        p_proc->pcb.p_lock = 0;
        p_proc->pcb.stat   = READY; // 状态

        /**************LDT*********************************/
        p_proc->pcb.ldt_sel = selector_ldt;
        memcpy(
            &p_proc->pcb.ldts[0],
            &gdt[SELECTOR_KERNEL_CS >> 3],
            sizeof(descriptor_t));
        p_proc->pcb.ldts[0].attr0 = DA_C | RPL_TASK << 5;
        memcpy(
            &p_proc->pcb.ldts[1],
            &gdt[SELECTOR_KERNEL_DS >> 3],
            sizeof(descriptor_t));
        p_proc->pcb.ldts[1].attr0 = DA_DRW | RPL_TASK << 5;

        /**************寄存器初值**********************************/
        p_proc->pcb.regs.cs =
            ((8 * 0) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.ds =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.es =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.fs =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.ss =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_TASK;
        p_proc->pcb.regs.gs     = (SELECTOR_KERNEL_GS & SA_MASK_RPL) | RPL_TASK;
        p_proc->pcb.regs.eflags = 0x1202; /* IF=1, IOPL=1 */
        // p_proc->pcb.cr3 在页表初始化中处理

        /**************线性地址布局初始化**********************************/ // edit by visual 2016.5.25
        p_proc->pcb.memmap.ph_info         = NULL;
        p_proc->pcb.memmap.vpage_lin_base  = VpageLinBase; // 保留内存基址
        p_proc->pcb.memmap.vpage_lin_limit = VpageLinBase; // 保留内存界限
        p_proc->pcb.memmap.heap_lin_base   = HeapLinBase;  // 堆基址
        p_proc->pcb.memmap.heap_lin_limit  = HeapLinBase;  // 堆界限
        p_proc->pcb.memmap.stack_lin_base  = StackLinBase; // 栈基址
        p_proc->pcb.memmap.stack_lin_limit =
            StackLinBase - 0x4000; // 栈界限（使用时注意栈的生长方向）
        p_proc->pcb.memmap.arg_lin_base    = ArgLinBase; // 参数内存基址
        p_proc->pcb.memmap.arg_lin_limit   = ArgLinBase; // 参数内存界限
        p_proc->pcb.memmap.kernel_lin_base = KernelLinBase; // 内核基址
        p_proc->pcb.memmap.kernel_lin_limit =
            KernelLinBase + KernelSize; // 内核大小初始化为8M

        /*************************进程树信息初始化***************************************/
        p_proc->pcb.tree_info.type = TYPE_PROCESS; // 当前是进程还是线程
        p_proc->pcb.tree_info.real_ppid = -1; // 亲父进程，创建它的那个进程
        p_proc->pcb.tree_info.ppid        = -1; // 当前父进程
        p_proc->pcb.tree_info.child_p_num = 0;  // 子进程数量
        // p_proc->pcb.tree_info.child_process[NR_CHILD_MAX];//子进程列表
        p_proc->pcb.tree_info.child_t_num = 0; // 子线程数量
        // p_proc->pcb.tree_info.child_thread[NR_CHILD_MAX];//子线程列表
        p_proc->pcb.tree_info.text_hold = 1; // 是否拥有代码
        p_proc->pcb.tree_info.data_hold = 1; // 是否拥有数据

        /***************初始化PID进程页表*****************************/
        p_proc->pcb.cr3 = pg_create_and_init();

        // pde_addr_phy_temp = get_pde_phy_addr(pid);//获取该进程页目录物理地址
        // //edit by visual 2016.5.19

        /****************代码数据*****************************/
        p_proc->pcb.regs.eip =
            (u32)initial; // 进程入口线性地址		edit by visual 2016.5.17

        /****************栈（此时堆、栈已经区分，以后实验会重新规划堆的位置）*****************************/
        p_proc->pcb.regs.esp = (u32)StackLinBase; // 栈地址最高处

        bool ok = pg_map_laddr_range(
            p_proc->pcb.cr3,
            p_proc->pcb.memmap.stack_lin_limit,
            p_proc->pcb.memmap.stack_lin_base,
            PG_P | PG_U | PG_RWX,
            PG_P | PG_U | PG_RWX);
        assert(ok);

        /***************copy registers data to kernel
         * stack****************************/
        // copy registers data to the bottom of the new kernel stack
        // added by xw, 17/12/11
        p_regs = (char*)(p_proc + 1);
        p_regs -= P_STACKTOP;
        memcpy(p_regs, (char*)p_proc, 18 * 4);

        /***************some field about process
         * switch****************************/
        p_proc->pcb.esp_save_int =
            p_regs; // initialize esp_save_int, added by xw, 17/12/11
        // p_proc->pcb.save_type = 1;
        p_proc->pcb.esp_save_context =
            p_regs
            - 10 * 4; // when the process is chosen to run for the first time,
                      // sched() will fetch value from esp_save_context
        eip_context = restart_restore;
        *(u32*)(p_regs - 4) =
            (u32)eip_context; // initialize EIP in the context, so the process
                              // can start run. added by xw, 18/4/18
        *(u32*)(p_regs - 8) = 0x1202; // initialize EFLAGS in the context, IF=1,
                                      // IOPL=1. xw, 18/4/20

        /***************变量调整****************************/
        p_proc++;
        selector_ldt += 1 << 3;
    }
    for (
        ; pid < NR_PCBS;
        pid++) { // 3>对后NR_K_PCBS~NR_PCBS的PCB表部分初始化,(名称,pid,stat,LDT选择子),状态为IDLE.
        /*************基本信息*********************************/
        strcpy(p_proc->pcb.name, "USER"); // 名称
        p_proc->pcb.pid    = pid;         // pid
        p_proc->pcb.p_lock = 0;
        p_proc->pcb.stat   = IDLE; // 状态

        /**************LDT*********************************/
        p_proc->pcb.ldt_sel = selector_ldt;
        memcpy(
            &p_proc->pcb.ldts[0],
            &gdt[SELECTOR_KERNEL_CS >> 3],
            sizeof(descriptor_t));
        p_proc->pcb.ldts[0].attr0 = DA_C | RPL_USER << 5;
        memcpy(
            &p_proc->pcb.ldts[1],
            &gdt[SELECTOR_KERNEL_DS >> 3],
            sizeof(descriptor_t));
        p_proc->pcb.ldts[1].attr0 = DA_DRW | RPL_USER << 5;

        /**************寄存器初值**********************************/
        p_proc->pcb.regs.cs =
            ((8 * 0) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
        p_proc->pcb.regs.ds =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
        p_proc->pcb.regs.es =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
        p_proc->pcb.regs.fs =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
        p_proc->pcb.regs.ss =
            ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
        p_proc->pcb.regs.gs     = (SELECTOR_KERNEL_GS & SA_MASK_RPL) | RPL_USER;
        p_proc->pcb.regs.eflags = 0x0202; /* IF=1, 倒数第二位恒为1 */

        /****************页表、代码数据、堆栈*****************************/
        // 无

        /***************copy registers data to kernel
         * stack****************************/
        // copy registers data to the bottom of the new kernel stack
        // added by xw, 17/12/11
        p_regs = (char*)(p_proc + 1);
        p_regs -= P_STACKTOP;
        memcpy(p_regs, (char*)p_proc, 18 * 4);

        /***************some field about process
         * switch****************************/
        p_proc->pcb.esp_save_int =
            p_regs; // initialize esp_save_int, added by xw, 17/12/11
        // p_proc->pcb.save_type = 1;
        p_proc->pcb.esp_save_context =
            p_regs
            - 10 * 4; // when the process is chosen to run for the first time,
                      // sched() will fetch value from esp_save_context
        eip_context = restart_restore;
        *(u32*)(p_regs - 4) =
            (u32)eip_context; // initialize EIP in the context, so the process
                              // can start run. added by xw, 18/4/18
        *(u32*)(p_regs - 8) = 0x1202; // initialize EFLAGS in the context, IF=1,
                                      // IOPL=1. xw, 18/4/20

        /***************变量调整****************************/
        p_proc++;
        selector_ldt += 1 << 3;
    }

    proc_table[0].pcb.live_ticks = proc_table[0].pcb.priority = 1;
    proc_table[1].pcb.live_ticks = proc_table[1].pcb.priority = 1;
    proc_table[2].pcb.live_ticks = proc_table[2].pcb.priority = 1;
    proc_table[3].pcb.live_ticks = proc_table[3].pcb.priority = 1;
    proc_table[4].pcb.live_ticks = proc_table[4].pcb.priority = 1;

    proc_table[NR_K_PCBS].pcb.live_ticks = proc_table[NR_K_PCBS].pcb.priority =
        1;

    /* When the first process begin running, a clock-interruption will happen
     * immediately. If the first process's initial ticks is 1, it won't be the
     * first process to execute its user code. Thus, it's will look weird, for
     * proc_table[0] don't output first. added by xw, 18/4/19
     */
    proc_table[0].pcb.live_ticks = 2;

    return 0;
}

/*======================================================================*
                            kernel_main
 *======================================================================*/
int kernel_main() {
    //! clear screen
    vga_set_disppos(0);
    for (int i = 0; i < SCR_WIDTH * SCR_HEIGHT; ++i) { kprintf(" "); }
    vga_set_disppos(0);

    int error;
    clear_kernel_pagepte_low();
    trace_logging("-----Kernel Initialization Begins-----\n");
    kernel_initial = 1; // kernel is in initial state. added by xw, 18/5/31

    init_mem(); // 内存管理模块的初始化  add by liang
    trace_logging("-----mem module init done-----\n");

    // initialize PCBs, added by xw, 18/5/26
    error = initialize_processes();
    if (error != 0) return error;

    // initialize CPUs, added by xw, 18/6/2
    error = initialize_cpus();
    if (error != 0) return error;

    k_reenter = 0; // record nest level of only interruption! it's different
                   // from Orange's. usage modified by xw
    ticks          = 0; // initialize system-wide ticks
    p_proc_current = cpu_table;

    /************************************************************************
    *device initialization
    added by xw, 18/6/4
    *************************************************************************/
    /* initialize 8253 PIT */
    outb(TIMER_MODE, RATE_GENERATOR);
    outb(TIMER0, (u8)(TIMER_FREQ / HZ));
    outb(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));

    /* initialize clock-irq */
    put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
    enable_irq(CLOCK_IRQ); /* 让8259A可以接收时钟中断 */

    init_keyboard(); // added by mingxuan 2019-5-19

    /* initialize hd-irq and hd rdwt queue */
    init_hd();

    /* enable interrupt, we should read information of some devices by
     * interrupt. Note that you must have initialized all devices ready before
     * you enable interrupt. added by xw
     */
    enable_int();

    /***********************************************************************
    open hard disk and initialize file system
    coded by zcr on 2017.6.10. added by xw, 18/5/31
    ************************************************************************/
    // hd_open(MINOR(ROOT_DEV));
    hd_open(PRIMARY_MASTER); // modified by mingxuan 2020-10-27

    vfs_setup_and_init(); // added by mingxuan 2020-10-30
    init_fs();
    init_fs_fat(); // added by mingxuan 2019-5-17
    // init_vfs();	//added by mingxuan 2019-5-17	//deleted by mingxuan
    // 2020-10-30

    /*************************************************************************
     *第一个进程开始启动执行
     **************************************************************************/
    /* we don't want interrupt happens before processes run.
     * added by xw, 18/5/31
     */
    disable_int();

    trace_logging("-----Processes Begin-----\n");

    /* linear address 0~8M will no longer be mapped to physical address 0~8M.
     * note that disp_xx can't work after this function is invoked until
     * processes runs. add by visual 2016.5.13; moved by xw, 18/5/30
     */

    p_proc_current = proc_table;
    kernel_initial = 0; // kernel initialization is done. added by xw, 18/5/31
    restart_initial();  // modified by xw, 18/4/19

    panic("unreachable");
}
