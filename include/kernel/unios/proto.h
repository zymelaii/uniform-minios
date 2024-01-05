#pragma once

#include <stdint.h>
#include <stdarg.h>
#include "tty.h"
#include "proc.h"

/* klib.asm */

void vga_set_disppos(int new_pos);
int  vga_get_disppos();
void vga_write_char(const char ch, u8 color);
void vga_write_str(const char* str);
void vga_write_str_color(const char* str, u8 color);
int  uart_kprintf(const char* fmt, ...);

// added by zcr
void disable_irq(int irq);
void enable_irq(int irq);
void init_8259A();
//~zcr

/* protect.c */
void init_prot();
u32  seg2phys(u16 seg);

/* klib.c */

/* kernel.asm */
void restart_initial();              // added by xw, 18/4/18
void restart_restore();              // added by xw, 18/4/20
void sched();                        // added by xw, 18/4/18
u32  get_arg(void* uesp, int order); // added by xw, 18/6/18

/* ktest.c */
void initial();

/* keyboard.c */
void init_kb();
void keyboard_read(tty_t* p_tty);

/* tty.c */
void tty_keyboard_proc(tty_t* tty, u32 key);
void task_tty();
void tty_write(tty_t* tty, char* buf, int len);
int  tty_read(tty_t* tty, char* buf, int len);

/* recycler.c */
void sweeper();

/* printf.c */
// added by mingxuan 2019-5-19
int printf(const char* fmt, ...);

/* vsprintf.c */
// added by mingxuan 2019-5-19
int vsprintf(char* buf, const char* fmt, va_list args);

/* i8259.c */
void put_irq_handler(int irq, irq_handler_t handler);
void spurious_irq(int irq);

/* clock.c */
void clock_handler(int irq);

/* proc.c */
PROCESS* alloc_pcb();
void     free_pcb(PROCESS* p);
int      ldt_seg_linear(PROCESS* p, int idx);
void*    va2la(int pid, void* va);

/***************************************************************
 * 以上是系统调用相关函数的声明
 ****************************************************************/

void page_fault_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags);
void clear_kernel_pagepte_low();

PROCESS* pid2pcb(int pid);
int      proc2pid(PROCESS* proc);
