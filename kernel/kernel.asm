
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               kernel.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


%include "sconst.inc"

; 导入函数
extern	cstart
extern	kernel_main
extern	exception_handler
extern	spurious_irq
extern	clock_handler
extern	disp_str
extern	irq_table
extern	page_fault_handler
extern	divide_error_handler	;added by xw, 18/12/22
extern  schedule
extern  switch_pde

; 导入全局变量
extern	gdt_ptr
extern	idt_ptr
extern	p_proc_current
extern	tss
extern	k_reenter
extern	sys_call_table
extern 	cr3_ready			;add by visual 2016.4.5
extern  p_proc_current
extern	p_proc_next			;added by xw, 18/4/26
extern	kernel_initial		;added by xw, 18/6/10

bits 32

[SECTION .bss]
StackSpace		resb	2 * 1024
StackTop:		; used only as irq-stack in minios. added by xw

; added by xw, 18/6/15
KernelStackSpace	resb	2 * 1024
KernelStackTop:	; used as stack of kernel itself
; ~xw

[section .text]	; 代码在此

global _start	; 导出 _start

;global restart
global restart_initial	;Added by xw, 18/4/21
global restart_restore	;Added by xw, 18/4/21
global sched			;Added by xw, 18/4/21
global sys_call
global read_cr2   ;//add by visual 2016.5.9
global refresh_page_cache ; // add by visual 2016.5.12
global halt  			;added by xw, 18/6/11
global get_arg			;added by xw, 18/6/18

global	divide_error
global	single_step_exception
global	nmi
global	breakpoint_exception
global	overflow
global	bounds_check
global	inval_opcode
global	copr_not_available
global	double_fault
global	copr_seg_overrun
global	inval_tss
global	segment_not_present
global	stack_exception
global	general_protection
global	page_fault
global	copr_error
global	hwint00
global	hwint01
global	hwint02
global	hwint03
global	hwint04
global	hwint05
global	hwint06
global	hwint07
global	hwint08
global	hwint09
global	hwint10
global	hwint11
global	hwint12
global	hwint13
global	hwint14
global	hwint15


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

	jmp	SELECTOR_KERNEL_CS:csinit
csinit:		; “这个跳转指令强制使用刚刚初始化的结构”——<<OS:D&I 2nd>> P90.

	xor	eax, eax
	mov	ax, SELECTOR_TSS
	ltr	ax

	jmp	kernel_main


; 中断和异常 -- 硬件中断
; ---------------------------------
%macro	hwint_master	1
	;call save
	call save_int		;save registers and some other things. modified by xw, 17/12/11
	inc  dword [k_reenter]  ;If k_reenter isn't equal to 0, there is no switching to the irq-stack, 
					;which is performed in save_int. Added by xw, 18/4/21
	
	in	al, INT_M_CTLMASK	; `.
	or	al, (1 << %1)		;  | 屏蔽当前中断
	out	INT_M_CTLMASK, al	; /
	
	mov	al, EOI			; `. 置EOI位
	out	INT_M_CTL, al		; /
	
	sti				; CPU在响应中断的过程中会自动关中断，这句之后就允许响应新的中断
	push %1				; `.
	call [irq_table + 4 * %1]	;  | 中断处理程序
	pop	ecx			; /								; 	┛
	
	cli
	dec dword [k_reenter]
	
	in	al, INT_M_CTLMASK	; `.
	and	al, ~(1 << %1)		;  | 恢复接受当前中断
	out	INT_M_CTLMASK, al	; /
	ret
%endmacro


ALIGN	16
hwint00:		; Interrupt routine for irq 0 (the clock).
	hwint_master	0

ALIGN	16
hwint01:		; Interrupt routine for irq 1 (keyboard)
	hwint_master	1

ALIGN	16
hwint02:		; Interrupt routine for irq 2 (cascade!)
	hwint_master	2

ALIGN	16
hwint03:		; Interrupt routine for irq 3 (second serial)
	hwint_master	3

ALIGN	16
hwint04:		; Interrupt routine for irq 4 (first serial)
	hwint_master	4

ALIGN	16
hwint05:		; Interrupt routine for irq 5 (XT winchester)
	hwint_master	5

ALIGN	16
hwint06:		; Interrupt routine for irq 6 (floppy)
	hwint_master	6

