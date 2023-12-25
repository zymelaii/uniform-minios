
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            start.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "stdio.h"
#include "protect.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

#include "assert.h"
#include "string.h"

/*
 * 当发生不可挽回的错误时就打印错误信息并使CPU核休眠
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	// 确保CPU核不受外界中断的影响
	asm volatile("cli");
	asm volatile("cld");

	va_start(ap, fmt);
	kprintf("kernel panic at %s:%d: ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n");
	va_end(ap);
	// 休眠CPU核，直接罢工
	while(1)
		asm volatile("hlt");
}

/*
 * 很像panic，但是不会休眠CPU核，就是正常打印信息
 */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	kprintf("kernel warning at %s:%d: ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n");
	va_end(ap);
}


/*======================================================================*
                            cstart
 *======================================================================*/
void cstart()
{
	vga_set_disppos(0);
	kprintf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n-----\"cstart\" begins-----\n");

	// 将 LOADER 中的 GDT 复制到新的 GDT 中
	memcpy(	&gdt,				    // New GDT
		(void*)(*((u32*)(&gdt_ptr[2]))),   // Base  of Old GDT
		*((u16*)(&gdt_ptr[0])) + 1	    // Limit of Old GDT
		);
	// gdt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sgdt 以及 lgdt 的参数。
	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
	u32* p_gdt_base  = (u32*)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
	*p_gdt_base  = (u32)&gdt;

	// idt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sidt 以及 lidt 的参数。
	u16* p_idt_limit = (u16*)(&idt_ptr[0]);
	u32* p_idt_base  = (u32*)(&idt_ptr[2]);
	*p_idt_limit = IDT_SIZE * sizeof(GATE) - 1;
	*p_idt_base  = (u32)&idt;

	init_prot();

	kprintf("-----\"cstart\" finished-----\n");
}