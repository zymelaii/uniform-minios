
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* Used to find saved registers in the new kernel stack,
 * for there is no variable name you can use in C.
 * The macro defined below must coordinate with the equivalent in sconst.inc,
 * whose content is used in asm.
 * Note that you shouldn't use saved registers in the old
 * kernel stack, i.e. STACK_FRAME, any more.
 * To access them, use a pointer plus a offset defined
 * below instead.
 * added by xw, 17/12/11
 */

#ifndef MINIOS_PROC_H
#define MINIOS_PROC_H

#include "type.h"
#include "protect.h"

#define INIT_STACK_SIZE 1024 * 8 // new kernel stack is 8kB

#define NR_GSREG        0
#define NR_FSREG        (NR_GSREG + 1)
#define NR_ESREG        (NR_FSREG + 1)
#define NR_DSREG        (NR_ESREG + 1)
#define NR_EDIREG       (NR_DSREG + 1)
#define NR_ESIREG       (NR_EDIREG + 1)
#define NR_EBPREG       (NR_ESIREG + 1)
#define NR_KERNELESPREG (NR_EBPREG + 1)
#define NR_EBXREG       (NR_KERNELESPREG + 1)
#define NR_EDXREG       (NR_EBXREG + 1)
#define NR_ECXREG       (NR_EDXREG + 1)
#define NR_EAXREG       (NR_ECXREG + 1)
#define NR_RETADR       (NR_EAXREG + 1)
#define NR_EIPREG       (NR_RETADR + 1)
#define NR_CSREG        (NR_EIPREG + 1)
#define NR_EFLAGSREG    (NR_CSREG + 1)
#define NR_ESPREG       (NR_EFLAGSREG + 1)
#define NR_SSREG        (NR_ESPREG + 1)

#define P_STACKBASE  0
#define GSREG        (NR_GSREG * 4)
#define FSREG        (NR_FSREG * 4)
#define ESREG        (NR_ESREG * 4)
#define DSREG        (NR_DSREG * 4)
#define EDIREG       (NR_EDIREG * 4)
#define ESIREG       (NR_ESIREG * 4)
#define EBPREG       (NR_EBPREG * 4)
#define KERNELESPREG (NR_KERNELESPREG * 4)
#define EBXREG       (NR_EBXREG * 4)
#define EDXREG       (NR_EDXREG * 4)
#define ECXREG       (NR_ECXREG * 4)
#define EAXREG       (NR_EAXREG * 4)
#define RETADR       (NR_RETADR * 4)
#define EIPREG       (NR_EIPREG * 4)
#define CSREG        (NR_CSREG * 4)
#define EFLAGSREG    (NR_EFLAGSREG * 4)
#define ESPREG       (NR_ESPREG * 4)
#define SSREG        (NR_SSREG * 4)
#define P_STACKTOP   (SSREG + 4)

/*总PCB表数和taskPCB表数*/
// modified by xw, 18/8/27
// the memory space we put kernel is 0x30400~0x6ffff, so we should limit kernel
// size
//  #define NR_PCBS	32		//add by visual 2016.4.5
//  #define NR_K_PCBS 10		//add by visual 2016.4.5
#define NR_PCBS 25
// #define NR_TASKS	4	//TestA~TestC + hd_service //deleted by mingxuan
// 2019-5-19
#define NR_TASKS  2 // task_tty + hd_service	//modified by mingxuan 2019-5-19
#define NR_K_PCBS 4 // no K_PCB is empty now

//~xw

#define NR_CPUS  1  // numbers of cpu. added by xw, 18/6/1
#define NR_FILES 64 // numbers of files a process can own. added by xw, 18/6/14

// enum proc_stat	{IDLE,READY,WAITING,RUNNING};		//add by visual smile
// 2016.4.5 enum proc_stat	{IDLE,READY,SLEEPING};		//eliminate RUNNING
// state
enum proc_stat {
    IDLE,
    READY,
    SLEEPING,
    KILLED
}; /* add KILLED state. when a process's state is KILLED, the process
    * won't be scheduled anymore, but all of the resources owned by
    * it is not freed yet.
    * added by xw, 18/12/19
    */

#define NR_CHILD_MAX \
 (NR_PCBS - NR_K_PCBS - 1) // 定义最多子进程/线程数量	//add by visual
                           // 2016.5.26
#define TYPE_PROCESS 0     // 进程//add by visual 2016.5.26
#define TYPE_THREAD  1     // 线程//add by visual 2016.5.26

