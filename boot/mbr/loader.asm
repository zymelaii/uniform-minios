
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               loader.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


org  0100h

	jmp	LABEL_START		; Start

; 下面是 FAT12 磁盘的头, 之所以包含它是因为下面用到了磁盘的一些信息
%include	"fat32.inc"
%include	"loader.inc"
%include	"pm.inc"


; GDT ------------------------------------------------------------------------------------------------------------------------------------------------------------
;                                                段基址            段界限     , 属性
LABEL_GDT:				Descriptor             0,                    0, 0						; 空描述符
LABEL_DESC_FLAT_C:		Descriptor             0,              0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K			; 0 ~ 4G
LABEL_DESC_FLAT_RW:		Descriptor             0,              0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K			; 0 ~ 4G
LABEL_DESC_VIDEO:		Descriptor		 0B8000h,               0ffffh, DA_DRW | DA_DPL3	; 显存首地址
; GDT ------------------------------------------------------------------------------------------------------------------------------------------------------------

GdtLen		equ	$ - LABEL_GDT
GdtPtr		dw	GdtLen - 1							; 段界限
			dd	BaseOfLoaderPhyAddr + LABEL_GDT		; 基地址

; GDT 选择子 ----------------------------------------------------------------------------------
SelectorFlatC		equ	LABEL_DESC_FLAT_C	- LABEL_GDT
SelectorFlatRW		equ	LABEL_DESC_FLAT_RW	- LABEL_GDT
SelectorVideo		equ	LABEL_DESC_VIDEO	- LABEL_GDT + SA_RPL3
; GDT 选择子 ----------------------------------------------------------------------------------

FAT_START_SECTOR 	DD 	0 ;FAT表的起始扇区号 ;added by mingxuan 2020-9-17
DATA_START_SECTOR 	DD  0 ;数据区起始扇区号 ;added by mingxuan 2020-9-17

LABEL_START:			; <--- 从这里开始 *************
	
	;for test
	;added by mingxuan 2020-9-11
	; 清屏
	;mov		ax, 0600h		; AH = 6,  AL = 0h
	;mov		bx, 0700h		; 黑底白字(BL = 07h)
	;mov		cx, 0			; 左上角: (0, 0)
	;mov		dx, 0184fh		; 右下角: (80, 50)
	;int		10h				; int 10h
	
	;for test
	;added by mingxuan 2020-9-11
	;mov		dh, 1
	;call 	DispStrRealMode
	;jmp 	$

	cld
	mov		ax, cs
	mov		ds, ax
	mov		es, ax	;deleted by mingxuan 2020-9-16
	mov		ss, ax

	;added by mingxuan 2020-9-16
	mov		ax, BaseOfBoot
	mov		fs, ax 			;fs存储BaseOfBoot，用于查找FAT32的配置信息

	; 计算FAT表的起始扇区号 ; added by mingxuan 2020-9-17
	mov		eax, [ fs:OffsetOfActiPartStartSec]
	add		ax, [ fs:BPB_RsvdSecCnt ]
	mov 	[FAT_START_SECTOR], eax
	
	; 计算数据区起始扇区号 ; added by mingxuan 2020-9-17
	add		eax, [ fs:BS_SecPerFAT ]
	add		eax, [ fs:BS_SecPerFAT ]
	mov		[DATA_START_SECTOR], eax

	mov		sp, STACK_ADDR
	mov		bp, BaseOfStack

	mov 	dword [bp - DAP_SECTOR_HIGH ],	00h
	mov 	byte  [bp - DAP_RESERVED1   ], 	00h
	mov 	byte  [bp - DAP_RESERVED2   ], 	00h
	mov 	byte  [bp - DAP_PACKET_SIZE ], 	10h
	mov		byte  [bp - DAP_READ_SECTORS],  01h
	mov		word  [bp - DAP_BUFFER_SEG  ],	0x09000

	; 得到内存数
	mov	ebx, 0			; ebx = 后续值, 开始时需为 0
	mov	di, _MemChkBuf		; es:di 指向一个地址范围描述符结构（Address Range Descriptor Structure）
.MemChkLoop:
	mov	eax, 0E820h		; eax = 0000E820h
	mov	ecx, 20			; ecx = 地址范围描述符结构的大小
	mov	edx, 0534D4150h		; edx = 'SMAP'
	int	15h			; int 15h
	jc	.MemChkFail
	add	di, 20
	inc	dword [_dwMCRNumber]	; dwMCRNumber = ARDS 的个数
	cmp	ebx, 0
	jne	.MemChkLoop
	jmp	.MemChkOK
.MemChkFail:
	mov	dword [_dwMCRNumber], 0
.MemChkOK:

	;jmp	_SEARCH_LOADER	;deleted by mingxuan 2020-9-16
	jmp		_SEARCH_KERNEL	;modified by mingxuan 2020-9-16

