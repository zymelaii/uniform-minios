
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks		equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_get_pid       	equ 1 ;	//add by visual 2016.4.6
_NR_kmalloc       	equ 2 ;	//add by visual 2016.4.6
_NR_kmalloc_4k		equ 3 ;	//add by visual 2016.4.7
_NR_malloc      	equ 4 ;	//add by visual 2016.4.7
_NR_malloc_4k      	equ 5 ;	//add by visual 2016.4.7
_NR_free      		equ 6 ;	//add by visual 2016.4.7
_NR_free_4k      	equ 7 ;	//add by visual 2016.4.7
_NR_fork     		equ 8 ;	//add by visual 2016.4.8
_NR_pthread     	equ 9 ;	//add by visual 2016.4.11
_NR_udisp_int     	equ 10 ;	//add by visual 2016.5.16
_NR_udisp_str     	equ 11 ;	//add by visual 2016.5.16
_NR_exec     		equ 12 ;	//add by visual 2016.5.16
_NR_yield		equ 13 ;	//added by xw, 17/12
_NR_sleep		equ 14 ;	//added by xw, 17/12
_NR_print_E		equ 15 ;	//added by xw, 18/4/27
_NR_print_F		equ 16 ;	//added by xw, 18/4/27

_NR_open		equ 17 ;	//added by xw, 18/6/18
_NR_close		equ 18 ;	//added by xw, 18/6/18
_NR_read		equ 19 ;	//added by xw, 18/6/18
_NR_write		equ 20 ;	//added by xw, 18/6/18
_NR_lseek		equ 21 ;	//added by xw, 18/6/18
_NR_unlink		equ 22 ;	//added by xw, 18/6/18

_NR_create		equ 23 ;    //added by mingxuan 2019-5-17
_NR_delete 		equ 24 ;    //added by mingxuan 2019-5-17
_NR_opendir 		equ 25 ;    //added by mingxuan 2019-5-17
_NR_createdir  		equ 26 ;    //added by mingxuan 2019-5-17
_NR_deletedir   	equ 27 ;    //added by mingxuan 2019-5-17

INT_VECTOR_SYS_CALL	equ 0x90

; 导出符号
global	get_ticks
global	get_pid		;		//add by visual 2016.4.6
global	kmalloc		;		//add by visual 2016.4.6
global	kmalloc_4k	;		//add by visual 2016.4.7
global	malloc		;		//add by visual 2016.4.7
global	malloc_4k	;		//add by visual 2016.4.7
global	free		;		//add by visual 2016.4.7
global	free_4k		;		//add by visual 2016.4.7
global	fork		;		//add by visual 2016.4.8
global	pthread		;		//add by visual 2016.4.11
global	udisp_int	;		//add by visual 2016.5.16
global	udisp_str	;		//add by visual 2016.5.16
global	exec		;		//add by visual 2016.5.16
global  yield		;		//added by xw
global  sleep		;		//added by xw
global	print_E		;		//added by xw
global	print_F		;		//added by xw

global	open		;		//added by xw, 18/6/18
global	close		;		//added by xw, 18/6/18
global	read		;		//added by xw, 18/6/18
global	write		;		//added by xw, 18/6/18
global	lseek		;		//added by xw, 18/6/18
global	unlink		;		//added by xw, 18/6/19

global	create		;		//added by mingxuan 2019-5-17
global	delete		;		//added by mingxuan 2019-5-17
global  opendir		;		//added by mingxuan 2019-5-17
global	createdir	;		//added by mingxuan 2019-5-17
global  deletedir	;		//added by mingxuan 2019-5-17

bits 32
[section .text]
; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================
;                              get_pid		//add by visual 2016.4.6
; ====================================================================
get_pid:
	mov	eax, _NR_get_pid
	int	INT_VECTOR_SYS_CALL
	ret
	
; ; ====================================================================
; ;                              kmalloc		//add by visual 2016.4.6
; ; ====================================================================
; kmalloc:
; 	push	ebx
; 	mov	ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!
; 	mov	eax, _NR_kmalloc
; 	int	INT_VECTOR_SYS_CALL
; 	pop	ebx
; 	ret
	
; ; ====================================================================
; ;                              kmalloc_4k		//add by visual 2016.4.7
; ; ====================================================================
; kmalloc_4k:
; 	push	ebx
; 	mov	ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!111
; 	mov	eax, _NR_kmalloc_4k
; 	int	INT_VECTOR_SYS_CALL
; 	pop	ebx
; 	ret
	
; ; ====================================================================
; ;                              malloc		//add by visual 2016.4.7
; ; ====================================================================
; malloc:
; 	push	ebx
; 	mov	ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!111
; 	mov	eax, _NR_malloc
; 	int	INT_VECTOR_SYS_CALL
; 	pop	ebx
; 	ret
	