ALIGN	16
hwint07:		; Interrupt routine for irq 7 (printer)
	hwint_master	7

; ---------------------------------
%macro	hwint_slave	1
	;added by xw, 18/5/29
	call save_int			;save registers and some other things. 
	inc  dword [k_reenter]  ;If k_reenter isn't equal to 0, there is no switching to the irq-stack, 
							;which is performed in save_int. Added by xw, 18/4/21
	
	in	al, INT_S_CTLMASK	; `.
	or	al, (1 << (%1 - 8))		;  | 屏蔽当前中断
	out	INT_S_CTLMASK, al	; /
	mov	al, EOI				; `.
	out	INT_M_CTL, al		; / 置EOI位(master)
	nop						; `.一定注意：slave和master都要置EOI	
	out	INT_S_CTL, al		; / 置EOI位(slave)
	
	sti						; CPU在响应中断的过程中会自动关中断，这句之后就允许响应新的中断
	push %1						; `.
	call [irq_table + 4 * %1]	;  | 中断处理程序
	pop	ecx						; /
	
	cli
	dec dword [k_reenter]
	in	al, INT_S_CTLMASK		; `.
	and	al, ~(1 << (%1 - 8))	;  | 恢复接受当前中断
	out	INT_S_CTLMASK, al		; /
	ret
	;~xw
%endmacro
; ---------------------------------

ALIGN	16
hwint08:		; Interrupt routine for irq 8 (realtime clock).
	hwint_slave	8

ALIGN	16
hwint09:		; Interrupt routine for irq 9 (irq 2 redirected)
	hwint_slave	9

ALIGN	16
hwint10:		; Interrupt routine for irq 10
	hwint_slave	10

ALIGN	16
hwint11:		; Interrupt routine for irq 11
	hwint_slave	11

ALIGN	16
hwint12:		; Interrupt routine for irq 12
	hwint_slave	12

ALIGN	16
hwint13:		; Interrupt routine for irq 13 (FPU exception)
	hwint_slave	13

ALIGN	16
hwint14:		; Interrupt routine for irq 14 (AT winchester)
	hwint_slave	14

ALIGN	16
hwint15:		; Interrupt routine for irq 15
	hwint_slave	15

; ====================================================================================
;                                   exception
; ====================================================================================
;restructured exception-handling procedure
;added by xw, 18/12/18
%macro	exception_no_errcode	2
	push	0xFFFFFFFF		;no err code
	call	save_exception		;save registers and some other things. 
	mov	esi, esp		;esp points to pushed address of restart_exception at present
	add	esi, 4 * 17		;we use esi to help to fetch arguments of exception handler from the stack.
					;17 is calculated by: 4+8+retaddr+errcode+eip+cs+eflag=17
	mov	eax, [esi]		;saved eflags
	push	eax
	mov	eax, [esi - 4]		;saved cs
	push	eax
	mov	eax, [esi - 4 * 2]	;saved eip
	push	eax
	mov	eax, [esi - 4 * 3]	;saved err code
	push	eax
	push	%1			;vector_no
	sti
	call	%2
	cli
	add	esp, 4 * 5		;clear arguments of exception handler in stack
	ret				;returned to 'restart_exception' procedure
%endmacro

%macro	exception_errcode	2
	call	save_exception		;save registers and some other things.
	mov		esi, esp	;esp points to pushed address of restart_exception at present
	add		esi, 4 * 17	;we use esi to help to fetch arguments of exception handler from the stack.
					;17 is calculated by: 4+8+retaddr+errcode+eip+cs+eflag=17
	mov	eax, [esi]		;saved eflags
	push	eax
	mov	eax, [esi - 4]		;saved cs
	push	eax
	mov	eax, [esi - 4 * 2]	;saved eip
	push	eax
	mov	eax, [esi - 4 * 3]	;saved err code
	push	eax
	push	%1			;vector_no
	sti
	call	%2
	cli
	add	esp, 4 * 5		;clear arguments of exception handler in stack
	ret				;returned to 'restart_exception' procedure
%endmacro

divide_error:					; vector_no	= 0
;	exception_no_errcode	0, exception_handler
	exception_no_errcode	0, divide_error_handler	;added by xw, 18/12/22

single_step_exception:			; vector_no	= 1
	exception_no_errcode	1, exception_handler

nmi:							; vector_no	= 2
	exception_no_errcode	2, exception_handler
	
breakpoint_exception:			; vector_no	= 3
	exception_no_errcode	3, exception_handler
	
overflow:						; vector_no	= 4
	exception_no_errcode	4, exception_handler
	
bounds_check:					; vector_no	= 5
	exception_no_errcode	5, exception_handler
	
inval_opcode:					; vector_no	= 6
	exception_no_errcode	6, exception_handler
	
copr_not_available:				; vector_no	= 7
	exception_no_errcode	7, exception_handler
	
double_fault:					; vector_no	= 8
	exception_errcode	8, exception_handler
	
copr_seg_overrun:				; vector_no	= 9
	exception_no_errcode	9, exception_handler
	
inval_tss:						; vector_no	= 10
	exception_errcode	10, exception_handler
	
segment_not_present:			; vector_no	= 11
	exception_errcode	11, exception_handler
	
stack_exception:				; vector_no	= 12
	exception_errcode	12, exception_handler
	
general_protection:				; vector_no	= 13
	exception_errcode	13, exception_handler
	
page_fault:						; vector_no	= 14
	exception_errcode	14, page_fault_handler
	
copr_error:						; vector_no	= 16
	exception_no_errcode	16, exception_handler

;environment saving when an exception occurs
;added by xw, 18/12/18
save_exception:
	pushad          ; `.
	push    ds      ;  |
	push    es      ;  | 保存原寄存器值
	push    fs      ;  |
	push    gs      ; /
	mov     dx, ss
	mov     ds, dx
	mov     es, dx
	mov	fs, dx			;value of fs and gs in user process is different to that in kernel
	mov	dx, SELECTOR_VIDEO - 2	;added by xw, 18/6/20
	mov	gs, dx

	mov     esi, esp                     
	push    restart_exception
	jmp     [esi + RETADR - P_STACKBASE]	;the err code is in higher address than retaddr in stack, so there is
						;no need to modify the position jumped to, that is, 
						;"jmp [esi + RETADR - 4 - P_STACKBASE]" is actually wrong.