;_MISSING_LOADER: 	;deleted by mingxuan 2020-9-16
_MISSING_KERNEL:	;modified by mingxuan 2020-9-16
_DISK_ERROR:
	;for test
	;added by mingxuan 2020-9-16
	; 清屏
	;mov		ax, 0600h		; AH = 6,  AL = 0h
	;mov		bx, 0700h		; 黑底白字(BL = 07h)
	;mov		cx, 0			; 左上角: (0, 0)
	;mov		dx, 0184fh		; 右下角: (80, 50)
	;int		10h				; int 10h

	jmp	$

ReadSector:
	pusha
	mov		ah, 42h
	lea 	si, [BP - DAP_PACKET_SIZE]

	;mov	dl, [BS_DriveNum]	;deleted by mingxuan 2020-9-16
	mov		dl, [fs:BS_DriveNum];modified by mingxuan 2020-9-16

	int 	13h
	jc 		_DISK_ERROR
	popa
	ret

;_SEARCH_LOADER:	;deleted by mingxuan 2020-9-16
_SEARCH_KERNEL:		;modified by mingxuan 2020-9-16
	mov		word [bp - DAP_BUFFER_OFF], DATA_BUF_OFF

	;mov	eax, dword [BS_RootClus]	;deleted by mingxuan 2020-9-16
	mov		eax, dword [fs:BS_RootClus]	;modified by mingxuan 2020-9-16

	mov		dword [bp - CURRENT_CLUSTER], eax

_NEXT_ROOT_CLUSTER:
	dec 	eax
	dec 	eax
	xor		ebx, ebx
	
	;mov	bl, byte [BPB_SecPerClu]	;deleted by mingxuan 2020-9-16
	mov		bl, byte [fs:BPB_SecPerClu] ;modified by mingxuan 2020-9-16

	mul		ebx
	;add		eax, DATA_START_SECTOR
	add		eax, [DATA_START_SECTOR] ;modified by mingxuan 2020-9-17
	mov		dword [BP - DAP_SECTOR_LOW], eax
	
	;mov	dl, [BPB_SecPerClu] ;deleted by mingxuan 2020-9-16
	mov		dl, [fs:BPB_SecPerClu] ;modified by mingxuan 2020-9-16

_NEXT_ROOT_SECTOR:
	call	ReadSector

	mov		di, DATA_BUF_OFF
	mov		bl, DIR_PER_SECTOR

_NEXT_ROOT_ENTRY:
	cmp		byte [di], DIR_NAME_FREE
	;jz		_MISSING_LOADER 	;deleted by mingxuan 2020-9-16
	jz		_MISSING_KERNEL		;modified by mingxuan 2020-9-16

	push 	di;

	;cmpsb将DS:SI和ES:DI中的字符串进行比较, 故需要修改一下es, mingxuan
	;mov		ax, ds ;added by mingxuan 2020-9-16
	;mov		es, ax ;added by mingxuan 2020-9-16

	mov		si, KernelFileName
	mov		cx, 10
	repe	cmpsb ;将DS:SI和ES:DI中的字符串进行比较

	;把es再改回BaseOfBoot，用于索引FAT32的配置信息, mingxuan
	;mov		ax, BaseOfBoot ;added by mingxuan 2020-9-16
	;mov 	es, ax		   ;added by mingxuan 2020-9-16

	;jcxz	_FOUND_LOADER 
	jcxz	_FOUND_KERNEL ;modified by mingxuan 2020-9-16

	pop		di
	add		di, DIR_ENTRY_SIZE
	dec		bl
	jnz		_NEXT_ROOT_ENTRY

	dec		dl
	jz 		_CHECK_NEXT_ROOT_CLUSTER
	inc		dword [bp - DAP_SECTOR_LOW]
	jmp 	_NEXT_ROOT_SECTOR

_CHECK_NEXT_ROOT_CLUSTER:

	 ; 计算FAT所在的簇号和偏移 
	 ; FatOffset = ClusterNum*4
	 XOR  EDX,EDX
	 MOV  EAX,DWORD[BP - CURRENT_CLUSTER]
	 SHL  EAX,2
	 XOR  ECX,ECX

	 ;MOV  CX,WORD [ BPB_BytesPerSec ]	;deleted by mingxuan 2020-9-16
	 MOV  CX,WORD [ fs:BPB_BytesPerSec ];modified by mingxuan 2020-9-16
	 
	 DIV  ECX  ; EAX = Sector EDX = OFFSET
	 ;ADD  EAX, FAT_START_SECTOR
	 ADD  EAX, [FAT_START_SECTOR] ;modified by mingxuan 2020-9-17
	 MOV  DWORD [ BP - DAP_SECTOR_LOW ], EAX 
	   
	 call  ReadSector
	  
	 ; 检查下一个簇
	 MOV  DI,DX
	 ADD  DI,DATA_BUF_OFF
	 MOV  EAX,DWORD[DI]  ; EAX = 下一个要读的簇号
	 AND  EAX,CLUSTER_MASK
	 MOV  DWORD[ BP - CURRENT_CLUSTER ],EAX
	 CMP  EAX,CLUSTER_LAST  ; CX >= 0FFFFFF8H，则意味着没有更多的簇了
	 JB  _NEXT_ROOT_CLUSTER
	 
	 ;JMP  _MISSING_LOADER	;deleted by mingxuan 2020-9-16
	 JMP  _MISSING_KERNEL	;modified by mingxuan 2020-9-16

