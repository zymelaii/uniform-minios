#include <unios/vfs.h>
#include <unios/const.h>
#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/proto.h>
#include <unios/fs_const.h>
#include <unios/hd.h>
#include <stdint.h>

int        kernel_initial;
int        ticks;
u8         gdt_ptr[6];
DESCRIPTOR gdt[GDT_SIZE];
u8         idt_ptr[6];
GATE       idt[IDT_SIZE];
u32        k_reenter;
int        u_proc_sum; //<! current total proc num
TSS        tss;
PROCESS   *p_proc_current;
PROCESS   *p_proc_next;

u32 cr3_ready;

/* save the execution environment of each cpu, which doesn't belong to any
 * process. added by xw, 18/6/1
 */
PROCESS cpu_table[NR_CPUS];
PROCESS proc_table[NR_PCBS];

#define TASK_ENTRY(handler) \
 { handler, STACK_SIZE_TASK, #handler }
TASK task_table[NR_TASKS] = {
    TASK_ENTRY(hd_service),
    TASK_ENTRY(task_tty),
    TASK_ENTRY(sweeper),
};

irq_handler_t irq_table[NR_IRQ];

tty_t   tty_table[NR_CONSOLES];     // added by mingxuan 2019-5-19
CONSOLE console_table[NR_CONSOLES]; // added by mingxuan 2019-5-19
