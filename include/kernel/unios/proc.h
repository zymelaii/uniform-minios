#pragma once

#include <unios/fs_misc.h>
#include <sys/types.h>
#include <stdint.h>
#include "protect.h"

// new kernel stack is 8kB
#define INIT_STACK_SIZE 1024 * 8

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

#define NR_PCBS      20
#define NR_TASKS     3
#define NR_K_PCBS    5
#define NR_RECY_PROC 2

#define NR_CPUS  1
#define NR_FILES 64

enum process_stat {
    IDLE,
    READY,
    SLEEPING,
    KILLED,
    ZOMBIE
}; /* add KILLED state. when a process's state is KILLED, the process
    * won't be scheduled anymore, but all of the resources owned by
    * it is not freed yet.
    * added by xw, 18/12/19
    */

#define NR_CHILD_MAX (NR_PCBS - NR_K_PCBS - 1)
#define TYPE_PROCESS 0
#define TYPE_THREAD  1

typedef struct stackframe_s { /*      proc_ptr points here      ↑          Low*/
    u32 gs;  /* ┓                              │                       */
    u32 fs;  /* ┃                              │                       */
    u32 es;  /* ┃                              │                       */
    u32 ds;  /* ┃                              │                       */
    u32 edi; /* ┃                              │                       */
    u32 esi; /* ┣  pushed by save()            │                       */
    u32 ebp; /* ┃                              │                       */
    u32 kernel_esp; /* <- popad will ignore it │                       */
    u32 ebx; /* ┃                              ↑  栈从高地址往低地址增长 */
    u32 edx; /* ┃                              │                       */
    u32 ecx; /* ┃                              │                       */
    u32 eax; /* ┛                              │                       */
    u32 retaddr; /* return address for assembly code save()     │      */
    u32 eip;     /*  ┓                                          │      */
    u32 cs;      /*  ┃                                          │      */
    u32 eflags;  /*  ┣ these are pushed by CPU during interrupt │      */
    u32 esp;     /*  ┃                                          │      */
    u32 ss;      /*  ┛                                          ┷High  */
} stack_frame_t;

typedef struct tree_info_s {
    int type;                        //<! type of task
    u32 real_ppid;                   //<! pid of creator task
    u32 ppid;                        //<! pid of current parent task
    u32 child_p_num;                 //<! total child proc
    u32 child_process[NR_CHILD_MAX]; //<! child proc list
    u32 child_t_num;                 //<! total child thread
    u32 child_thread[NR_CHILD_MAX];  //<! child thread list
    int text_hold;                   //<! owner of text or not
    int data_hold;                   //<! owner of data or not
} tree_info_t;

typedef struct ph_info_s {
    u32               base;
    u32               limit;
    struct ph_info_s* next;
    struct ph_info_s* before;
} ph_info_t;

typedef struct lin_memmap_s { // 线性地址分布结构体	edit by visual 2016.5.25
    ph_info_t* ph_info;
    //! stack limit for child thread
    u32 stack_child_limit;
    //! reserved
    u32 vpage_lin_base;
    u32 vpage_lin_limit;
    //! heap
    u32 heap_lin_base;
    u32 heap_lin_limit;
    //! stack
    u32 stack_lin_base;
    u32 stack_lin_limit;
    //! where to store exec argv & envp
    u32 arg_lin_base;
    u32 arg_lin_limit;
    //! kernel space
    u32 kernel_lin_base;
    u32 kernel_lin_limit;
} lin_memmap_t;

typedef struct pcb_s {
    //! WARNING: offset 0 is reserved for user context regs
    stack_frame_t regs;

    u16          ldt_sel;        /* gdt selector giving ldt base and limit */
    descriptor_t ldts[LDT_SIZE]; /* local descriptors for code and data */

    char* esp_save_int;
    char* esp_save_syscall;
    char* esp_save_context;

    //! if non-zero, sleeping on channel, which is a pointer of the target field
    //! for example, as for syscall sleep(int n), the target field is 'ticks',
    //! and the channel is a pointer of 'ticks'.
    void*        channel;
    lin_memmap_t memmap;

    tree_info_t tree_info;
    int         live_ticks;
    int         priority;

    u32  pid;
    char name[16];

    enum process_stat stat;
    u32               cr3;

    struct file_desc* filp[NR_FILES];
    u32               p_lock;
    u32               p_exitcode;
} pcb_t;

// new PROCESS struct with PCB and process's kernel stack
typedef union task_union {
    pcb_t pcb;
    char  stack[INIT_STACK_SIZE / sizeof(char)];
} PROCESS;

typedef struct task_s {
    task_handler_t initial_eip;
    int            stacksize;
    char           name[32];
} TASK;

#define STACK_SIZE_TASK 0x1000

extern tss_t    tss;
extern PROCESS* p_proc_current;
extern PROCESS* p_proc_next;
extern PROCESS  cpu_table[];
extern PROCESS  proc_table[];
extern TASK     task_table[];

PROCESS* pid2pcb(int pid);
int      proc2pid(PROCESS* proc);