;_FOUND_LOADER:
_FOUND_KERNEL: ;modified by mingxuan 2020-9-16
	 ; 目录结构地址放在DI中
	 pop  di
	 xor  eax, eax
	 mov  ax, [di + OFF_START_CLUSTER_HIGH] ; 起始簇号高32位
	 shl  ax, 16
	 mov  ax, [di + OFF_START_CLUSTER_LOW]  ; 起始簇号低32位
	 mov  dword [ bp - CURRENT_CLUSTER ], eax
	 mov  cx, BaseOfKernelFile      ; CX  = 缓冲区段地址 

_NEXT_DATA_CLUSTER:
	 ; 根据簇号计算扇区号
	 DEC  EAX
	 DEC  EAX  
	 XOR  EBX,EBX

	 ;MOV  BL, BYTE [ BPB_SecPerClu ] ;deleted by mingxuan 2020-9-16
	 MOV  BL, BYTE [ fs:BPB_SecPerClu ];modified by mingxuan 2020-9-16

	 MUL  EBX 
	 ;ADD  EAX, DATA_START_SECTOR
	 ADD  EAX, [DATA_START_SECTOR] ;modified by mingxuan 2020-9-17
	 MOV  DWORD[ BP - DAP_SECTOR_LOW  ], EAX

	 ;MOV  BL , BYTE [BPB_SecPerClu]	;deleted by mingxuan 2020-9-16
	 MOV  BL , BYTE [fs:BPB_SecPerClu] ;modified by mingxuan 2020-9-16

	 ; 设置缓冲区
	 MOV  WORD [ BP - DAP_BUFFER_SEG   ], CX
	 MOV  WORD [ BP - DAP_BUFFER_OFF   ], OffsetOfKernelFile

_NEXT_DATA_SECTOR:
	 ; 读取簇中的每个扇区(内层循环)
	 ; 注意 : 通过检查文件大小，可以避免读取最后一个不满簇的所有大小
	 call  ReadSector
	 
	 ; 更新地址，继续读取
	 ;MOV  AX, WORD [BPB_BytesPerSec] ;deleted by mingxuan 2020-9-16
	 MOV  AX, WORD [fs:BPB_BytesPerSec] ;modified by mingxuan 2020-9-16

	 ADD  WORD  [BP - DAP_BUFFER_OFF], ax 
	 INC  DWORD [BP - DAP_SECTOR_LOW]  ; 递增扇区号
	 DEC  BL        ; 内层循环计数
	 JNZ  _NEXT_DATA_SECTOR
	  
	 ; 更新读取下一个簇的缓冲区地址
	 ;MOV  CL, BYTE [ BPB_SecPerClu ]	;deleted by mingxuan 2020-9-16
	 MOV  CL, BYTE [ fs:BPB_SecPerClu ] ;modified by mingxuan 2020-9-16
	 
	 ;MOV  AX, WORD [BPB_BytesPerSec]	;deleted by mingxuan 2020-9-16
	 MOV  AX, WORD [fs:BPB_BytesPerSec] ;modified by mingxuan 2020-9-16

	 SHR  AX, 4
	 MUL  CL
	 ADD  AX, WORD [ BP - DAP_BUFFER_SEG ] 
	 MOV  CX, AX ; 保存下一个簇的缓冲区段地址
	 
	 ;====================================================================
	 ; 检查是否还有下一个簇(读取FAT表的相关信息)
	 ;  LET   N = 数据簇号
	 ;  THUS FAT_BYTES  = N*4  (FAT32)
	 ;  FAT_SECTOR = FAT_BYTES / BPB_BytesPerSec
	 ;  FAT_OFFSET = FAT_BYTES % BPB_BytesPerSec
	 ;====================================================================
	 
	 ; 计算FAT所在的簇号和偏移 
	 MOV  EAX,DWORD [BP - CURRENT_CLUSTER]
	 XOR  EDX,EDX
	 SHL  EAX,2
	 XOR  EBX,EBX
	 
	 ;MOV  BX,WORD [ BPB_BytesPerSec ]	;deleted by mingxuan 2020-9-16
	 MOV  BX,WORD [ fs:BPB_BytesPerSec ];modified by mingxuan 2020-9-16
	 
	 DIV  EBX   ; EAX = Sector  EDX = Offset
	 
	 ; 设置缓冲区地址
	 ;ADD  EAX, FAT_START_SECTOR
	 ADD  EAX, [FAT_START_SECTOR] ;modified by mingxuan 2020-9-17
	 MOV  DWORD [ BP - DAP_SECTOR_LOW ], EAX 
	 MOV  WORD [BP - DAP_BUFFER_SEG  ], 0x09000 
	 MOV  WORD [BP - DAP_BUFFER_OFF  ], DATA_BUF_OFF

	 ; 读取扇区
	 CALL  ReadSector
	  
	 ; 检查下一个簇
	 MOV  DI,DX
	 ADD  DI,DATA_BUF_OFF
	 MOV  EAX,DWORD[DI]  ; EAX = 下一个要读的簇号
	 AND  EAX,CLUSTER_MASK
	 MOV  DWORD[ BP - CURRENT_CLUSTER ],EAX
	 CMP  EAX,CLUSTER_LAST  ; CX >= 0FFFFFF8H，则意味着没有更多的簇了
	 JB  _NEXT_DATA_CLUSTER


