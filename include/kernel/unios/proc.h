#pragma once

#include <unios/fs_misc.h>
#include <unios/protect.h>
#include <unios/layout.h>
#include <unios/memory.h>
#include <unios/sync.h>
#include <sys/types.h>
#include <stdint.h>

// new kernel stack is 8 KB
#define INIT_STACK_SIZE (8 * NUM_1K)
#define STACK_SIZE_TASK NUM_4K

#define NR_GSREG        0
#define NR_FSREG        1
#define NR_ESREG        2
#define NR_DSREG        3
#define NR_EDIREG       4
#define NR_ESIREG       5
#define NR_EBPREG       6
#define NR_KERNELESPREG 7
#define NR_EBXREG       8
#define NR_EDXREG       9
#define NR_ECXREG       10
#define NR_EAXREG       11
#define NR_RETADR       12
#define NR_EIPREG       13
#define NR_CSREG        14
#define NR_EFLAGSREG    15
#define NR_ESPREG       16
#define NR_SSREG        17

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

//! see https://en.wikipedia.org/wiki/FLAGS_register
#define EFLAGS_RESERVED 0x0002              //<! always 1 in eflags
#define EFLAGS_IF       0x0200              //<! interrupt enable flag
#define EFLAGS_IOPL(pl) (((pl)&0b11) << 12) //<! I/O privilege level

#define NR_PCBS      64 //<! total pcbs
#define NR_TASKS     2  //<! predefined task k-pcbs
#define NR_K_PCBS    2  //<! reserved k-pcbs, only predefined tasks currently
#define NR_RECY_PROC 1  //<! no. of recycler proc `scanvenger`

#define NR_FILES 64

enum process_stat {
    IDLE,      //<! idle pcb
    READY,     //<! ready to be scheduled
    SLEEPING,  //<! sleeping
    KILLED,    //<! killed for any reasons
    ZOMBIE,    //<! exit and become a zombie proc, wait for recycle
    KILLING,   //<! being killed
    PREINITED, //<! already inited and wait to be a ready one
};

#define NR_CHILD_MAX (NR_PCBS - NR_K_PCBS - 1)
#define TYPE_PROCESS 0
#define TYPE_THREAD  1

typedef struct stack_frame_s {
    u32 gs;         //<! ━┓
    u32 fs;         //<!  ┃
    u32 es;         //<!  ┃
    u32 ds;         //<!  ┃
    u32 edi;        //<!  ┃
    u32 esi;        //<!  ┣━┫ push by `save`
    u32 ebp;        //<!  ┃
    u32 kernel_esp; //<!  ┣ ignored by popad
    u32 ebx;        //<!  ┃
    u32 edx;        //<!  ┃
    u32 ecx;        //<!  ┃
    u32 eax;        //<! ━┫
    u32 retaddr;    //<!  ┣ retaddr for `save`
    u32 eip;        //<! ━┫
    u32 cs;         //<!  ┃
    u32 eflags;     //<!  ┣━┫ push by interrupt
    u32 esp;        //<!  ┃
    u32 ss;         //<! ━┛
} stack_frame_t;

typedef struct tree_info_s {
    int type;                        //<! type of task
    u32 real_ppid;                   //<! pid of creator task
    u32 ppid;                        //<! pid of current parent task
    u32 child_p_num;                 //<! total child proc
    u32 child_process[NR_CHILD_MAX]; //<! child proc list
    u32 child_t_num;                 //<! total child thread
    u32 child_thread[NR_CHILD_MAX];  //<! child thread list
    u32 child_k_num;                 //<! total killed child proc/thread
    u32 child_killed[NR_CHILD_MAX];  //<! child killed list
    int text_hold;                   //<! owner of text or not
    int data_hold;                   //<! owner of data or not
} tree_info_t;

typedef struct ph_info_s {
    u32               base;
    u32               limit;
    struct ph_info_s* next;
    struct ph_info_s* before;
} ph_info_t;

typedef struct lin_memmap_s {
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

    u16          ldt_sel;
    descriptor_t ldts[LDT_SIZE];

    char* esp_save_int;
    char* esp_save_syscall;
    char* esp_save_context;

    //! if non-zero, sleeping on channel, which is a pointer of the target field
    //! for example, as for syscall sleep(int n), the target field is
    //! 'system_ticks', and the channel is a pointer of 'system_ticks'.
    void*        channel;
    lin_memmap_t memmap;

    tree_info_t tree_info;
    int         live_ticks;
    int         priority;

    u32  pid;
    char name[16];

    enum process_stat   stat;
    u32                 cr3;
    memblk_allocator_t* allocator;
    u32                 heap_lock;

    file_desc_t* filp[NR_FILES];
    u32          lock;
    u32          exit_code;
} pcb_t;

typedef union {
    pcb_t pcb;
    char  stack[INIT_STACK_SIZE / sizeof(char)];
} process_t;

typedef struct task_s {
    task_handler_t initial_eip;
    int            stacksize;
    char           name[32];
} task_t;

bool init_locked_pcb(
    process_t* proc, const char* name, void* entry_point, u32 rpl);
process_t* try_lock_free_pcb();
ph_info_t* clone_ph_info(ph_info_t* src);
int        ldt_seg_linear(process_t* p, int idx);
void*      va2la(int pid, void* va);
process_t* pid2proc(int pid);
int        proc2pid(process_t* proc);

extern tss_t      tss;
extern process_t* p_proc_current;
extern process_t* p_proc_next;
extern process_t* proc_table[];
extern task_t     task_table[];
extern rwlock_t   proc_table_rwlock;
