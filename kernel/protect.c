#include <unios/syscall.h>
#include <unios/const.h>
#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/global.h>
#include <unios/proto.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

static void init_idt_desc(
    unsigned char vector,
    u8            desc_type,
    int_handler_t handler,
    unsigned char privilege);
static void
    init_descriptor(DESCRIPTOR* p_desc, u32 base, u32 limit, u16 attribute);

/* 中断处理函数 */
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

/*======================================================================*
                            init_prot
 *----------------------------------------------------------------------*
 初始化 IDT
 *======================================================================*/
void init_prot() {
    init_8259A();

    // 全部初始化成中断门(没有陷阱门)
    init_idt_desc(
        INT_VECTOR_DIVIDE, DA_386IGate, division_error, PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_DEBUG, DA_386IGate, debug_exception, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_NMI, DA_386IGate, nmi, PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_BREAKPOINT,
        DA_386IGate,
        breakpoint_exception,
        PRIVILEGE_USER);

    init_idt_desc(
        INT_VECTOR_OVERFLOW, DA_386IGate, overflow_exception, PRIVILEGE_USER);

    init_idt_desc(
        INT_VECTOR_BOUNDS, DA_386IGate, bound_range_exceeded, PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_INVAL_OP, DA_386IGate, invalid_opcode, PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_DEVICE_NOT,
        DA_386IGate,
        device_not_available,
        PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_DOUBLE_FAULT, DA_386IGate, double_fault, PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_COPROC_SEG, DA_386IGate, copr_seg_overrun, PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_INVAL_TSS, DA_386IGate, invalid_tss, PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_SEG_NOT, DA_386IGate, segment_not_present, PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_STACK_FAULT,
        DA_386IGate,
        stack_seg_exception,
        PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_PROTECTION, DA_386IGate, general_protection, PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_PAGE_FAULT, DA_386IGate, page_fault, PRIVILEGE_KRNL);

    init_idt_desc(
        INV_VECTOR_FP_EXCEPTION,
        DA_386IGate,
        floating_point_exception,
        PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 0, DA_386IGate, hwint00, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 1, DA_386IGate, hwint01, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 2, DA_386IGate, hwint02, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 3, DA_386IGate, hwint03, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 4, DA_386IGate, hwint04, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 5, DA_386IGate, hwint05, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 6, DA_386IGate, hwint06, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ0 + 7, DA_386IGate, hwint07, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 0, DA_386IGate, hwint08, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 1, DA_386IGate, hwint09, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 2, DA_386IGate, hwint10, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 3, DA_386IGate, hwint11, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 4, DA_386IGate, hwint12, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 5, DA_386IGate, hwint13, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 6, DA_386IGate, hwint14, PRIVILEGE_KRNL);

    init_idt_desc(INT_VECTOR_IRQ8 + 7, DA_386IGate, hwint15, PRIVILEGE_KRNL);

    init_idt_desc(
        INT_VECTOR_SYS_CALL, DA_386IGate, syscall_handler, PRIVILEGE_USER);

    /*修改显存描述符*/ // add by visual 2016.5.12
    init_descriptor(
        &gdt[INDEX_VIDEO], K_PHY2LIN(0x0B8000), 0x0ffff, DA_DRW | DA_DPL3);

    /* 填充 GDT 中 TSS 这个描述符 */
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = SELECTOR_KERNEL_DS;
    init_descriptor(
        &gdt[INDEX_TSS],
        vir2phys(seg2phys(SELECTOR_KERNEL_DS), &tss),
        sizeof(tss) - 1,
        DA_386TSS);
    tss.iobase = sizeof(tss); /* 没有I/O许可位图 */

    // 填充 GDT 中进程的 LDT 的描述符
    int      i;
    PROCESS* p_proc       = proc_table;
    u16      selector_ldt = INDEX_LDT_FIRST << 3;
    for (i = 0; i < NR_PCBS; i++) { // edit by visual 2016.4.5
        init_descriptor(
            &gdt[selector_ldt >> 3],
            vir2phys(seg2phys(SELECTOR_KERNEL_DS), proc_table[i].pcb.ldts),
            LDT_SIZE * sizeof(DESCRIPTOR) - 1,
            DA_LDT);
        p_proc++;
        selector_ldt += 1 << 3;
    }
}