;;add end add by liang 2016.04.20	
; 下面准备跳入保护模式 -------------------------------------------
	
	;for test
	;added by mingxuan 2020-9-11
	;mov		dh, 1
	;call 	DispStrRealMode
	;jmp 	$


; 加载 GDTR
	lgdt	[GdtPtr]

; 关中断
	cli

; 打开地址线A20
	in	al, 92h
	or	al, 00000010b
	out	92h, al

; 准备切换到保护模式
	mov	eax, cr0
	or	eax, 1
	mov	cr0, eax

; 真正进入保护模式
	jmp	dword SelectorFlatC:(BaseOfLoaderPhyAddr+LABEL_PM_START)


;============================================================================
;变量
;----------------------------------------------------------------------------
wSectorNo		dw	0		; 要读取的扇区号
bOdd			db	0		; 奇数还是偶数
dwKernelSize		dd	0		; KERNEL.BIN 文件大小
_dwEchoSize		dd	0		;echo size    add by liang 2016.04.20
;============================================================================
;字符串
;----------------------------------------------------------------------------
KernelFileName		db	"KERNEL  BIN", 0	; KERNEL.BIN 之文件名
EchoFileName		db	"INIT    BIN", 0	;add by liang 2016.04.20  ;edit by visual 2016.5.16
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	9
LoadMessage:		db	"Loading  "
Message1		db	"Ready.   "
Message2		db	"No KERNEL"
Message3		db	"exLoading"		;add by liang 2016.04.20
Message4		db	"exReady. "		;add by liang 2016.04.20
Message5		db	"No ECHO  "		;add by liang 2016.04.20
;============================================================================

;----------------------------------------------------------------------------
; 函数名: DispStrRealMode
;----------------------------------------------------------------------------
; 运行环境:
;	实模式（保护模式下显示字符串由函数 DispStr 完成）
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
DispStrRealMode:
	mov		ax, MessageLength
	mul		dh
	add		ax, LoadMessage
	mov		bp, ax			; ┓
	mov		ax, ds			; ┣ ES:BP = 串地址
	mov		es, ax			; ┛
	mov		cx, MessageLength	; CX = 串长度
	mov		ax, 01301h		; AH = 13,  AL = 01h
	mov		bx, 0007h		; 页号为0(BH = 0) 黑底白字(BL = 07h)
	mov		dl, 0
	add		dh, 3			; 从第 3 行往下显示
	int		10h			; int 10h
	ret
;----------------------------------------------------------------------------


;----------------------------------------------------------------------------
; 函数名: KillMotor
;----------------------------------------------------------------------------
; 作用:
;	关闭软驱马达
KillMotor:
	push	dx
	mov	dx, 03F2h
	mov	al, 0
	out	dx, al
	pop	dx
	ret
;----------------------------------------------------------------------------

; 从此以后的代码在保护模式下执行 ----------------------------------------------------
; 32 位代码段. 由实模式跳入 ---------------------------------------------------------
[SECTION .s32]

ALIGN	32

[BITS	32]

