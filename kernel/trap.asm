%include "sconst.inc"

extern k_reenter
extern irq_table
extern save_exception
extern save_int

[section .text]

; impl_hwint_master <irq-name>, <irq-id>
%macro impl_hwint_master 2
    global %1
    align 16
%1:
    call save_int		        ; save context
    inc  dword [k_reenter]      ; mark as entering kernel
    in	al, INT_M_CTLMASK	    ; mask current int -->
    or	al, (1 << %2)		    ;
    out	INT_M_CTLMASK, al	    ; <--
    mov	al, EOI			        ; set master EOI -->
    out	INT_M_CTL, al		    ; <--
    sti				            ; enable respond to new int
    push %2				        ; run int handler -->
    call [irq_table + 4 * %2]	;
    pop	ecx			            ; <--
    cli
    dec dword [k_reenter]
    in	al, INT_M_CTLMASK	    ; unmask current int -->
    and	al, ~(1 << %2)		    ;
    out	INT_M_CTLMASK, al   	; <--
    ret
%endmacro

; impl_hwint_slave <irq-name>, <irq-id>
%macro impl_hwint_slave 2
    global %1
    align 16
%1:
    call save_int			    ; save context
    inc  dword [k_reenter]      ; mark as entering kernel
    in	al, INT_S_CTLMASK	    ; mask current int -->
    or	al, (1 << (%2 - 8))		;
    out	INT_S_CTLMASK, al	    ; <--
    mov	al, EOI				    ; set master & slave EOI -->
    out	INT_M_CTL, al		    ;
    nop						    ;
    out	INT_S_CTL, al		    ; <--
    sti						    ; enable respond to new int
    push %2						; run int handler -->
    call [irq_table + 4 * %2]	;
    pop	ecx						; <--
    cli
    dec dword [k_reenter]
    in	al, INT_S_CTLMASK		; unmask current int -->
    and	al, ~(1 << (%2 - 8))	;
    out	INT_S_CTLMASK, al		; <--
    ret
%endmacro

; impl_exception_no_errcode <exception-name>, <vec-no>, <handler>
%macro impl_exception_no_errcode 3
    global %1
%1:
    push	0xffffffff		    ; no err code
    call	save_exception      ; save registers and some other things.
    mov	    esi, esp		    ; esp points to pushed address of restart_exception at present
    add	    esi, 4 * 17		    ; we use esi to help to fetch arguments of exception handler from the stack.
                                ; 17 is calculated by: 4+8+retaddr+errcode+eip+cs+eflag=17
    mov	    eax, [esi]		    ; saved eflags
    push	eax
    mov	    eax, [esi - 4]		; saved cs
    push	eax
    mov	    eax, [esi - 4 * 2]	; saved eip
    push	eax
    mov	    eax, [esi - 4 * 3]	; saved err code
    push	eax
    push	%2			        ; vector_no
    sti
    call	%3
    cli
    add	    esp, 4 * 5		    ; clear arguments of exception handler in stack
    ret				            ; returned to 'restart_exception' procedure
%endmacro

; impl_exception_errcode <exception-name>, <vec-no>, <handler>
%macro impl_exception_errcode 3
    global %1
%1:
    call	save_exception		; save registers and some other things.
    mov		esi, esp	        ; esp points to pushed address of restart_exception at present
    add		esi, 4 * 17	        ; we use esi to help to fetch arguments of exception handler from the stack.
                                ; 17 is calculated by: 4+8+retaddr+errcode+eip+cs+eflag=17
    mov	    eax, [esi]		    ; saved eflags
    push	eax
    mov	    eax, [esi - 4]		; saved cs
    push	eax
    mov	    eax, [esi - 4 * 2]	; saved eip
    push	eax
    mov	    eax, [esi - 4 * 3]	; saved err code
    push	eax
    push	%2			        ; vector_no
    sti
    call	%3
    cli
    add	    esp, 4 * 5	        ; clear arguments of exception handler in stack
    ret				            ; returned to 'restart_exception' procedure
%endmacro

impl_hwint_master hwint00, 0  ; interrupt routine for irq 0 (the clock)
impl_hwint_master hwint01, 1  ; interrupt routine for irq 1 (keyboard)
impl_hwint_master hwint02, 2  ; interrupt routine for irq 2 (cascade)
impl_hwint_master hwint03, 3  ; interrupt routine for irq 3 (second serial)
impl_hwint_master hwint04, 4  ; interrupt routine for irq 4 (first serial)
impl_hwint_master hwint05, 5  ; interrupt routine for irq 5 (XT winchester)
impl_hwint_master hwint06, 6  ; interrupt routine for irq 6 (floppy)
impl_hwint_master hwint07, 7  ; interrupt routine for irq 7 (printer)
impl_hwint_slave  hwint08, 8  ; interrupt routine for irq 8 (realtime clock)
impl_hwint_slave  hwint09, 9  ; interrupt routine for irq 9 (irq 2 redirected)
impl_hwint_slave  hwint10, 10 ; interrupt routine for irq 10
impl_hwint_slave  hwint11, 11 ; interrupt routine for irq 11
impl_hwint_slave  hwint12, 12 ; interrupt routine for irq 12
impl_hwint_slave  hwint13, 13 ; interrupt routine for irq 13 (fpu exception)
impl_hwint_slave  hwint14, 14 ; interrupt routine for irq 14 (AT winchester)
impl_hwint_slave  hwint15, 15 ; interrupt routine for irq 15

extern division_error_handler
extern exception_handler
extern page_fault_handler

impl_exception_no_errcode division_error,           0, division_error_handler ; division error, fault, #DE, no error code
impl_exception_no_errcode debug_exception,          1, exception_handler      ; debug exception, fault/trap, #DB, no error code
impl_exception_no_errcode nmi,                      2, exception_handler      ; non-maskable interrupt, int, \, no error code
impl_exception_no_errcode breakpoint_exception,     3, exception_handler      ; breakpoint exception, trap, #BP, no error code
impl_exception_no_errcode overflow_exception,       4, exception_handler      ; overflow exception, trap, #OF, no error code
impl_exception_no_errcode bound_range_exceeded,     5, exception_handler      ; bound range exceeded exception, fault, #BR, no error code
impl_exception_no_errcode invalid_opcode,           6, exception_handler      ; invalid opcode, fault, #UD, no error code
impl_exception_no_errcode device_not_available,     7, exception_handler      ; device not available, fault, #NM, no error code
impl_exception_errcode    double_fault,             8, exception_handler      ; double fault, abort, #DF, error code = 0
impl_exception_no_errcode copr_seg_overrun,         9, exception_handler      ; coprocessor segment overrun, fault, \, no error code
impl_exception_errcode    invalid_tss,              10, exception_handler     ; invalid tss, fault, #TS, error code
impl_exception_errcode    segment_not_present,      11, exception_handler     ; segment not present, fault, #NP, error code
impl_exception_errcode    stack_seg_exception,      12, exception_handler     ; stack-segment fault, fault, #SS, error code
impl_exception_errcode    general_protection,       13, exception_handler     ; general protection fault, fault, #PF, error code
impl_exception_errcode    page_fault,               14, page_fault_handler    ; page fault, fault, #PF, error code
impl_exception_no_errcode floating_point_exception, 16, exception_handler     ; floating-point exception, fault, #MF, no error code