/*======================================================================*
                             init_idt_desc
 *----------------------------------------------------------------------*
 初始化 386 中断门
 *======================================================================*/
void init_idt_desc(
    unsigned char vector,
    u8            desc_type,
    int_handler_t handler,
    unsigned char privilege) {
    GATE* p_gate        = &idt[vector];
    u32   base          = (u32)handler;
    p_gate->offset_low  = base & 0xFFFF;
    p_gate->selector    = SELECTOR_KERNEL_CS;
    p_gate->dcount      = 0;
    p_gate->attr        = desc_type | (privilege << 5);
    p_gate->offset_high = (base >> 16) & 0xFFFF;
}

/*======================================================================*
                           seg2phys
 *----------------------------------------------------------------------*
 由段名求绝对地址
 *======================================================================*/
u32 seg2phys(u16 seg) {
    DESCRIPTOR* p_dest = &gdt[seg >> 3];

    return (p_dest->base_high << 24) | (p_dest->base_mid << 16)
         | (p_dest->base_low);
}

/*======================================================================*
                           init_descriptor
 *----------------------------------------------------------------------*
 初始化段描述符
 *======================================================================*/
static void
    init_descriptor(DESCRIPTOR* p_desc, u32 base, u32 limit, u16 attribute) {
    p_desc->limit_low = limit & 0x0FFFF;      // 段界限 1		(2 字节)
    p_desc->base_low  = base & 0x0FFFF;       // 段基址 1		(2 字节)
    p_desc->base_mid  = (base >> 16) & 0x0FF; // 段基址 2		(1 字节)
    p_desc->attr1     = attribute & 0xFF;     // 属性 1
    p_desc->limit_high_attr2 =
        ((limit >> 16) & 0x0F) | ((attribute >> 8) & 0xF0); // 段界限 2 + 属性 2
    p_desc->base_high = (base >> 24) & 0x0FF; // 段基址 3		(1 字节)
}

/*======================================================================*
                            exception_handler
 *----------------------------------------------------------------------*
 异常处理
 *======================================================================*/
void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags) {
    int  i;
    int  text_color            = 0x74; /* 灰底红字 */
    char err_description[][64] = {
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

    trace_logging(
        "\n[Exception %s] eip=%08x eflags=0x%x cs=0x%x err_code=%d from "
        "pid=%d\n",
        err_description[vec_no],
        eip,
        eflags,
        cs,
        err_code,
        p_proc_current->pcb.pid);

    p_proc_current->pcb.stat = KILLED;
}

/*======================================================================*
                            divide error handler
 *======================================================================*/
// used for testing if a exception handler can be interrupted rightly, so it's
// not a real division_error handler now. added by xw, 18/12/22
void division_error_handler() {
    int vec_no, err_code, eip, cs, eflags;
    int i, j;

    __asm__ volatile(
        "mov 8(%%ebp), %0\n\t"  // get vec_no from stack
        "mov 12(%%ebp), %1\n\t" // get err_code from stack
        "mov 16(%%ebp), %2\n\t" // get eip from stack
        "mov 20(%%ebp), %3\n\t" // get cs from stack
        "mov 24(%%ebp), %4\n\t" // get eflags from stack
        : "=r"(vec_no), "=r"(err_code), "=r"(eip), "=r"(cs), "=r"(eflags));
    exception_handler(vec_no, err_code, eip, cs, eflags);

    while (1) {
        kprintf("Loop in divide error handler...\n");

        i = 100;
        while (--i) {
            j = 1000;
            while (--j) {}
        }
    }
}