LABEL_PM_START:
	mov		ax, SelectorVideo
	mov		gs, ax
	mov		ax, SelectorFlatRW
	mov		ds, ax
	mov		es, ax
	mov		fs, ax
	mov		ss, ax
	mov		esp, TopOfStack

	push	szMemChkTitle
	call	DispStr
	add		esp, 4

	call	DispMemInfo
	call	getFreeMemInfo			;add by liang 2016.04.13
	call	DispEchoSize			;add by liang 2016.04.21
	call	SetupPaging

	; added by mingxuan 2020-9-16
	; for test
	mov	ah, 0Fh				; 0000: 黑底    1111: 白字
	mov	al, 'P'
	mov	[gs:((80 * 0 + 39) * 2)], ax	; 屏幕第 0 行, 第 39 列。

	call	InitKernel

	;jmp	$

	;***************************************************************
	jmp	SelectorFlatC:KernelEntryPointPhyAddr	; 正式进入内核 *
	;***************************************************************
	; 内存看上去是这样的：
	;              ┃                                    ┃
	;              ┃                 .                  ┃
	;              ┃                 .                  ┃
	;              ┃                 .                  ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■┃
	;              ┃■■■■■■Page  Tables■■■■■■■■■■■■■■■■■■┃
	;              ┃■■■■■(大小由LOADER决定)■■■■■■■■■■■■■┃
	;    00101000h ┃■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■┃ PageTblBase
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■┃
	;    00100000h ┃■■■■Page Directory Table■■■■■■■■■■■■┃ PageDirBase  <- 1M
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□┃
	;       F0000h ┃□□□□□□□System ROM□□□□□□□□□□□□□□□□□□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□┃
	;       E0000h ┃□□□□Expansion of system ROM □□□□□□□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□┃
	;       C0000h ┃□□□Reserved for ROM expansion□□□□□□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□┃ B8000h ← gs
	;       A0000h ┃□□□Display adapter reserved□□□□□□□□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□┃
	;       9FC00h ┃□□extended BIOS data area (EBDA)□□□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■┃
	;       90000h ┃■■■■■■■LOADER.BIN■■■■■■■■■■■■■■■■■■■┃ somewhere in LOADER ← esp
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■┃
	;       80000h ┃■■■■■■■KERNEL.BIN■■■■■■■■■■■■■■■■■■■┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■┃
	;       30000h ┃■■■■■■■■KERNEL■■■■■■■■■■■■■■■■■■■■■■┃ 30400h ← KERNEL 入口 (KernelEntryPointPhyAddr)
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃                                    ┃
	;        7E00h ┃              F  R  E  E            ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■┃
	;        7C00h ┃■■■■■■BOOT  SECTOR■■■■■■■■■■■■■■■■■■┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃                                    ┃
	;         500h ┃              F  R  E  E            ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□□┃
	;         400h ┃□□□□ROM BIOS parameter area □□□□□□□□┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇◇┃
	;           0h ┃◇◇◇◇◇◇Int  Vectors◇◇◇◇◇◇┃
	;              ┗━━━━━━━━━━━━━━━━━━┛ ← cs, ds, es, fs, ss
	;
	;
	;		┏━━━┓				┏━━━┓
	;		┃■■■■■■┃ 我们使用 	┃□□□□□□┃ 不能使用的内存
	;		┗━━━┛				┗━━━┛
	;		┏━━━┓				┏━━━┓
	;		┃      ┃ 未使用空间	┃◇◇◇┃ 可以覆盖的内存
	;		┗━━━┛				┗━━━┛
	;
	; 注：KERNEL 的位置实际上是很灵活的，可以通过同时改变 LOAD.INC 中的 KernelEntryPointPhyAddr 和 MAKEFILE 中参数 -Ttext 的值来改变。
	;     比如，如果把 KernelEntryPointPhyAddr 和 -Ttext 的值都改为 0x400400，则 KERNEL 就会被加载到内存 0x400000(4M) 处，入口在 0x400400。
	;




; ------------------------------------------------------------------------
; 显示 AL 中的数字
; ------------------------------------------------------------------------
DispAL:
	push	ecx
	push	edx
	push	edi

	mov	edi, [dwDispPos]

	mov	ah, 0Fh			; 0000b: 黑底    1111b: 白字
	mov	dl, al
	shr	al, 4
	mov	ecx, 2
.begin:
	and	al, 01111b
	cmp	al, 9
	ja	.1
	add	al, '0'
	jmp	.2
.1:
	sub	al, 0Ah
	add	al, 'A'
.2:
	mov	[gs:edi], ax
	add	edi, 2

	mov	al, dl
	loop	.begin
	;add	edi, 2

	mov	[dwDispPos], edi

	pop	edi
	pop	edx
	pop	ecx

	ret
; DispAL 结束-------------------------------------------------------------


; ------------------------------------------------------------------------
; 显示一个整形数
; ------------------------------------------------------------------------
DispInt:
	mov	eax, [esp + 4]
	shr	eax, 24
	call	DispAL

	mov	eax, [esp + 4]
	shr	eax, 16
	call	DispAL

	mov	eax, [esp + 4]
	shr	eax, 8
	call	DispAL

	mov	eax, [esp + 4]
	call	DispAL

	mov	ah, 07h			; 0000b: 黑底    0111b: 灰字
	mov	al, 'h'
	push	edi
	mov	edi, [dwDispPos]
	mov	[gs:edi], ax
	add	edi, 4
	mov	[dwDispPos], edi
	pop	edi

	ret
; DispInt 结束------------------------------------------------------------

; ------------------------------------------------------------------------
; 显示一个字符串
; ------------------------------------------------------------------------
DispStr:
	push	ebp
	mov	ebp, esp
	push	ebx
	push	esi
	push	edi

	mov	esi, [ebp + 8]	; pszInfo
	mov	edi, [dwDispPos]
	mov	ah, 0Fh
