#pragma once

#include <stdint.h>
#include "protect.h"
#include "proc.h"
#include "const.h"
#include "tty.h"
#include "console.h"
#include "hd.h"

/* equal to 1 if kernel is initializing, equal to 0 if done.
 * added by xw, 18/5/31
 */
extern int kernel_initial;

extern int ticks;

extern u8         gdt_ptr[6]; // 0~15:Limit  16~47:Base
extern DESCRIPTOR gdt[GDT_SIZE];
extern u8         idt_ptr[6]; // 0~15:Limit  16~47:Base
extern GATE       idt[IDT_SIZE];

extern u32 k_reenter;
extern int u_proc_sum; // 内核中用户进程/线程数量

extern TSS      tss;
extern PROCESS* p_proc_current;
// the next process that will run. added by xw, 18/4/26
extern PROCESS* p_proc_next;

extern PROCESS       cpu_table[];
extern PROCESS       proc_table[];
extern char          task_stack[];
extern TASK          task_table[];
extern irq_handler_t irq_table[];

/* tty */

extern tty_t   tty_table[];
extern CONSOLE console_table[];
extern int     current_console;

// extern u32 PageTblNum; // 页表数量
extern u32 cr3_ready; // 当前进程的页目录

struct memfree {
    u32 addr;
    u32 size;
};

extern struct hd_info hd_info[1];