; ; ====================================================================
; ;                              malloc_4k		//add by visual 2016.4.7
; ; ====================================================================
; malloc_4k:
; 	push	ebx
; 	mov	ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!111
; 	mov	eax, _NR_malloc_4k
; 	int	INT_VECTOR_SYS_CALL
; 	pop	ebx
; 	ret

; ; ====================================================================
; ;                              free		//add by visual 2016.4.7
; ; ====================================================================
; free:
; 	push	ebx
; 	mov	ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!111
; 	mov	eax, _NR_free
; 	int	INT_VECTOR_SYS_CALL
; 	pop	ebx
; 	ret

; ; ====================================================================
; ;                              free_4k		//add by visual 2016.4.7
; ; ====================================================================
; free_4k:
; 	push	ebx
; 	mov	ebx,[esp+4] ; 将C函数调用时传来的参数放到ebx里!!111
; 	mov	eax, _NR_free_4k
; 	int	INT_VECTOR_SYS_CALL
; 	pop	ebx
; 	ret
	
; ====================================================================
;                              fork		//add by visual 2016.4.8
; ====================================================================
fork:
	mov	eax, _NR_fork
	int	INT_VECTOR_SYS_CALL
	ret
	
; ====================================================================
;                              pthread		//add by visual 2016.4.11
; ====================================================================
pthread:
	push	ebx
	mov	ebx,[esp+8]
	mov	eax, _NR_pthread
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret
	
; ====================================================================
;                              udisp_int		//add by visual 2016.5.16
; ====================================================================	
udisp_int:
	push	ebx
	mov	ebx,[esp+8]
	mov	eax, _NR_udisp_int
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret
	
; ====================================================================
;                              udisp_str		//add by visual 2016.5.16
; ====================================================================	
udisp_str:
	push	ebx
	mov	ebx,[esp+8]
	mov	eax, _NR_udisp_str
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

; ====================================================================
;                              exec		//add by visual 2016.5.16
; ====================================================================	
exec:
	push	ebx
	mov	ebx,[esp+8]
	mov	eax, _NR_exec
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

; ====================================================================
;                              yield	//added by xw
; ====================================================================	
yield:
	mov	eax, _NR_yield
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================
;                              sleep	//added by xw
; ====================================================================	
sleep:
	push	ebx
	mov	ebx,[esp+8]
	mov	eax, _NR_sleep
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

; ; ====================================================================
; ;                              print_E	//added by xw
; ; ====================================================================	
; print_E:
; 	push	ebx
; 	mov	ebx,[esp+4]
; 	mov	eax, _NR_print_E
; 	int	INT_VECTOR_SYS_CALL
; 	pop	ebx
; 	ret

; ; ====================================================================
; ;                              print_F	//added by xw
; ; ====================================================================	
; print_F:
; 	push	ebx
; 	mov	ebx,[esp+4]
; 	mov	eax, _NR_print_F
; 	int	INT_VECTOR_SYS_CALL
; 	pop	ebx
; 	ret

; ====================================================================
;                              open		//added by xw, 18/6/18
; ====================================================================	
; open has more than one parameter, to pass them, we save the esp to ebx, 
; and ebx will be passed into kernel as usual. In kernel, we use the saved
; esp in user space to get the number of parameters and the values of them.
open:
	push	ebx
	mov	ebx, esp
	mov	eax, _NR_open
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret
	
; ====================================================================
;                              close	//added by xw, 18/6/18
; ====================================================================	
; close has only one parameter, but we can still use this method.
close:
	push 	ebx			
	mov 	ebx, esp
	mov	eax, _NR_close
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

; ====================================================================
;                              read		//added by xw, 18/6/18
; ====================================================================
read:
	push	ebx			
	mov	ebx, esp
	mov	eax, _NR_read
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

; ====================================================================
;                              write		//added by xw, 18/6/18
; ====================================================================
write:
	push	ebx			
	mov	ebx, esp
	mov	eax, _NR_write
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

; ====================================================================
;                              lseek		//added by xw, 18/6/18
; ====================================================================
lseek:
	push	ebx			
	mov	ebx, esp
	mov	eax, _NR_lseek
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret
	
; ====================================================================
;                              unlink		//added by xw, 18/6/18
; ====================================================================
unlink:
	push	ebx			
	mov	ebx, esp
	mov	eax, _NR_unlink
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

;和FAT32有关的系统调用
create:
	push	ebx
	mov	ebx, esp
	mov	eax, _NR_create
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

delete:
	push	ebx
	mov	ebx, esp
	mov	eax, _NR_delete
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

opendir:
	push	ebx
	mov	ebx, esp
	mov	eax, _NR_opendir
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

createdir:
	push	ebx
	mov	ebx, esp
	mov	eax, _NR_createdir
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

deletedir:
	push	ebx
	mov	ebx, esp
	mov	eax, _NR_deletedir
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret