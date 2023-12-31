#pragma once

#include <stdint.h>

#define ASMCALL __attribute__((optimize("O3"))) static inline

ASMCALL u8 inb(int port) {
    u8 data;
    __asm__ volatile("inb %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

ASMCALL void insb(int port, void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "insb\n"
                     : "=D"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "memory", "cc");
}

ASMCALL u16 inw(int port) {
    u16 data;
    __asm__ volatile("inw %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

ASMCALL void insw(int port, void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "insw\n"
                     : "=D"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "memory", "cc");
}

ASMCALL u32 inl(int port) {
    u32 data;
    __asm__ volatile("inl %w1, %0" : "=a"(data) : "d"(port));
    return data;
}

ASMCALL void insl(int port, void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "insl\n"
                     : "=D"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "memory", "cc");
}

ASMCALL void outb(int port, u8 data) {
    __asm__ volatile("outb %0, %w1" : : "a"(data), "d"(port));
}

ASMCALL void outsb(int port, const void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "outsb\n"
                     : "=S"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "cc");
}

ASMCALL void outw(int port, u16 data) {
    __asm__ volatile("outw %0, %w1" : : "a"(data), "d"(port));
}

ASMCALL void outsw(int port, const void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "outsw\n"
                     : "=S"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "cc");
}

ASMCALL void outsl(int port, const void *addr, int cnt) {
    __asm__ volatile("cld\n"
                     "repne\n"
                     "outsl\n"
                     : "=S"(addr), "=c"(cnt)
                     : "d"(port), "0"(addr), "1"(cnt)
                     : "cc");
}

ASMCALL void outl(int port, u32 data) {
    __asm__ volatile("outl %0, %w1" : : "a"(data), "d"(port));
}

ASMCALL void lcr0(u32 val) {
    __asm__ volatile("movl %0, %%cr0" : : "r"(val));
}

ASMCALL u32 rcr0() {
    u32 val;
    __asm__ volatile("movl %%cr0, %0" : "=r"(val));
    return val;
}

ASMCALL u32 rcr2() {
    u32 val;
    __asm__ volatile("movl %%cr2, %0" : "=r"(val));
    return val;
}

ASMCALL void lcr3(u32 val) {
    __asm__ volatile("movl %0, %%cr3" : : "r"(val));
}

ASMCALL u32 rcr3() {
    u32 val;
    __asm__ volatile("movl %%cr3, %0" : "=r"(val));
    return val;
}

ASMCALL void lcr4(u32 val) {
    __asm__ volatile("movl %0, %%cr4" : : "r"(val));
}

ASMCALL u32 rcr4() {
    u32 cr4;
    __asm__ volatile("movl %%cr4, %0" : "=r"(cr4));
    return cr4;
}

ASMCALL void tlbflush() {
    __asm__ volatile("mov %cr3, %eax\n"
                     "mov %eax, %cr3\n");
}

ASMCALL u32 read_eflags() {
    u32 eflags;
    __asm__ volatile("pushfl\n"
                     "popl %0\n"
                     : "=r"(eflags));
    return eflags;
}

ASMCALL void write_eflags(u32 eflags) {
    __asm__ volatile("pushl %0\n"
                     "popfl\n"
                     :
                     : "r"(eflags));
}

ASMCALL u32 read_ebp() {
    u32 ebp;
    __asm__ volatile("movl %%ebp, %0" : "=r"(ebp));
    return ebp;
}

ASMCALL u32 read_esp() {
    u32 esp;
    __asm__ volatile("movl %%esp, %0" : "=r"(esp));
    return esp;
}

ASMCALL u64 read_tsc() {
    u64 tsc;
    __asm__ volatile("rdtsc" : "=A"(tsc));
    return tsc;
}

ASMCALL u32 xchg(volatile u32 *addr, u32 newval) {
    u32 result;
    __asm__ volatile("lock\n"
                     "xchgl %0, %1\n"
                     : "+m"(*addr), "=a"(result)
                     : "1"(newval)
                     : "cc");
    return result;
}

ASMCALL void clear_dir_flag() {
    __asm__ volatile("cld");
}

ASMCALL void disable_int() {
    __asm__ volatile("cli");
}

ASMCALL void enable_int() {
    __asm__ volatile("sti");
}

ASMCALL void halt() {
    __asm__ volatile("hlt");
}

#undef ASMCALL
