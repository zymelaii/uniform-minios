%include "sconst.inc"

; 导入函数
extern	cstart
extern	kernel_main
extern	spurious_irq
extern	clock_handler
extern	disp_str
extern	irq_table
extern  schedule
extern  switch_pde

; 导入全局变量
extern	gdt_ptr
extern	idt_ptr
extern	p_proc_current
extern	tss
extern	k_reenter
extern	syscall_table
extern 	cr3_ready
extern  p_proc_current
extern	p_proc_next
extern	kernel_initial

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
	; 此时内存看上去是这样的（更详细的内存情况在 LOADER.ASM 中有说明）：
	;              ┃                                    ┃
	;              ┃                 ...                ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■Page  Tables■■■■■■┃
	;              ┃■■■■■(大小由LOADER决定)■■■■┃ PageTblBase
	;    00101000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■Page Directory Table■■■■┃ PageDirBase = 1M
	;    00100000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□ Hardware  Reserved □□□□┃ B8000h ← gs
	;       9FC00h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■LOADER.BIN■■■■■■┃ somewhere in LOADER ← esp
	;       90000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■KERNEL.BIN■■■■■■┃
	;       80000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■KERNEL■■■■■■■┃ 30400h ← KERNEL 入口 (KernelEntryPointPhyAddr)
	;       30000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┋                 ...                ┋
	;              ┋                                    ┋
	;           0h ┗━━━━━━━━━━━━━━━━━━┛ ← cs, ds, es, fs, ss
	;
	;
	; GDT 以及相应的描述符是这样的：
	;
	;		              Descriptors               Selectors
	;              ┏━━━━━━━━━━━━━━━━━━┓
	;              ┃         Dummy Descriptor           ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_FLAT_C    (0～4G)     ┃   8h = cs
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_FLAT_RW   (0～4G)     ┃  10h = ds, es, fs, ss
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_VIDEO                 ┃  1Bh = gs
	;              ┗━━━━━━━━━━━━━━━━━━┛
	;
	; 注意! 在使用 C 代码的时候一定要保证 ds, es, ss 这几个段寄存器的值是一样的
	; 因为编译器有可能编译出使用它们的代码, 而编译器默认它们是一样的. 比如串拷贝操作会用到 ds 和 es.
	;
	;

	; 把 esp 从 LOADER 挪到 KERNEL
	; modified by xw, 18/6/15
	;mov	esp, StackTop	; 堆栈在 bss 段中
	mov	esp, KernelStackTop	; 堆栈在 bss 段中


	sgdt	[gdt_ptr]	; cstart() 中将会用到 gdt_ptr
	call	cstart		; 在此函数中改变了gdt_ptr，让它指向新的GDT
	lgdt	[gdt_ptr]	; 使用新的GDT

	lidt	[idt_ptr]

	jmp		SELECTOR_KERNEL_CS:csinit
csinit:		; “这个跳转指令强制使用刚刚初始化的结构”——<<OS:D&I 2nd>> P90.

	xor	eax, eax
	mov	ax, SELECTOR_TSS
	ltr	ax

	jmp	kernel_main

; environment saving when an exception occurs
	global save_exception
save_exception:
	pushad          ; `.
	push    ds      ;  |
	push    es      ;  | 保存原寄存器值
	push    fs      ;  |
	push    gs      ; /
	mov     dx, ss
	mov     ds, dx
	mov     es, dx
	mov		fs, dx 	;value of fs and gs in user process is different to that in kernel
	mov		dx, SELECTOR_VIDEO - 2
	mov		gs, dx

	mov     esi, esp
	push    restart_exception
	jmp     [esi + RETADR - P_STACKBASE]
						;the err code is in higher address than retaddr in stack, so there is
						;no need to modify the position jumped to, that is,
						;"jmp [esi + RETADR - 4 - P_STACKBASE]" is actually wrong.

;added by xw, 18/12/18
restart_exception:
	call	sched
	pop		gs
	pop		fs
	pop		es
	pop		ds
	popad
	add		esp, 4 * 2	; clear retaddr and error code in stack
	iretd

; ====================================================================================
;                                   save
; ====================================================================================
	global save_int
save_int:
	pushad          ; `.
	push    ds      ;  |
	push    es      ;  | 保存原寄存器值
	push    fs      ;  |
	push    gs      ; /

	cmp		dword [k_reenter], 0
	jnz		instack

	; save esp position in the kernel-stack of the process
	mov		ebx,  [p_proc_current]
	mov		dword [ebx + ESP_SAVE_INT], esp
;	or		dword [ebx + SAVE_TYPE], 1		;set 1st-bit of save_type, added by xw, 17/12/04
	mov     dx, ss
	mov     ds, dx
	mov     es, dx
	mov		fs, dx							;value of fs and gs in user process is different to that in kernel
	mov		dx, SELECTOR_VIDEO - 2			;added by xw, 18/6/20
	mov		gs, dx

    mov     esi, esp
	mov     esp, StackTop   ;switches to the irq-stack from current process's kernel stack
	push    restart_int		;added by xw, 18/4/19
	jmp     [esi + RETADR - P_STACKBASE]
