%include "loader.inc"

extern setup_paging
extern load_and_enter_kernel

[bits 32]

[section .text]

    global ProtectStart
ProtectStart:
    mov     ax, SelectorFlatRW
    mov     ds, ax
    mov     ss, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     esp, StackTopInProtect
    mov     [PartitionLBA], ebx
    ; enable the page table
    call    setup_paging
    mov     cr3, eax
    mov     eax, cr0
    or      eax, 80000000h
    mov     cr0, eax
    ; jmp into kernel, always noreturn
    call    load_and_enter_kernel

[section .data]

    global PartitionLBA
PartitionLBA    dd 0
