#include <unios/interrupt.h>
#include <unios/tracing.h>
#include <unios/proc.h>
#include <arch/x86.h>

irq_handler_t irq_table[NR_IRQS];

static const char* int_str_table[64] = {
    "#DE Divide Error",
    "#DB RESERVED",
    "—  NMI Interrupt",
    "#BP Breakpoint",
    "#OF Overflow",
    "#BR BOUND Range Exceeded",
    "#UD Invalid Opcode (Undefined Opcode)",
    "#NM Device Not Available (No Math Coprocessor)",
    "#DF Double Fault",
    "    Coprocessor Segment Overrun (reserved)",
    "#TS Invalid TSS",
    "#NP Segment Not Present",
    "#SS Stack-Segment Fault",
    "#GP General Protection",
    "#PF Page Fault",
    "—  (Intel reserved. Do not use.)",
    "#MF x87 FPU Floating-Point Error (Math Fault)",
    "#AC Alignment Check",
    "#MC Machine Check",
    "#XF SIMD Floating-Point Exception"};

void init_interrupt_controller() {
    //! NOTE: reference can be found at:
    //! https://github.com/intel/CODK-A-X86/blob/master/external/zephyr/drivers/interrupt_controller/i8259.c

    //! ICW1: init
    //! NOTE: require ICW4 & initiates initialization sequence
    outb(INT_M_CTL, 0x11);
    outb(INT_S_CTL, 0x11);

    //! ICW2: set int vector offset
    //! NOTE: master begins at irq 0, slave begins at irq 8
    outb(INT_M_CTLMASK, INT_VECTOR_IRQ0);
    outb(INT_S_CTLMASK, INT_VECTOR_IRQ8);

    //! ICW3: set cascade mode
    //! NOTE: cascade at irq 4, slave connect to irq 2
    outb(INT_M_CTLMASK, 0x4);
    outb(INT_S_CTLMASK, 0x2);

    //! ICW4: set ctrl word
    //! NOTE: set mode 8086
    outb(INT_M_CTLMASK, 0x1);
    outb(INT_S_CTLMASK, 0x1);

    //! OCW1: ints barrier
    //! NOTE: barrier all interrupts
    outb(INT_M_CTLMASK, 0xff);
    outb(INT_S_CTLMASK, 0xff);

    //! set default irq handler
    for (int i = 0; i < NR_IRQS; i++) { irq_table[i] = spurious_irq; }
}

bool is_irq_masked(int irq) {
    uint8_t mask = 1 << (irq % 8);
    int     port = irq < 8 ? INT_M_CTLMASK : INT_S_CTLMASK;
    return !!(inb(port) & mask);
}

void enable_irq(int irq) {
    uint8_t mask = 1 << (irq % 8);
    int     port = irq < 8 ? INT_M_CTLMASK : INT_S_CTLMASK;
    outb(port, inb(port) & ~mask);
}

void disable_irq(int irq) {
    uint8_t mask = 1 << (irq % 8);
    int     port = irq < 8 ? INT_M_CTLMASK : INT_S_CTLMASK;
    outb(port, inb(port) | mask);
}

void spurious_irq(int irq) {
    kwarn("spurious irq: %d", irq);
}

void put_irq_handler(int irq, irq_handler_t handler) {
    bool masked = is_irq_masked(irq);
    disable_irq(irq);
    irq_table[irq] = handler;
    if (!masked) { enable_irq(irq); }
}

void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags) {
    kerror(
        "[%s] eip=%#08x eflags=%#x cs=%#x err_code=%d from pid=%d",
        int_str_table[vec_no],
        eip,
        eflags,
        cs,
        err_code,
        p_proc_current->pcb.pid);

    p_proc_current->pcb.stat = KILLED;
}
