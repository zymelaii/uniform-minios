#pragma once

#include <unios/fs_misc.h>
#include <unios/protect.h>
#include <unios/layout.h>
#include <unios/memory.h>
#include <unios/regs.h>
#include <unios/sync.h>
#include <sys/types.h>
#include <stdint.h>

#define DEFAULT_STACK_SIZE (8 * NUM_1K)

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
    uint32_t gs;         //<! ━┓
    uint32_t fs;         //<!  ┃
    uint32_t es;         //<!  ┃
    uint32_t ds;         //<!  ┃
    uint32_t edi;        //<!  ┃
    uint32_t esi;        //<!  ┣━┫ push by `save`
    uint32_t ebp;        //<!  ┃
    uint32_t kernel_esp; //<!  ┣ ignored by popad
    uint32_t ebx;        //<!  ┃
    uint32_t edx;        //<!  ┃
    uint32_t ecx;        //<!  ┃
    uint32_t eax;        //<! ━┫
    uint32_t retaddr;    //<!  ┣ retaddr for `save`
    uint32_t eip;        //<! ━┫
    uint32_t cs;         //<!  ┃
    uint32_t eflags;     //<!  ┣━┫ push by interrupt
    uint32_t esp;        //<!  ┃
    uint32_t ss;         //<! ━┛
} stack_frame_t;

typedef struct tree_info_s {
    int      type;                        //<! type of task
    uint32_t real_ppid;                   //<! pid of creator task
    uint32_t ppid;                        //<! pid of current parent task
    uint32_t child_p_num;                 //<! total child proc
    uint32_t child_process[NR_CHILD_MAX]; //<! child proc list
    uint32_t child_t_num;                 //<! total child thread
    uint32_t child_thread[NR_CHILD_MAX];  //<! child thread list
    uint32_t child_k_num;                 //<! total killed child proc/thread
    uint32_t child_killed[NR_CHILD_MAX];  //<! child killed list
    int      text_hold;                   //<! owner of text or not
    int      data_hold;                   //<! owner of data or not
} tree_info_t;

typedef struct ph_info_s {
    uint32_t          base;
    uint32_t          limit;
    struct ph_info_s* next;
    struct ph_info_s* before;
} ph_info_t;

typedef struct lin_memmap_s {
    ph_info_t* ph_info;
    //! heap
    uint32_t heap_lin_base;
    uint32_t heap_lin_limit;
    //! kernel space
    uint32_t kernel_lin_base;
    uint32_t kernel_lin_limit;
    //! where to store exec argv & envp
    uint32_t arg_lin_base;
    uint32_t arg_lin_limit;
    //! stack
    uint32_t stack_lin_base;
    uint32_t stack_lin_limit;
    //! stack limit for child thread
    uint32_t stack_child_limit;
} lin_memmap_t;

typedef struct pcb_s {
    //! WARNING: offset 0 is reserved for user context regs
    stack_frame_t regs;

    uint16_t     ldt_sel;
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

    uint32_t pid;
    char     name[16];

    int                 stat;
    uint32_t            cr3;
    memblk_allocator_t* allocator;
    uint32_t            heap_lock;

    file_desc_t* filp[NR_FILES];
    uint32_t     lock;
    uint32_t     exit_code;
} pcb_t;

typedef union {
    pcb_t   pcb;
    uint8_t stack[DEFAULT_STACK_SIZE];
    //! NOTE: only for convenient access
    uint8_t       stack_top[0];
    stack_frame_t frame_top[0];
} process_t;

typedef struct task_s {
    task_handler_t entry_point;
    char           name[32];
} task_t;

bool init_locked_pcb(
    process_t* proc, const char* name, void* entry_point, uint32_t rpl);
process_t* try_lock_free_pcb();
ph_info_t* clone_ph_info(ph_info_t* src);
int        ldt_seg_linear(process_t* p, int idx);
void*      va2la(int pid, void* va);
process_t* pid2proc(int pid);
int        proc2pid(process_t* proc);

//! ATTENTION: only called in a restart restore routine
void wakeup_exclusive(void* channel);

extern tss_t      tss;
extern process_t* p_proc_current;
extern process_t* p_proc_next;
extern process_t* proc_table[];
extern task_t     task_table[];
extern rwlock_t   proc_table_rwlock;
