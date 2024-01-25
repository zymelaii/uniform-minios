%include "loader.inc"

[bits 32]
[section .text]
global ProtectStart
ProtectStart:
	mov		ax, SelectorFlatRW
	mov		ds, ax
	mov		ss, ax
	mov		es, ax
	mov		fs, ax
	mov		gs, ax
	mov		esp, StackTopInProtect
	mov		[PartitionLBA], ebx

SetupPaging:
	;启动页表机制
extern setup_paging
	call	setup_paging
	mov		cr3, eax
	mov		eax, cr0
	or		eax, 80000000h
	mov		cr0, eax
LoadKernel:
extern load_kernel
	call	load_kernel
	; 返回值是要跳入的kernel entry
	jmp		eax

[section .data]
global	PartitionLBA
PartitionLBA	dd 0
