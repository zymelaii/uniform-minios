#include <unios/protect.h>
#include <unios/tracing.h>
#include <unios/tracing_handler.h>
#include <stdio.h>
#include <string.h>

void cstart() {
    // descriptor(&gdt, 0, 0x00000000, 0x00000, 0);
    // descriptor(&gdt, 1, 0x00000000, 0xfffff, DA_CR | DA_32 | DA_LIMIT_4K);
    // descriptor(&gdt, 2, 0x00000000, 0xfffff, DA_DRW | DA_32 | DA_LIMIT_4K);
    // descriptor(&gdt, 3, 0x000b8000, 0x0ffff, DA_DRW | DA_DPL3);

    memcpy(&gdt, (void*)gdt_ptr.base, gdt_ptr.limit + 1);

    gdt_ptr.limit = GDT_SIZE * sizeof(descriptor_t) - 1;
    gdt_ptr.base  = (u32)&gdt;
    idt_ptr.limit = IDT_SIZE * sizeof(gate_descriptor_t) - 1;
    idt_ptr.base  = (u32)&idt;

    init_protect_mode();

    klog_set_handler(klog_serial_handler, NULL);
    kdebug("cstart done");
}
