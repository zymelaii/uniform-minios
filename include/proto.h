#pragma once

#include "unios/tty.h"
#include "type.h"
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
u32  read_cr2();           // add by visual 2016.5.9
void refresh_page_cache(); // add by visual 2016.5.12
// void restart_int();
// void save_context();
void restart_initial();              // added by xw, 18/4/18
void restart_restore();              // added by xw, 18/4/20
void sched();                        // added by xw, 18/4/18
void halt();                         // added by xw, 18/6/11
u32  get_arg(void* uesp, int order); // added by xw, 18/6/18

/* ktest.c */
void initial();

/* keyboard.c */
// added by mingxuan 2019-5-19
void init_kb();
void keyboard_read(tty_t* p_tty);

/* tty.c */
// added by mingxuan 2019-5-19
void tty_keyboard_proc(tty_t* tty, u32 key);
void task_tty();
void tty_write(tty_t* tty, char* buf, int len);
int  tty_read(tty_t* tty, char* buf, int len);

/* printf.c */
// added by mingxuan 2019-5-19
int printf(const char* fmt, ...);

/* vsprintf.c */
// added by mingxuan 2019-5-19
int vsprintf(char* buf, const char* fmt, va_list args);

/* i8259.c */
void put_irq_handler(int irq, irq_handler handler);
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

/*pagepte.c*/
u32  init_page_pte(u32 pid); // edit by visual 2016.4.28
void page_fault_handler(
    u32 vec_no,
    u32 err_code,
    u32 eip,
    u32 cs,
    u32 eflags);                 // add by visual 2016.4.19
u32  get_pde_index(u32 AddrLin); // add by visual 2016.4.28
u32  get_pte_index(u32 AddrLin);
u32  get_pde_phy_addr(u32 pid);
u32  get_pte_phy_addr(u32 pid, u32 AddrLin);
u32  get_page_phy_addr(u32 pid, u32 AddrLin); // 线性地址
u32  pte_exist(u32 PageTblAddrPhy, u32 AddrLin);
u32  phy_exist(u32 PageTblPhyAddr, u32 AddrLin);
void write_page_pde(
    u32 PageDirPhyAddr, u32 AddrLin, u32 TblPhyAddr, u32 Attribute);
void write_page_pte(u32 TblPhyAddr, u32 AddrLin, u32 PhyAddr, u32 Attribute);
int  lin_mapping_phy(
     u32 AddrLin,
     u32 phy_addr,
     u32 pid,
     u32 pde_Attribute,
     u32 pte_Attribute);         // edit by visual 2016.5.19
void clear_kernel_pagepte_low(); // add by visual 2016.5.12