typedef struct s_stackframe { /* proc_ptr points here				↑ Low
                               */
    u32 gs;                   /* ┓						│			*/
    u32 fs;                   /* ┃						│			*/
    u32 es;                   /* ┃						│			*/
    u32 ds;                   /* ┃						│			*/
    u32 edi;                  /* ┃						│			*/
    u32 esi;                  /* ┣ pushed by save()				│			*/
    u32 ebp;                  /* ┃						│			*/
    u32 kernel_esp;           /* <- 'popad' will ignore it			│			*/
    u32 ebx;                  /* ┃						↑栈从高地址往低地址增长*/
    u32 edx;                  /* ┃						│			*/
    u32 ecx;                  /* ┃						│			*/
    u32 eax;                  /* ┛						│			*/
    u32 retaddr; /* return address for assembly code save()	│			*/
    u32 eip;     /*  ┓						│			*/
    u32 cs;      /*  ┃						│			*/
    u32 eflags;  /*  ┣ these are pushed by CPU during interrupt	│			*/
    u32 esp;     /*  ┃						│			*/
    u32 ss;      /*  ┛						┷High			*/
} STACK_FRAME;

typedef struct s_tree_info { // 进程树记录，包括父进程，子进程，子线程  //add by
                             // visual 2016.5.25
    int type;                        // 当前是进程还是线程
    int real_ppid;                   // 亲父进程，创建它的那个进程
    int ppid;                        // 当前父进程
    int child_p_num;                 // 子进程数量
    int child_process[NR_CHILD_MAX]; // 子进程列表
    int child_t_num;                 // 子线程数量
    int child_thread[NR_CHILD_MAX];  // 子线程列表
    int text_hold;                   // 是否拥有代码
    int data_hold;                   // 是否拥有数据
} TREE_INFO;

typedef struct s_ph_info {
    u32               lin_addr_base;
    u32               lin_addr_limit;
    struct s_ph_info* next;
    struct s_ph_info* before;
} PH_INFO;

typedef struct s_lin_memmap { // 线性地址分布结构体	edit by visual 2016.5.25
    // u32 text_lin_base;        // 代码段基址// BULL SHIT!!!!!
    // u32 text_lin_limit;       // 代码段界限

    // u32 data_lin_base;  // 数据段基址// BULL SHIT!!!!!
    // u32 data_lin_limit; // 数据段界限

    PH_INFO* ph_info;

    u32 vpage_lin_base;  // 保留内存基址
    u32 vpage_lin_limit; // 保留内存界限

    u32 heap_lin_base;  // 堆基址
    u32 heap_lin_limit; // 堆界限

    u32 stack_lin_base;  // 栈基址
    u32 stack_lin_limit; // 栈界限（使用时注意栈的生长方向）

    u32 arg_lin_base;  // 参数内存基址
    u32 arg_lin_limit; // 参数内存界限

    u32 kernel_lin_base;  // 内核基址
    u32 kernel_lin_limit; // 内核界限

    u32 stack_child_limit; // 分给子线程的栈的界限		//add by visual
                           // 2016.5.27
} LIN_MEMMAP;

typedef struct s_proc {
    STACK_FRAME regs; /* process registers saved in stack frame */

    u16        ldt_sel;        /* gdt selector giving ldt base and limit */
    DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data */

    char* esp_save_int; // to save the position of esp in the kernel stack of
                        // the process added by xw, 17/12/11
    char* esp_save_syscall; // to save the position of esp in the kernel stack
                            // of the process
    char* esp_save_context; // to save the position of esp in the kernel stack
                            // of the process
    //	int   save_type;		//the cause of process losting CPU	//save_type
    // is not needed any more, xw, 18/4/20 1st-bit for interruption, 2nd-bit for
    // context, 3rd-bit for syscall
    void* channel; /*if non-zero, sleeping on channel, which is a pointer of the
                   target field for example, as for syscall sleep(int n), the
                   target field is 'ticks', and the channel is a pointer of
                   'ticks'.
                   */

    LIN_MEMMAP memmap; // 线性内存分部信息 		add by visual 2016.5.4 //
                       // BULLSHIT FUCK IDIOT
    TREE_INFO info;    // 记录进程树关系	add by visual 2016.5.25
    int       ticks;   /* remained ticks */
    int       priority;

    u32  pid;        /* process id passed in from MM */
    char p_name[16]; /* name of the process */

    enum proc_stat stat; // add by visual 2016.4.5

    u32 cr3; // add by visual 2016.4.5

    // added by zcr
    struct file_desc* filp[NR_FILES];
    //~zcr
} PROCESS_0;

// new PROCESS struct with PCB and process's kernel stack
// added by xw, 17/12/11
typedef union task_union {
    PROCESS_0 task;
    char      stack[INIT_STACK_SIZE / sizeof(char)];
} PROCESS;

typedef struct s_task {
    task_f initial_eip;
    int    stacksize;
    char   name[32];
} TASK;

#define STACK_SIZE_TASK 0x1000 // add by visual 2016.4.5
// #define STACK_SIZE_TOTAL	(STACK_SIZE_TASK*NR_PCBS)	//edit by visual
// 2016.4.5

// added by zcr
#define proc2pid(x) (x - proc_table)

#endif
