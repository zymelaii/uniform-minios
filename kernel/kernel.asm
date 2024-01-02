; Memory Model
;            ┃ ...                 ┃
;            ┣━━━━━━━━━━━━━━━━━━━━━┫
;            ┃■ page table ■■■■■■■■┃ PageTblBase, size is decided by loader
; 0x00101000 ┣━━━━━━━━━━━━━━━━━━━━━┫
;            ┃■ page dir table ■■■■┃ PageDirBase = 1M
; 0x00100000 ┣━━━━━━━━━━━━━━━━━━━━━┫
;            ┃□ hardware reserved □┃ 0x000b8000 ← gs
; 0x0009fc00 ┣━━━━━━━━━━━━━━━━━━━━━┫
;            ┃■ loader.bin ■■■■■■■■┃ somewhere in loader ← esp
; 0x00090000 ┣━━━━━━━━━━━━━━━━━━━━━┫
;            ┃■ kernel.bin ■■■■■■■■┃
; 0x00080000 ┣━━━━━━━━━━━━━━━━━━━━━┫
;            ┃■ kernel runtime ■■■■┃ 30400h ← kernel entry point, KernelEntryPointPhyAddr
; 0x00030000 ┣━━━━━━━━━━━━━━━━━━━━━┫
;            ┋ ...                 ┋
; 0x00000000 ┗━━━━━━━━━━━━━━━━━━━━━┛ ← cs, ds, es, fs, ss


; GDT
; ┏━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━┓
; ┃      Descriptors      ┃        Selectors       ┃
; ┣━━━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━┫
; ┃   Dummy Descriptor    ┃                        ┃
; ┣━━━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━┫
; ┃   DESC_FLAT_C (0~4G)  ┃ 0x08 = cs              ┃
; ┣━━━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━┫
; ┃  DESC_FLAT_RW (0~4G)  ┃ 0x10 = ds, es, fs, ss  ┃
; ┣━━━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━┫
; ┃       DESC_VIDEO      ┃ 0x1b = gs              ┃
; ┗━━━━━━━━━━━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━━━━┛

%include "sconst.inc"

extern cstart
extern kernel_main
extern spurious_irq
extern clock_handler
extern disp_str
extern irq_table
extern schedule
extern halt
extern switch_pde

extern gdt_ptr
extern idt_ptr
extern p_proc_current
extern tss
extern k_reenter
extern syscall_table
extern cr3_ready
extern p_proc_current
extern p_proc_next
extern kernel_initial

[bits 32]

[section .bss]
StackSpace resb 2 * 1024
StackTop:       ; used only as irq-stack in minios

KernelStackSpace resb 2 * 1024
KernelStackTop: ; used as stack of kernel itself

[section .text]

global restart_initial
global restart_restore
global sched

    global _start
_start:
    mov esp, KernelStackTop

    ; load & flush gdt
    sgdt [gdt_ptr]
    call cstart
    lgdt [gdt_ptr]

    lidt [idt_ptr]

    ; force to activate the lately inited struct
    jmp  SELECTOR_KERNEL_CS:csinit
csinit:
    xor eax, eax
    mov ax, SELECTOR_TSS
    ltr ax

    jmp kernel_main

    global save_exception
save_exception:
    pushad          ; save regs -->
    push    ds      ;
    push    es      ;
    push    fs      ;
    push    gs      ; <--
    mov     dx, ss
    mov     ds, dx
    mov     es, dx
    mov     fs, dx
    mov     dx, SELECTOR_VIDEO - 2
    mov     gs, dx

    mov     esi, esp
    push    restart_exception
    jmp     [esi + RETADR - P_STACKBASE]

restart_exception:
    call    sched
    pop     gs
    pop     fs
    pop     es
    pop     ds
    popad
    ; clear retaddr and error code in stack
    add     esp, 4 * 2
    iretd

    global save_int
save_int:
    pushad          ; save regs -->
    push    ds      ;
    push    es      ;
    push    fs      ;
    push    gs      ; <--
    cmp     dword [k_reenter], 0
    jnz     instack ; already in the irq-stack
    mov     ebx,  [p_proc_current]
    mov     dword [ebx + ESP_SAVE_INT], esp
    mov     dx, ss
    mov     ds, dx
    mov     es, dx
    mov     fs, dx
    mov     dx, SELECTOR_VIDEO - 2
    mov     gs, dx
    mov     esi, esp
    mov     esp, StackTop
    push    restart_int
    jmp     [esi + RETADR - P_STACKBASE]
instack:
    push    restart_restore
    ; TODO: so why offset 1 dword here? expect comment
    jmp     [esp + 4 + RETADR - P_STACKBASE]

    global save_syscall
save_syscall:
    pushad          ; save regs -->
    push    ds      ;
    push    es      ;
    push    fs      ;
    push    gs      ; <--
    mov     edx,  [p_proc_current]
    mov     dword [edx + ESP_SAVE_SYSCALL], esp
    mov     dx, ss
    mov     ds, dx
    mov     es, dx
    mov     fs, dx
    mov     dx, SELECTOR_VIDEO - 2
    mov     gs, dx
    mov     esi, esp
    push    restart_syscall
    jmp     [esi + RETADR - P_STACKBASE]

    global sched
sched:
    pushfd
    pushad
    cli
    mov     ebx,  [p_proc_current]
    mov     dword [ebx + ESP_SAVE_CONTEXT], esp
    call    schedule
    mov     ebx,  [p_proc_next]
    mov     dword [p_proc_current], ebx
    call    renew_env
    mov     ebx, [p_proc_current]
    mov     esp, [ebx + ESP_SAVE_CONTEXT]
    popad
    popfd
    ret

renew_env:
    call    switch_pde
    mov     eax, [cr3_ready]
    mov     cr3, eax
    mov     eax, [p_proc_current]
    lldt    [eax + P_LDT_SEL]
    lea     ebx, [eax + INIT_STACK_SIZE]
    mov     dword [tss + TSS3_S_SP0], ebx
    ret

    global syscall_handler
syscall_handler:
    call save_syscall
    sti
    call    [syscall_table + eax * 4]
    cli
    mov     edx, [p_proc_current]
    mov     esi, [edx + ESP_SAVE_SYSCALL]
    mov     [esi + EAXREG - P_STACKBASE], eax
    ret

restart_int:
    mov  eax, [p_proc_current]
    mov  esp, [eax + ESP_SAVE_INT]
    cmp     dword [kernel_initial], 0
    jnz     restart_restore
    call sched
    jmp     restart_restore

restart_syscall:
    mov  eax, [p_proc_current]
    mov  esp, [eax + ESP_SAVE_SYSCALL]
    call sched
    jmp  restart_restore

    global restart_restore
restart_restore:
    pop  gs
    pop  fs
    pop  es
    pop  ds
    popad
    add  esp, 4
    iretd

    global restart_initial
restart_initial:
    call renew_env
    mov  eax, [p_proc_current]
    mov  esp, [eax + ESP_SAVE_INT]
    jmp  restart_restore

; u32 get_arg(void *uesp, int order)
; used to get the specified argument of the syscall from user space stack
; NOTE: let order = n to get the nth argument
; TODO: abandon this
    global get_arg
get_arg:
    push ebp
    mov ebp, esp
    push esi
    push edi
    mov esi, dword [ebp + 8] ; void *uesp
    mov edi, dword [ebp + 12] ; int order
    mov eax, dword [esi + edi * 4 + 4]
    pop edi
    pop esi
    pop ebp
    ret
