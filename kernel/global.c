
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*
 * To make things more direct. In the headers below,
 * the variable will be defined here.
 * added by xw, 18/6/17
 */
// #define GLOBAL_VARIABLES_HERE
#include <const.h>
#include <type.h>
#include <protect.h>
#include <proc.h>
#include <proto.h>
#include <fs_const.h>
#include <hd.h>
#include <unios/vfs.h>

int        kernel_initial;
int        ticks;
u8         gdt_ptr[6];
DESCRIPTOR gdt[GDT_SIZE];
u8         idt_ptr[6];
GATE       idt[IDT_SIZE];
u32        k_reenter;
int        u_proc_sum;
TSS        tss;
PROCESS   *p_proc_current;
PROCESS   *p_proc_next;

u32 cr3_ready;

/* save the execution environment of each cpu, which doesn't belong to any
 * process. added by xw, 18/6/1
 */
PROCESS cpu_table[NR_CPUS];

PROCESS proc_table[NR_PCBS]; // edit by visual 2016.4.5

TASK task_table[NR_TASKS] = {
    {hd_service, STACK_SIZE_TASK, "hd_service"},
    {task_tty,   STACK_SIZE_TASK, "task_tty"  }
}; // added by xw, 18/8/27

irq_handler irq_table[NR_IRQ];

system_call sys_call_table[NR_SYS_CALL] = {
    sys_get_ticks,
    sys_get_pid,
    sys_kmalloc,
    sys_kmalloc_4k,
    sys_malloc,
    sys_malloc_4k,
    sys_free,
    sys_free_4k,
    sys_fork,
    sys_pthread,
    sys_udisp_int,
    sys_udisp_str,
    sys_exec,
    sys_yield,
    sys_sleep,
    sys_print_E,
    sys_print_F,
    sys_open,
    sys_close,
    sys_read,
    sys_write,
    sys_lseek,
    sys_unlink,
    sys_create,
    sys_delete,
    sys_opendir,
    sys_createdir,
    sys_deletedir,  
    sys_exit,
};

tty_t   tty_table[NR_CONSOLES];     // added by mingxuan 2019-5-19
CONSOLE console_table[NR_CONSOLES]; // added by mingxuan 2019-5-19