instack:						;already in the irq-stack
	push    restart_restore	;modified by xw, 18/4/19
;   jmp		[esp + RETADR - P_STACKBASE]
	jmp		[esp + 4 + RETADR - P_STACKBASE]	;modified by xw, 18/6/4


; can't modify EAX, for it contains syscall number
; can't modify EBX, for it contains the syscall argument
save_syscall:
	pushad          ; `.
	push    ds      ;  |
	push    es      ;  | 保存原寄存器值
	push    fs      ;  |
	push    gs      ; /
	mov	edx,  [p_proc_current]				;xw
	mov	dword [edx + ESP_SAVE_SYSCALL], esp	;xw save esp position in the kernel-stack of the process
	mov     dx, ss
	mov     ds, dx
	mov     es, dx
	mov	fs, dx							; value of fs and gs in user process is different to that in kernel
	mov	dx, SELECTOR_VIDEO - 2			; added by xw, 18/6/20
	mov	gs, dx
	mov     esi, esp
	push    restart_syscall		; modified by xw, 17/12/04
	jmp     [esi + RETADR - P_STACKBASE]

	global sched
sched:
;could be called by C function, you must save ebp, ebx, edi, esi,
;for C function assumes that they stay unchanged. added by xw, 18/4/19
;save_context
	pushfd
	pushad			;modified by xw, 18/6/4
	cli
	mov	ebx,  [p_proc_current]
	mov	dword [ebx + ESP_SAVE_CONTEXT], esp	;save esp position in the kernel-stack of the process
	; schedule
	call	schedule			;schedule is a C function, save eax, ecx, edx if you want them to stay unchanged.
	; prepare to run new process
	mov	ebx,  [p_proc_next]	;added by xw, 18/4/26
	mov	dword [p_proc_current], ebx
	call	renew_env			;renew process executing environment
	mov	ebx, [p_proc_current]
	mov 	esp, [ebx + ESP_SAVE_CONTEXT]		;switch to a new kernel stack
	popad
	popfd
	ret

; renew process executing environment. Added by xw, 18/4/19
renew_env:
	call	switch_pde		;to change the global variable cr3_ready
	mov 	eax,[cr3_ready]	;to switch the page directory table
	mov 	cr3,eax

	mov	eax, [p_proc_current]
	lldt	[eax + P_LDT_SEL]				;load LDT
	lea	ebx, [eax + INIT_STACK_SIZE]
	mov	dword [tss + TSS3_S_SP0], ebx	;renew esp0
	ret

	global syscall_handler
syscall_handler:
;get syscall number from eax
;syscall that's called gets its argument from pushed ebx
;so we can't modify eax and ebx in save_syscall
	call	save_syscall	;save registers and some other things. modified by xw, 17/12/11
	sti
	push 	ebx				;push the argument the syscall need
	call    [syscall_table + eax * 4]	;将参数压入堆栈后再调用函数			add by visual 2016.4.6
	add	esp, 4				;clear the argument in the stack, modified by xw, 17/12/11
	cli
	mov	edx, [p_proc_current]
	mov 	esi, [edx + ESP_SAVE_SYSCALL]
	mov     [esi + EAXREG - P_STACKBASE], eax	;the return value of C function is in EAX
	ret

; ====================================================================================
;				    restart
; ====================================================================================

restart_int:
	mov		eax, [p_proc_current]
	mov 	esp, [eax + ESP_SAVE_INT]
	cmp	   	dword [kernel_initial], 0
	jnz	    restart_restore
	call	sched
	jmp     restart_restore

restart_syscall:
	mov		eax, [p_proc_current]
	mov 	esp, [eax + ESP_SAVE_SYSCALL]
	call	sched
	jmp 	restart_restore

	global restart_restore
restart_restore:
	pop		gs
	pop		fs
	pop		es
	pop		ds
	popad
	add		esp, 4
	iretd

; to launch the first process in the os. added by xw, 18/4/19
	global restart_initial
restart_initial:
;	mov		eax, [p_proc_current]
;	lldt	[eax + P_LDT_SEL]
;	lea		ebx, [eax + INIT_STACK_SIZE]
;	mov		dword [tss + TSS3_S_SP0], ebx
	call	renew_env						;renew process executing environment
	mov		eax, [p_proc_current]
	mov 	esp, [eax + ESP_SAVE_INT]		;restore esp position
	jmp 	restart_restore

	global read_cr2
read_cr2:
	mov eax,cr2
	ret

	global refresh_page_cache
refresh_page_cache:
	mov eax,cr3
	mov cr3,eax
	ret

	global halt
halt:
	hlt

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
	mov esi, dword [ebp + 8]	; void *uesp
	mov edi, dword [ebp + 12]	; int order
	mov eax, dword [esi + edi * 4 + 4]
	pop edi
	pop esi
	pop ebp
	ret