;added by xw, 18/12/18
restart_exception:
	call	sched
	pop	gs
	pop	fs
	pop	es
	pop	ds
	popad
	add	esp, 4 * 2	;clear retaddr and error code in stack
	iretd
	
; ====================================================================================
;                                   save
; ====================================================================================
save_int:
        pushad          ; `.
        push    ds      ;  |
        push    es      ;  | 保存原寄存器值
        push    fs      ;  |
        push    gs      ; /
		
	cmp	dword [k_reenter], 0			;Added by xw, 18/4/19
	jnz	instack
	
	mov	ebx,  [p_proc_current]			;xw
	mov	dword [ebx + ESP_SAVE_INT], esp	;xw save esp position in the kernel-stack of the process
;		or		dword [ebx + SAVE_TYPE], 1		;set 1st-bit of save_type, added by xw, 17/12/04
        mov     dx, ss
        mov     ds, dx
        mov     es, dx
	mov	fs, dx							;value of fs and gs in user process is different to that in kernel
	mov	dx, SELECTOR_VIDEO - 2			;added by xw, 18/6/20
	mov	gs, dx

        mov     esi, esp  		                                        
	mov     esp, StackTop   ;switches to the irq-stack from current process's kernel stack 
	push    restart_int		;added by xw, 18/4/19
	jmp     [esi + RETADR - P_STACKBASE]
instack:						;already in the irq-stack
	push    restart_restore	;modified by xw, 18/4/19
;		jmp		[esp + RETADR - P_STACKBASE]
	jmp		[esp + 4 + RETADR - P_STACKBASE]	;modified by xw, 18/6/4
                             

save_syscall:			;can't modify EAX, for it contains syscall number
				;can't modify EBX, for it contains the syscall argument
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
	mov	fs, dx							;value of fs and gs in user process is different to that in kernel
	mov	dx, SELECTOR_VIDEO - 2			;added by xw, 18/6/20
	mov	gs, dx

        mov     esi, esp                    
	push    restart_syscall		;modified by xw, 17/12/04
	jmp     [esi + RETADR - P_STACKBASE]
;modified end

; ====================================================================================
;                                sched(process switch)
; ====================================================================================
sched:
;could be called by C function, you must save ebp, ebx, edi, esi, 
;for C function assumes that they stay unchanged. added by xw, 18/4/19

;save_context
	pushfd
	pushad			;modified by xw, 18/6/4
	cli
	mov	ebx,  [p_proc_current]				
	mov	dword [ebx + ESP_SAVE_CONTEXT], esp	;save esp position in the kernel-stack of the process
;schedule
	call	schedule			;schedule is a C function, save eax, ecx, edx if you want them to stay unchanged.
;prepare to run new process
	mov	ebx,  [p_proc_next]	;added by xw, 18/4/26
	mov	dword [p_proc_current], ebx
	call	renew_env			;renew process executing environment
	mov	ebx, [p_proc_current]
	mov 	esp, [ebx + ESP_SAVE_CONTEXT]		;switch to a new kernel stack
	popad
	popfd
	ret
; ====================================================================================
;                        			renew_env
; ====================================================================================
;renew process executing environment. Added by xw, 18/4/19
renew_env:
	call	switch_pde		;to change the global variable cr3_ready
	mov 	eax,[cr3_ready]	;to switch the page directory table
	mov 	cr3,eax

	mov	eax, [p_proc_current]
	lldt	[eax + P_LDT_SEL]				;load LDT
	lea	ebx, [eax + INIT_STACK_SIZE]
	mov	dword [tss + TSS3_S_SP0], ebx	;renew esp0
	ret

; ====================================================================================
;                                 sys_call
; ====================================================================================
sys_call:
;get syscall number from eax
;syscall that's called gets its argument from pushed ebx
;so we can't modify eax and ebx in save_syscall
	call	save_syscall	;save registers and some other things. modified by xw, 17/12/11
	sti
	push 	ebx				;push the argument the syscall need
	call    [sys_call_table + eax * 4]	;将参数压入堆栈后再调用函数			add by visual 2016.4.6
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
	mov	eax, [p_proc_current]
	mov 	esp, [eax + ESP_SAVE_INT]		;switch back to the kernel stack from the irq-stack	
	cmp	dword [kernel_initial], 0		;added by xw, 18/6/10
	jnz	restart_restore
	call	sched	;save current process's context, invoke schedule(), and then
			;switch to the chosen process's kernel stack and restore it's context
			;added by xw, 18/4/19
;	call	renew_env
	jmp     restart_restore

restart_syscall:
	mov	eax, [p_proc_current]
	mov 	esp, [eax + ESP_SAVE_SYSCALL]	;xw	restore esp position
	call	sched							;added by xw, 18/4/26
	jmp 	restart_restore

;xw	restart_reenter:
restart_restore:
;	dec		dword [k_reenter]
	pop	gs
	pop	fs
	pop	es
	pop	ds
	popad
	add	esp, 4
	iretd

;to launch the first process in the os. added by xw, 18/4/19
restart_initial:
;	mov		eax, [p_proc_current]
;	lldt	[eax + P_LDT_SEL]
;	lea		ebx, [eax + INIT_STACK_SIZE]
;	mov		dword [tss + TSS3_S_SP0], ebx
	call	renew_env						;renew process executing environment
	mov	eax, [p_proc_current]
	mov 	esp, [eax + ESP_SAVE_INT]		;restore esp position
	jmp 	restart_restore

; ====================================================================================
;				    read_cr2				//add by visual 2016.5.9
; ====================================================================================	
read_cr2:
	mov eax,cr2
	ret
	
; ====================================================================================
;				    refresh_page_cache		//add by visual 2016.5.12
; ====================================================================================	
refresh_page_cache:
	mov eax,cr3
	mov cr3,eax
	ret
	
; ====================================================================================
;				    halt					//added by xw, 18/6/11			
; ====================================================================================
halt:
	hlt
	
; ====================================================================================
;				    u32 get_arg(void *uesp, int order)		//added by xw, 18/6/18			
; ====================================================================================
; used to get the specified argument of the syscall from user space stack
; @uesp: user space stack pointer
; @order: which argument you want to get
; @uesp+0: the number of args, @uesp+8: the first arg, @uesp+12: the second arg...
get_arg:
	push ebp
	mov ebp, esp
	push esi
	push edi
	mov esi, dword [ebp + 8]	;void *uesp
	mov edi, dword [ebp + 12]	;int order
	mov eax, dword [esi + edi * 4 + 4]
	pop edi
	pop esi
	pop ebp
	ret