#ifndef MINIOS_X86_H
#define MINIOS_X86_H

#include <type.h>

static inline u8
inb(int port)
{
	u8 data;
	asm volatile("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void
insb(int port, void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\tinsb"
		     : "=D" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "memory", "cc");
}

static inline u16
inw(int port)
{
	u16 data;
	asm volatile("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void
insw(int port, void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\tinsw"
		     : "=D" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "memory", "cc");
}

static inline u32
inl(int port)
{
	u32 data;
	asm volatile("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void
insl(int port, void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\tinsl"
		     : "=D" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "memory", "cc");
}

static inline void
outb(int port, u8 data)
{
	asm volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

static inline void
outsb(int port, const void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\toutsb"
		     : "=S" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "cc");
}

static inline void
outw(int port, u16 data)
{
	asm volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

static inline void
outsw(int port, const void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\toutsw"
		     : "=S" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "cc");
}

static inline void
outsl(int port, const void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\toutsl"
		     : "=S" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "cc");
}

static inline void
outl(int port, u32 data)
{
	asm volatile("outl %0,%w1" : : "a" (data), "d" (port));
}

static inline void
lcr0(u32 val)
{
	asm volatile("movl %0,%%cr0" : : "r" (val));
}

static inline u32
rcr0(void)
{
	u32 val;
	asm volatile("movl %%cr0,%0" : "=r" (val));
	return val;
}

static inline u32
rcr2(void)
{
	u32 val;
	asm volatile("movl %%cr2,%0" : "=r" (val));
	return val;
}

static inline void
lcr3(u32 val)
{
	asm volatile("movl %0,%%cr3" : : "r" (val));
}

static inline u32
rcr3(void)
{
	u32 val;
	asm volatile("movl %%cr3,%0" : "=r" (val));
	return val;
}

static inline void
lcr4(u32 val)
{
	asm volatile("movl %0,%%cr4" : : "r" (val));
}

static inline u32
rcr4(void)
{
	u32 cr4;
	asm volatile("movl %%cr4,%0" : "=r" (cr4));
	return cr4;
}

static inline void
tlbflush(void)
{
	u32 cr3;
	asm volatile("movl %%cr3,%0" : "=r" (cr3));
	asm volatile("movl %0,%%cr3" : : "r" (cr3));
}

static inline u32
read_eflags(void)
{
	u32 eflags;
	asm volatile("pushfl\n\t popl %0" : "=r" (eflags));
	return eflags;
}

static inline void
write_eflags(u32 eflags)
{
	asm volatile("pushl %0\n\t popfl" : : "r" (eflags));
}

static inline u32
read_ebp(void)
{
	u32 ebp;
	asm volatile("movl %%ebp,%0" : "=r" (ebp));
	return ebp;
}

static inline u32
read_esp(void)
{
	u32 esp;
	asm volatile("movl %%esp,%0" : "=r" (esp));
	return esp;
}

static inline u64
read_tsc(void)
{
	u64 tsc;
	asm volatile("rdtsc" : "=A" (tsc));
	return tsc;
}

static inline u32
xchg(volatile u32 *addr, u32 newval)
{
	u32 result;

	// The + in "+m" denotes a read-modify-write operand.
	asm volatile("lock\n\t xchgl %0, %1"
		     : "+m" (*addr), "=a" (result)
		     : "1" (newval)
		     : "cc");
	return result;
}

static inline void
disable_int()
{
	asm volatile("cli");
}

static inline void
enable_int()
{
	asm volatile("sti");
}

#endif /* MINIOS_X86_H */