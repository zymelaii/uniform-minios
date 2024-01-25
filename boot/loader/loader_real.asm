%include "loader.inc"
%include "pm.inc"

extern ProtectStart

[bits 16]

[section .text.real]

    global _start
_start:
    ; 当 boot 将执行权交给 loader 时，它会将 dl 设置为使用的驱动器号，注意不要将其覆盖了
    ; 与此同时需要注意 ebx 指向的是启动分区对应 LBA 基地址，注意不要将其覆盖了
    mov        ax, cs
    mov        ds, ax
    mov        es, ax
    mov        ss, ax
    jmp        Init

LABEL_GDT:          Descriptor  0,        0, 0                              ; 空描述符
LABEL_DESC_FLAT_C:  Descriptor  0,  0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K   ; 0 ~ 4G
LABEL_DESC_FLAT_RW: Descriptor  0,  0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K   ; 0 ~ 4G

GdtLen  equ     $ - LABEL_GDT
GdtPtr  dw      GdtLen - 1                      ; 段界限
        dd      BaseOfLoaderPhyAddr + LABEL_GDT ; 基地址
Init:
    mov     sp, StackTop
    ; 将 PartitionLBA 变量先存栈里
    push    ebx

    ; 清屏
    mov     ax, 0600h    ; AH = 6,  AL = 0h
    mov     bx, 0700h    ; BL = 07h 黑底白字
    mov     cx, 0        ; 左上角: (0, 0)
    mov     dx, 0184fh   ; 右下角: (80, 50)
    int     10h

GetDeviceInfo:
    mov     ax, BaseOfDeviceInfo
    mov     ds, ax
    mov     es, ax
    ; 显卡信息
    mov     ah, 0fh
    int     10h
    mov     [OffsetOfVideoCard + 0], bx    ; bh = display page
    mov     [OffsetOfVideoCard + 2], ax    ; al = video mode, ah = window width
    ; ega/vga 信息
    mov     ah, 12h
    mov     bl, 10h
    int     10h
    mov     [OffsetOfVGAInfo + 0], ax
    mov     [OffsetOfVGAInfo + 2], bx
    mov     [OffsetOfVGAInfo + 4], cx
    ; 内存信息
    mov     ebx, 0
    mov     dword [OffsetOfARDSCount], 0
    mov     di, OffsetOfARDSBuffer
.MemChk:
    mov     eax, 0E820h        ; eax = 0000E820h
    mov     ecx, 20            ; ecx = 地址范围描述符结构的大小
    mov     edx, 0534D4150h    ; edx = 'SMAP'
    int     15h
    jc      .MemChkFail
    add     di, 20
    inc     dword [OffsetOfARDSCount]
    cmp     ebx, 0
    jnz     .MemChk
    jmp     .MemChkOK
.MemChkFail:
    mov     dword [OffsetOfARDSCount], 0
.MemChkOK:
    mov     ax, cs
    mov     ds, ax
    mov     es, ax

InitProtectMode:
    ; 将 PartitionLBA 变量恢复
    pop     ebx
    lgdt    [GdtPtr]
    cli
    ; 打开地址线 A20
    in      al, 92h
    or      al, 00000010b
    out     92h, al
    ; 准备切换到保护模式
    mov     eax, cr0
    or      eax, 1
    mov     cr0, eax
    ; 真正进入保护模式
    jmp     dword SelectorFlatC:ProtectStart

Stack times 100h db 0
StackTop:
