%include "mbr.inc"
%include "boot.inc"
%include "packet.inc"

    org OffsetOfMBR
Start:
    ; 当 bios 启动时，它会将 dl 设置为使用的驱动器号，注意不要将其覆盖了
    mov    ax, cs
    mov    ds, ax
    mov    es, ax
    mov    ss, ax

    ; 将代码迁移到 BaseOfMBR 去
    cld
    mov    cx, 200h
    mov    si, OffsetOfBoot
    mov    di, OffsetOfMBR
    rep    movsb

    jmp    BaseOfMBR:Init

;============================================================================
;变量
;----------------------------------------------------------------------------
DriverNumber              db 0                 ; 驱动器号的临时存储位置
;============================================================================
;字符串
;----------------------------------------------------------------------------
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength             equ   11
BootMessage:              db    "Find MBR   "
Message1                  db    "Ready.     "
Message2                  db    "Read Fail  "
Message3                  db    "No Bootable"
;============================================================================


;----------------------------------------------------------------------------
; 函数名: DispStr
;----------------------------------------------------------------------------
; 作用:
;    显示一个字符串, 函数开始时 dh 中应该是字符串序号（从 0 开始）
DispStr:
    pusha
    push    es

    mov     ax, MessageLength
    mul     dh
    add     ax, BootMessage
    mov     bp, ax
    mov     ax, ds
    mov     es, ax            ; ES:BP = 串地址
    mov     cx, MessageLength ; CX = 串长度
    mov     ax, 01301h        ; AH = 13,  AL = 01h
    mov     bx, 0007h         ; BH = 0, BL = 07h 页号为 0 黑底白字
    mov     dl, 0
    int     10h

    pop     es
    popa
    ret

;----------------------------------------------------------------------------
; 函数名: ReadSector
;----------------------------------------------------------------------------
; 作用:
;    将磁盘的数据读入到内存中
;    eax: 从哪个扇区开始
;    cx: 读入多少个扇区
;    (es:bx): 读入的缓冲区的起始地址
ReadSector:
    pushad
    sub     sp, SizeOfPacket

    mov     si, sp
    mov     word [si + Packet_BufferPacketSize], SizeOfPacket
    mov     word [si + Packet_Sectors], cx
    mov     word [si + Packet_BufferOffset], bx
    mov     word [si + Packet_BufferSegment], es
    mov     dword [si + Packet_StartSectors], eax
    mov     dword [si + Packet_StartSectors + 4], 0

    mov     dl, [DriverNumber]
    mov     ah, 42h         ; 扩展读
    int     13h
    jc      .ReadFail       ; 读取失败，简单考虑就默认bios坏了

    add     sp, SizeOfPacket
    popad
    ret

.ReadFail:
    mov     dh, 2
    call    DispStr
    jmp     $               ; 如果 cf 位置 1，就意味着读入错误，这个时候建议直接开摆

Init:
    ; 临时存放设备号
    mov     [DriverNumber], dl
    mov     sp, OffsetOfMBR
    ; 清屏
    mov     ax, 0600h       ; AH = 6,  AL = 0h
    mov     bx, 0700h       ; BL = 07h 黑底白字
    mov     cx, 0           ; 左上角: (0, 0)
    mov     dx, 0184fh      ; 右下角: (80, 50)
    int     10h

    mov     dh, 0
    call    DispStr

    mov     si, OffsetOfMBR + StartOfMBRTable

CheckPartition:
    mov     dh, [si + MBRPartitionState]    ; 获取分区表状态
    cmp     dh, 80h                         ; 是否是 bootable 的
    jz      FoundBootablePartition
    add     si, MBRPartitionEntrySize
    cmp     si, OffsetOfMBR + EndOfMBRTable ; 检测是否遍历完
    jnz     CheckPartition

NotFoundBootablePartition:
    mov     dh, 3
    call    DispStr
    jmp     $

FoundBootablePartition:
    mov     dh, 1
    call    DispStr

    mov     cx, 1
    mov     eax, [si + MBRFirstLBA]
    mov     bx, BaseOfBoot
    mov     es, bx
    mov     bx, OffsetOfBoot
    call    ReadSector
    ; 检查是否是可引导扇区
    mov     ax, [es:OffsetOfBoot + 510]
    cmp     ax, 0xaa55
    jnz     NotFoundBootablePartition
    ; 将执行流交给 boot 前获取设备号
    mov     dl, [DriverNumber]
    ; 此时 ds:si 也指向了正在启动的 mbr 表项
    jmp     BaseOfBoot:OffsetOfBoot

times SizeOfMBR-($-$$) db 0