.1:
	lodsb
	test	al, al
	jz	.2
	cmp	al, 0Ah	; 是回车吗?
	jnz	.3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.1
.3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:
	mov	[dwDispPos], edi

	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret
; DispStr 结束------------------------------------------------------------

; ------------------------------------------------------------------------
; 换行
; ------------------------------------------------------------------------
DispReturn:
	push	szReturn
	call	DispStr			;printf("\n");
	add	esp, 4

	ret
; DispReturn 结束---------------------------------------------------------


; ------------------------------------------------------------------------
; 内存拷贝，仿 memcpy
; ------------------------------------------------------------------------
; void* MemCpy(void* es:pDest, void* ds:pSrc, int iSize);
; ------------------------------------------------------------------------
MemCpy:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx

	mov	edi, [ebp + 8]	; Destination
	mov	esi, [ebp + 12]	; Source
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; 判断计数器
	jz	.2		; 计数器为零时跳出

	mov	al, [ds:esi]		; ┓
	inc	esi			; ┃
					; ┣ 逐字节移动
	mov	byte [es:edi], al	; ┃
	inc	edi			; ┛

	dec	ecx		; 计数器减一
	jmp	.1		; 循环
.2:
	mov	eax, [ebp + 8]	; 返回值

	pop	ecx
	pop	edi
	pop	esi
	mov	esp, ebp
	pop	ebp

	ret			; 函数结束，返回
; MemCpy 结束-------------------------------------------------------------

;;;;;;;;;;;;;;;;;add begin add by liang 2016.04.13;;;;;;;;;;;;;;;;;;;;;

memtest:
	push	edi
	push	esi
	
	mov	esi,0xaa55aa55
	mov	edi,0x55aa55aa
	mov	eax,[esp+8+4]		;start
.mt_loop:
	mov	ebx,eax
	add	ebx,0xffc		;检查每4KB最后4B
	mov	ecx,[ebx]		;保存原值
	mov	[ebx],esi		;写入
	xor	dword [ebx],0xffffffff	;反转
	cmp	edi,[ebx]		;检查反转结果是否正确
	jne	.mt_fin
	xor	dword [ebx],0xffffffff	;再次反转
	cmp	esi,[ebx]		;检查是否恢复初始值
	jne	.mt_fin
	mov	[ebx],ecx		;恢复为修改前的值
	add	eax,0x1000		;每循环一次加0x1000
	cmp	eax,[esp+8+8]		;未到end，循环
	jbe	.mt_loop
	pop	esi
	pop	edi
	ret
	
.mt_fin:
	mov	[ebx],ecx		;恢复为修改前的值
	pop	esi
	pop	edi
	ret
	
getFreeMemInfo:
	push	eax
	push	ebx
	push	ecx
	push	edx
	
	mov	eax,cr0
	or	eax,0x60000000	;禁止缓存
	mov	cr0,eax
	
	mov	ebx,0x00000000	;检查0到32M
	mov	ecx,0x02000000
	mov	edx,FMIBuff	;存于0x007ff000处

.fmi_loop:
	push	ecx
	push	ebx
	call	memtest
	pop	ebx
	pop	ecx
	mov	[edx+4],eax	;留出前4B存放dwFMINumber
	add	edx,4
	add	eax,0x1000
	mov	ebx,eax
	inc	dword [dwFMINumber]	;循环次数，即返回值个数
	cmp	ebx,ecx
	jb	.fmi_loop
	
	mov	ebx,[dwFMINumber]
	mov	edx,FMIBuff
	mov	[edx],ebx		;前4B存放返回值个数
	mov	ebx,[FMIBuff]		
	push	ebx
	call	DispInt			;打印返回值个数
	add	esp,4
	
	mov	eax,cr0
	and	eax,0x9fffffff	;恢复缓存
	mov	cr0,eax
	
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	ret
	

;;;;;;;;;;;;;;;;;;;;;;;;;;add end add by liang 2016.04.13;;;;;;;;;;;;;;;;;;;;;;;;;;

;;add begin add by liang 2016.04.21	
DispEchoSize:
	push	eax
	mov	eax,[dwEchoSize]
	push	eax
	call	DispInt			
	add	esp,4
	pop	eax
	ret
;;add end add by liang 2016.04.21

; 显示内存信息 --------------------------------------------------------------
DispMemInfo:
	push	esi
	push	edi
	push	ecx

	mov	esi, MemChkBuf
	mov	ecx, [dwMCRNumber]	;for(int i=0;i<[MCRNumber];i++) // 每次得到一个ARDS(Address Range Descriptor Structure)结构
