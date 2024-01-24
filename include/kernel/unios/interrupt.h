#pragma once

#include <sys/types.h>
#include <stdbool.h>

//! 8259A interrupt controller ports
//! <Master> I/O port for interrupt controller
#define INT_M_CTL 0x20
//! <Master> setting bits in this port disables ints
#define INT_M_CTLMASK 0x21
//! <Slave> I/O port for second interrupt controller
#define INT_S_CTL 0xA0
//! <Slave> setting bits in this port disables ints
#define INT_S_CTLMASK 0xA1

//! init 8259A interrupt controller
void init_interrupt_controller();

//! hardware interrupts
#define NR_IRQS            16
#define CLOCK_IRQ          0  //<! clock
#define KEYBOARD_IRQ       1  //<! keyboard
#define CASCADE_IRQ        2  //<! cascade enable for 2nd AT controller
#define ETHER_IRQ          3  //<! default ethernet interrupt vector
#define SECONDARY_IRQ      3  //<! RS232 interrupt vector for port 2
#define RS232_IRQ          4  //<! RS232 interrupt vector for port 1
#define XT_WINI_IRQ        5  //<! xt winchester
#define FLOPPY_IRQ         6  //<! floppy disk
#define PRINTER_IRQ        7  //<! printer
#define REALTIME_CLOCK_IRQ 8  //<! realtime clock
#define REDIRECT_IRQ       9  //<! irq 2 redirected
#define MOUSE_IRQ          12 //<! mouse (or else)
#define FPU_IRQ            13 //<! fpu exception
#define AT_WINI_IRQ        14 //<! AT winchester disk

#define INT_VECTOR_IRQ0 0x20
#define INT_VECTOR_IRQ8 0x28

#define INT_VECTOR_DIVIDE       0x00
#define INT_VECTOR_DEBUG        0x01
#define INT_VECTOR_NMI          0x02
#define INT_VECTOR_BREAKPOINT   0x03
#define INT_VECTOR_OVERFLOW     0x04
#define INT_VECTOR_BOUNDS       0x05
#define INT_VECTOR_INVAL_OP     0x06
#define INT_VECTOR_DEVICE_NOT   0x07
#define INT_VECTOR_DOUBLE_FAULT 0x08
#define INT_VECTOR_COPROC_SEG   0x09
#define INT_VECTOR_INVAL_TSS    0x0a
#define INT_VECTOR_SEG_NOT      0x0b
#define INT_VECTOR_STACK_FAULT  0x0c
#define INT_VECTOR_PROTECTION   0x0d
#define INT_VECTOR_PAGE_FAULT   0x0e
#define INV_VECTOR_FP_EXCEPTION 0x10

#define INT_VECTOR_SYSCALL 0x80

bool is_irq_masked(int irq);
void enable_irq(int irq);
void disable_irq(int irq);
void spurious_irq(int irq);
void put_irq_handler(int irq, irq_handler_t handler);

void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags);

void hwint00();
void hwint01();
void hwint02();
void hwint03();
void hwint04();
void hwint05();
void hwint06();
void hwint07();
void hwint08();
void hwint09();
void hwint10();
void hwint11();
void hwint12();
void hwint13();
void hwint14();
void hwint15();

void division_error();
void debug_exception();
void nmi();
void breakpoint_exception();
void overflow_exception();
void bound_range_exceeded();
void invalid_opcode();
void device_not_available();
void double_fault();
void copr_seg_overrun();
void invalid_tss();
void segment_not_present();
void stack_seg_exception();
void general_protection();
void page_fault();
void floating_point_exception();

void syscall_handler();
