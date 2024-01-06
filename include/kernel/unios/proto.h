#pragma once

#include <stdint.h>
#include <stdarg.h>
#include "tty.h"
#include "proc.h"

void init_protect_mode();
u32  seg2phys(u16 seg);

void restart_initial();
void restart_restore();
void sched();
u32  get_arg(void* uesp, int order);

void initial();

void tty_keyboard_proc(tty_t* tty, u32 key);
void task_tty();
void tty_write(tty_t* tty, char* buf, int len);
int  tty_read(tty_t* tty, char* buf, int len);

void sweeper();

int printf(const char* fmt, ...);

int vsprintf(char* buf, const char* fmt, va_list args);

void clock_handler(int irq);

PROCESS* alloc_pcb();
void     free_pcb(PROCESS* p);
int      ldt_seg_linear(PROCESS* p, int idx);
void*    va2la(int pid, void* va);

void page_fault_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags);
void clear_kernel_pagepte_low();