.loop:					;{
	mov	edx, 5			;	for(int j=0;j<5;j++)	// 每次得到一个ARDS中的成员，共5个成员
	mov	edi, ARDStruct		;	{			// 依次显示：BaseAddrLow，BaseAddrHigh，LengthLow，LengthHigh，Type
.1:					;
	push	dword [esi]		;
	call	DispInt			;		DispInt(MemChkBuf[j*4]); // 显示一个成员
	pop	eax			;
	stosd				;		ARDStruct[j*4] = MemChkBuf[j*4];
	add	esi, 4			;
	dec	edx			;
	cmp	edx, 0			;
	jnz	.1			;	}
	call	DispReturn		;	printf("\n");
	cmp	dword [dwType], 1	;	if(Type == AddressRangeMemory) // AddressRangeMemory : 1, AddressRangeReserved : 2
	jne	.2			;	{
	mov	eax, [dwBaseAddrLow]	;
	add	eax, [dwLengthLow]	;
	cmp	eax, [dwMemSize]	;		if(BaseAddrLow + LengthLow > MemSize)
	jb	.2			;
	mov	[dwMemSize], eax	;			MemSize = BaseAddrLow + LengthLow;
.2:					;	}
	loop	.loop			;}
					;
	call	DispReturn		;printf("\n");
	push	szRAMSize		;
	call	DispStr			;printf("RAM size:");
	add	esp, 4			;
					;
	push	dword [dwMemSize]	;
	call	DispInt			;DispInt(MemSize);
	add	esp, 4			;

	pop	ecx
	pop	edi
	pop	esi
	ret
; ---------------------------------------------------------------------------

; 启动分页机制 --------------------------------------------------------------
SetupPaging:
	; 根据内存大小计算应初始化多少PDE以及多少页表
	xor	edx, edx
	mov	eax, [dwMemSize]
	mov	ebx, 400000h	; 400000h = 4M = 4096 * 1024, 一个页表对应的内存大小
	div	ebx
	mov	ecx, eax	; 此时 ecx 为页表的个数，也即 PDE 应该的个数
	test	edx, edx
	jz	.no_remainder
	inc	ecx		; 如果余数不为 0 就需增加一个页表
.no_remainder:
	push	ecx		; 暂存页表个数
	mov dword[PageTblNumAddr],ecx ;将页表数写进这个物理地址
	
	; 为简化处理, 所有线性地址对应相等的物理地址. 并且不考虑内存空洞.

	; 首先初始化页目录
	mov	ax, SelectorFlatRW
	mov	es, ax
	mov	edi, PageDirBase	; 此段首地址为 PageDirBase
	xor	eax, eax
	mov	eax, PageTblBase | PG_P  | PG_USU | PG_RWW
.1:
	stosd
	add	eax, 4096		; 为了简化, 所有页表在内存中是连续的.
	loop	.1
	
;;;;初始化3G处的页目录;;;;;;;;;;;;;;;;	//add by visual 2016.5.10
	pop eax			;页表个数
	mov ecx,eax 	;重新放到ecx里
	push ecx		;暂存页表个数
	mov	ax, SelectorFlatRW
	mov	es, ax	
	mov eax, 3072				;768*4
	add eax, PageDirBase		;
	mov	edi, eax				; 应该往页目录这个位置写： PageDirBase+768*4，即线性地址3G处
	
	xor	eax, eax				; 清0
	mov eax, ecx				;
	mov ebx, 4096				;
	mul ebx						;跳过前ecx个页表，即PageTblBase+页表数*4096
	add eax, PageTblBase | PG_P  | PG_USU | PG_RWW;		 
.1k:
	stosd
	add	eax, 4096		; 为了简化, 所有页表在内存中是连续的.
	loop	.1k
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

	; 再初始化所有页表（最开始处，一一映射的）
	pop	eax			; 页表个数
	push eax 		;暂存页表个数
	mov	ebx, 1024		; 每个页表 1024 个 PTE
	mul	ebx
	mov	ecx, eax		; PTE个数 = 页表个数 * 1024
	mov	edi, PageTblBase	; 此段首地址为 PageTblBase
	xor	eax, eax
	mov	eax, PG_P  | PG_USU | PG_RWW
.2:
	stosd
	add	eax, 4096		; 每一页指向 4K 的空间
	loop	.2
	
;;;;初始化3G处的页表;;;;;;;;;;;;;;;;	//add by visual 2016.5.10
	; 再初始化3G后的页表
	pop	eax			; 页表个数
	mov	ebx, 1024		; 每个页表 1024 个 PTE
	mul	ebx
	mov	ecx, eax		; PTE个数 = 页表个数 * 1024
	
	xor	eax, eax				; 清0
	mov eax, ecx				;
	mov ebx, 4
	mul ebx				;跳过前ecx个页表，即PageTblBase+页表数*4096
	add eax, PageTblBase		; 后面3G对应页表的起始位置为 PageTblBase+页表数*4096
	mov	edi, eax		
	xor	eax, eax
	mov	eax, PG_P  | PG_USU | PG_RWW		;从0开始
