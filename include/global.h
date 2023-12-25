
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* equal to 1 if kernel is initializing, equal to 0 if done.
 * added by xw, 18/5/31
 */
extern int		kernel_initial;

extern int		ticks;

extern u8		gdt_ptr[6];	// 0~15:Limit  16~47:Base
extern DESCRIPTOR	gdt[GDT_SIZE];
extern u8		idt_ptr[6];	// 0~15:Limit  16~47:Base
extern GATE		idt[IDT_SIZE];

extern u32		k_reenter;
extern int     u_proc_sum; 		//内核中用户进程/线程数量 add by visual 2016.5.25

extern TSS		tss;
extern PROCESS*	p_proc_current;
extern PROCESS*	p_proc_next;	//the next process that will run. added by xw, 18/4/26

extern	PROCESS		cpu_table[];	//added by xw, 18/6/1
extern	PROCESS		proc_table[];
extern	char		task_stack[];
extern  TASK        task_table[];
extern	irq_handler	irq_table[];

/* tty */
//added by mingxuan 2019-5-19
#include "tty.h"
#include "console.h"

extern  TTY		tty_table[];
extern  CONSOLE		console_table[];
extern	int		current_console;

//	u32 PageTblNum;		//页表数量		add by visual 2016.4.5
extern u32 cr3_ready;		//当前进程的页目录		add by visual 2016.4.5

struct memfree{
	u32	addr;
	u32	size;
};

#include "fs_const.h"
#include "hd.h"
extern struct hd_info hd_info[1];   //added by mingxuan 2020-10-27