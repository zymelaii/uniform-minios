%include "mbr.inc"
%include "boot.inc"
%include "loader.inc"
%include "fat32.inc"
%include "packet.inc"

OffsetOfBPB equ OffsetOfBoot

    org EndOfBPB
; 当 MBR 将执行权交给 boot 时，它会将 dl 设置为使用的驱动器号，注意不要将其覆盖了
; 与此同时需要注意 ds:si 指向的是启动分区对应的MBR项，需要先将其拷贝进来
Start:
    mov     ax, cs
    mov     ebx, [ds:si + MBRFirstLBA]
    mov     ds, ax
    mov     ss, ax
    mov     es, ax
    ; 初始化其余段寄存器，在初始化完之后才能正确访存
    jmp     Init

;============================================================================
;变量
;----------------------------------------------------------------------------
DriverNumber    db 0        ; 驱动器号的临时存储位置
EndOfBuffer     dw 0        ; buffer 段的末尾地址
SecOfDataZone   dd 0        ; 数据区基扇区号
PartitionLBA    dd 0        ; 分区起始 LBA
;============================================================================
;字符串
;----------------------------------------------------------------------------
LoaderName                db    "LOADER  BIN"
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength             equ   9
Message0                  db    "Read Fail"
Message1                  db    "No Loader"
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
    add     ax, Message0
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
    add     eax, [PartitionLBA]
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
    mov     dh, 0
    call    DispStr
    jmp     $               ; 如果 cf 位置 1，就意味着读入错误，这个时候建议直接开摆

;----------------------------------------------------------------------------
; 函数名: ReadCluster
;----------------------------------------------------------------------------
; 作用:
;    读取簇号对应的一整个簇
;    eax: 当前簇号
;    (es:bx): 读入的缓冲区的起始地址
ReadCluster:
    pushad

    sub     eax, CLUSTER_Base
    movzx   ecx, byte [BPB_SecPerClus]
    mul     ecx
    add     eax, [SecOfDataZone]
    call    ReadSector

    popad
    ret

;----------------------------------------------------------------------------
; 函数名: NextCluster
;----------------------------------------------------------------------------
; 作用:
;    获取下一个簇号，buffer 段内数据也会修改
;    eax: 当前簇号，而且返回时 eax 变为下一个簇号
NextCluster:
    pushad
    push    es

    mov     cx, BaseOfBootBuffer
    mov     es, cx
    shl     eax, 2
    mov     di, ax
    shr     eax, 9
    movzx   ecx, word [BPB_RsvdSecCnt]
    add     eax, ecx
    mov     cx, 1
    mov     bx, OffsetOfBootBuffer
    call    ReadSector

    and     di, 511
    mov     eax, [es:di+bx]
    and     eax, CLUSTER_Mask

    pop     es
    mov     bx, sp
    mov     [bx + 28], eax

    popad
    ret

Init:
    ; 临时存放设备号
    mov     [DriverNumber], dl
    mov     [PartitionLBA], ebx
    mov     sp, OffsetOfBoot

    ; es buffer 段基址
    ; di 指向 buffer 段中当前文件项的地址
    ; eax 当前根目录区簇号
FindLoaderInit:
    movzx   ax, byte [BPB_SecPerClus]
    mul     word [BPB_BytsPerSec]
    add     ax, OffsetOfBootBuffer
    mov     [EndOfBuffer], ax

    movzx   eax, byte [BPB_NumFATs]
    mul     dword [BPB_FATSz32]
    movzx   edx, word [BPB_RsvdSecCnt]
    add     eax, edx
    mov     [SecOfDataZone], eax

    mov     ax, BaseOfBootBuffer
    mov     es, ax
    mov     eax, [BPB_RootClus]
LoadDirCluster:
    mov     bx, OffsetOfBootBuffer
    call    ReadCluster

    mov     di, bx
FindLoader:
    push    di
    ; 由于 DIR_Name 为 0，就不用再加了
    mov     si, LoaderName
    mov     ecx, 11
    repe    cmpsb
    jz      LoaderFound

    pop     di
    add     di, SizeOfDIR
    cmp     di, [EndOfBuffer]
    jnz     FindLoader

    call    NextCluster
    cmp     eax, CLUSTER_Last
    jnz     LoadDirCluster

LoaderNotFound:
    mov     dh, 1
    call    DispStr
    jmp     $

    ; es loader 段基址
    ; bx loader 段偏移
    ; dx 一个簇对应的字节数
    ; eax 当前 loader 簇号
LoaderFound:
    movzx   ax, byte [BPB_SecPerClus]
    mul     word [BPB_BytsPerSec]
    mov     dx, ax

    pop     di
    mov     ax, [es:di + DIR_FstClusHI]
    shl     eax, 8
    mov     ax, [es:di + DIR_FstClusLO]

    mov     cx, BaseOfLoader
    mov     es, cx
    mov     bx, OffsetOfLoader
    movzx   cx, byte [BPB_SecPerClus]
LoadLoader:
    call    ReadCluster
    add     bx, dx
    call    NextCluster
    cmp     eax, CLUSTER_Last
    jnz     LoadLoader

    mov     dl, [DriverNumber]
    mov     ebx, [PartitionLBA]
    jmp     BaseOfLoader:OffsetOfLoader

times SizeOfBoot-($-$$) db 0