.2k:
	stosd
	add	eax, 4096		; 每一页指向 4K 的空间
	loop	.2k
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	;mov	ah, 0Fh				; 0000: 黑底    1111: 白字
	;mov	al, 'P'
	;mov	[gs:((80 * 0 + 39) * 2)], ax	; 屏幕第 0 行, 第 39 列。
	
	;启动页表机制
	mov	eax, PageDirBase
	mov	cr3, eax
	mov	eax, cr0
	or	eax, 80000000h
	mov	cr0, eax
	jmp	short .3
.3:
	nop

	ret
; 分页机制启动完毕 ----------------------------------------------------------



; InitKernel ---------------------------------------------------------------------------------
; 将 KERNEL.BIN 的内容经过整理对齐后放到新的位置
; --------------------------------------------------------------------------------------------
InitKernel:	; 遍历每一个 Program Header，根据 Program Header 中的信息来确定把什么放进内存，放到什么位置，以及放多少。
	xor	esi, esi
	mov	cx, word [BaseOfKernelFilePhyAddr + 2Ch]; ┓ ecx <- pELFHdr->e_phnum
	movzx	ecx, cx								; ┛
	mov	esi, [BaseOfKernelFilePhyAddr + 1Ch]	; esi <- pELFHdr->e_phoff
	add	esi, BaseOfKernelFilePhyAddr			; esi <- OffsetOfKernel + pELFHdr->e_phoff
.Begin:
	mov	eax, [esi + 0]
	cmp	eax, 0									; PT_NULL
	jz	.NoAction
	push	dword [esi + 010h]					; size	┓
	mov	eax, [esi + 04h]						;		┃
	add	eax, BaseOfKernelFilePhyAddr			;		┣ ::memcpy(	(void*)(pPHdr->p_vaddr),
	push	eax									; src	┃		uchCode + pPHdr->p_offset,
	push	dword [esi + 08h]					; dst	┃		pPHdr->p_filesz;
	call	MemCpy								;		┃
	add	esp, 12									;		┛
.NoAction:
	add	esi, 020h								; esi += pELFHdr->e_phentsize
	dec	ecx
	jnz	.Begin

	ret
; InitKernel ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


; SECTION .data1 之开始 ---------------------------------------------------------------------------------------------
[SECTION .data1]

ALIGN	32

LABEL_DATA:
; 实模式下使用这些符号
; 字符串
_szMemChkTitle:			db	"BaseAddrL BaseAddrH LengthLow LengthHigh   Type", 0Ah, 0
_szRAMSize:			db	"RAM size:", 0
_szReturn:			db	0Ah, 0
;; 变量
_dwMCRNumber:			dd	0	; Memory Check Result
_dwDispPos:			dd	(80 * 6 + 0) * 2	; 屏幕第 6 行, 第 0 列。
_dwMemSize:			dd	0
_ARDStruct:			; Address Range Descriptor Structure
	_dwBaseAddrLow:		dd	0
	_dwBaseAddrHigh:	dd	0
	_dwLengthLow:		dd	0
	_dwLengthHigh:		dd	0
	_dwType:		dd	0
_MemChkBuf:	times	256	db	0
_dwFMINumber:			dd	0		;add by liang 2016.04.13


;
;; 保护模式下使用这些符号
szMemChkTitle		equ	BaseOfLoaderPhyAddr + _szMemChkTitle
szRAMSize		equ	BaseOfLoaderPhyAddr + _szRAMSize
szReturn		equ	BaseOfLoaderPhyAddr + _szReturn
dwDispPos		equ	BaseOfLoaderPhyAddr + _dwDispPos
dwMemSize		equ	BaseOfLoaderPhyAddr + _dwMemSize
dwMCRNumber		equ	BaseOfLoaderPhyAddr + _dwMCRNumber
ARDStruct		equ	BaseOfLoaderPhyAddr + _ARDStruct
	dwBaseAddrLow	equ	BaseOfLoaderPhyAddr + _dwBaseAddrLow
	dwBaseAddrHigh	equ	BaseOfLoaderPhyAddr + _dwBaseAddrHigh
	dwLengthLow	equ	BaseOfLoaderPhyAddr + _dwLengthLow
	dwLengthHigh	equ	BaseOfLoaderPhyAddr + _dwLengthHigh
	dwType		equ	BaseOfLoaderPhyAddr + _dwType
MemChkBuf		equ	BaseOfLoaderPhyAddr + _MemChkBuf
dwFMINumber		equ	BaseOfLoaderPhyAddr + _dwFMINumber	;add by liang 2016.04.13
dwEchoSize		equ	BaseOfLoaderPhyAddr + _dwEchoSize		;add by liang 2016.04.21

; 堆栈就在数据段的末尾
StackSpace:	times	1000h	db	0
TopOfStack	equ	BaseOfLoaderPhyAddr + $	; 栈顶
; SECTION .data1 之结束 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

