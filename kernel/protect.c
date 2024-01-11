#include <unios/protect.h>
#include <unios/interrupt.h>
#include <unios/layout.h>
#include <unios/proc.h>
#include <sys/types.h>
#include <string.h>

descptr_t         gdt_ptr;
descriptor_t      gdt[GDT_SIZE];
descptr_t         idt_ptr;
gate_descriptor_t idt[IDT_SIZE];

void init_idt_desc(
    unsigned char vector, u8 type, int_handler_t handler, unsigned char rpl) {
    gate_descriptor_t* gate = &idt[vector];
    u32                base = (u32)handler;
    gate->offset_low        = base & 0xFFFF;
    gate->selector          = SELECTOR_KERNEL_CS;
    gate->reserved          = 0;
    gate->attr              = type | (rpl << 5);
    gate->offset_high       = (base >> 16) & 0xFFFF;
}

u32 seg2phys(u16 seg) {
    descriptor_t* desc = &gdt[seg >> 3];
    return (desc->base2 << 24) | (desc->base1 << 16) | (desc->base0);
}

void init_descriptor(descriptor_t* desc, u32 base, u32 limit, u16 attr) {
    desc->limit0       = limit & 0xffff;
    desc->base0        = base & 0xffff;
    desc->base1        = (base >> 16) & 0xff;
    desc->attr0        = attr & 0xff;
    desc->attr1_limit1 = ((limit >> 16) & 0x0f) | ((attr >> 8) & 0xf0);
    desc->base2        = (base >> 24) & 0xff;
}

void init_protect_mode() {
    init_interrupt_controller();

    init_idt_desc(INT_VECTOR_DIVIDE, DA_386IGate, division_error, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_DEBUG, DA_386IGate, debug_exception, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_NMI, DA_386IGate, nmi, RPL_KERNEL);
    init_idt_desc(
        INT_VECTOR_BREAKPOINT, DA_386IGate, breakpoint_exception, RPL_USER);
    init_idt_desc(
        INT_VECTOR_OVERFLOW, DA_386IGate, overflow_exception, RPL_USER);
    init_idt_desc(
        INT_VECTOR_BOUNDS, DA_386IGate, bound_range_exceeded, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_INVAL_OP, DA_386IGate, invalid_opcode, RPL_KERNEL);
    init_idt_desc(
        INT_VECTOR_DEVICE_NOT, DA_386IGate, device_not_available, RPL_KERNEL);
    init_idt_desc(
        INT_VECTOR_DOUBLE_FAULT, DA_386IGate, double_fault, RPL_KERNEL);
    init_idt_desc(
        INT_VECTOR_COPROC_SEG, DA_386IGate, copr_seg_overrun, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_INVAL_TSS, DA_386IGate, invalid_tss, RPL_KERNEL);
    init_idt_desc(
        INT_VECTOR_SEG_NOT, DA_386IGate, segment_not_present, RPL_KERNEL);
    init_idt_desc(
        INT_VECTOR_STACK_FAULT, DA_386IGate, stack_seg_exception, RPL_KERNEL);
    init_idt_desc(
        INT_VECTOR_PROTECTION, DA_386IGate, general_protection, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_PAGE_FAULT, DA_386IGate, page_fault, RPL_KERNEL);
    init_idt_desc(
        INV_VECTOR_FP_EXCEPTION,
        DA_386IGate,
        floating_point_exception,
        RPL_KERNEL);

    init_idt_desc(INT_VECTOR_IRQ0 + 0, DA_386IGate, hwint00, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 1, DA_386IGate, hwint01, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 2, DA_386IGate, hwint02, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 3, DA_386IGate, hwint03, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 4, DA_386IGate, hwint04, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 5, DA_386IGate, hwint05, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 6, DA_386IGate, hwint06, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ0 + 7, DA_386IGate, hwint07, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 0, DA_386IGate, hwint08, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 1, DA_386IGate, hwint09, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 2, DA_386IGate, hwint10, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 3, DA_386IGate, hwint11, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 4, DA_386IGate, hwint12, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 5, DA_386IGate, hwint13, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 6, DA_386IGate, hwint14, RPL_KERNEL);
    init_idt_desc(INT_VECTOR_IRQ8 + 7, DA_386IGate, hwint15, RPL_KERNEL);

    init_idt_desc(INT_VECTOR_SYSCALL, DA_386IGate, syscall_handler, RPL_USER);

    init_descriptor(
        &gdt[INDEX_VIDEO], (u32)K_PHY2LIN(0x0B8000), 0x0ffff, DA_DRW | DA_DPL3);

    memset(&tss, 0, sizeof(tss));
    tss.ss0 = SELECTOR_KERNEL_DS;
    init_descriptor(
        &gdt[INDEX_TSS],
        vir2phys(seg2phys(SELECTOR_KERNEL_DS), &tss),
        sizeof(tss) - 1,
        DA_386TSS);
    tss.iobase = sizeof(tss);
}
