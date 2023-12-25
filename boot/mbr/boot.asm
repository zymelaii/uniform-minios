;==============================================================================================================================
BaseOfStack			equ		0x07c00		; Boot状态下堆栈基地址
STACK_ADDR  		equ  	0x7bea 		; 堆栈栈顶

BaseOfBoot 					equ	1000h 		; added by mingxuan 2020-9-12
OffsetOfBoot				equ	7c00h		; load Boot sector to BaseOfBoot:OffsetOfBoot
OffsetOfActiPartStartSec	equ 7e00h		; 活动分区的起始扇区号相对于BaseOfBoot的偏移量	;added by mingxuan 2020-9-12 
											; 该变量来自分区表，保存在该内存地址，用于在os_boot和loader中查找FAT32文件

BOOT_FAT32_INFO		equ		0x5A		;位于boot中的FAT32配置信息的长度
										;added by mingxuan 2020-9-16

DATA_BUF_OFF 		equ  	0x2000		; 目录 被加载的缓冲区地址

OSLOADER_SEG 		equ  	0x09000 	; 起始段地址     
OSLOADER_SEG_OFF	equ		0x0100

;FAT_START_SECTOR 	equ  	0x820	  	; FAT表的起始扇区号  DWORD
;FAT_START_SECTOR 	equ  	0x1020	  	; FAT表的起始扇区号  DWORD ; for test 2020-9-10, mingxuan
;DATA_START_SECTOR 	equ  	0xd6a	  	; 数据区起始扇区号  DWORD
;DATA_START_SECTOR 	equ  	0x156a	  	; 数据区起始扇区号  DWORD ; for test 2020-9-10, mingxuan
;DATA_START_SECTOR 	equ  	0x13a4	  	; 数据区起始扇区号  DWORD ; for test 2020-9-13, mingxuan

DIR_PER_SECTOR  	equ  	0x10  		; 每个扇区所容纳的目录 BYTE

; 扩展磁盘服务所使用的地址包
DAP_SECTOR_HIGH  	equ  	4  			; 起始扇区号的高32位 ( 每次调用需要重置 ) DWORD
DAP_SECTOR_LOW  	equ  	8  			; 起始扇区号的低32位 ( 每次调用需要重置 ) DWORD

DAP_BUFFER_SEG  	equ  	10  		; 缓冲区段地址   ( 每次调用需要重置 ) WORD
DAP_BUFFER_OFF  	equ  	12  		; 缓冲区偏移   ( 每次调用需要重置 ) WORD  

DAP_RESERVED2  		equ  	13  		; 保留字节

DAP_READ_SECTORS 	equ  	14  		; 要处理的扇区数(1 - 127 )
DAP_RESERVED1  		equ  	15  		; 保留字节
DAP_PACKET_SIZE  	equ  	16  		; 包的大小为16字节

CURRENT_CLUSTER  	equ  	20  		; 当前正在处理的簇号 DWORD

; 目录项结构
OFF_START_CLUSTER_HIGH  equ  	20  		; 起始簇号高位  WORD
OFF_START_CLUSTER_LOW  	equ  	26  		; 起始簇号低位  WORD

; 相关常量
DIR_NAME_FREE    	equ  	0x00  		; 该项是空闲的
DIR_ENTRY_SIZE    	equ  	32  		; 每个目录项的尺寸

; 簇属性
CLUSTER_MASK    	equ  	0FFFFFFFH 	; 簇号掩码
CLUSTER_LAST    	equ  	0FFFFFF8H   	;0xFFFFFFF8-0xFFFFFFFF表示文件的最后一个簇

; added by mingxuan 2020-9-12
BPB_BytesPerSec	    equ		(OffsetOfBoot + 0xb)	;每扇区字节数
BPB_SecPerClu 		equ		(OffsetOfBoot + 0xd)	;每簇扇区数 
BPB_RsvdSecCnt		equ		(OffsetOfBoot + 0xe)	;保留扇区数
BPB_NumFATs			equ		(OffsetOfBoot + 0x10)	;FAT表数
BPB_RootEntCnt		equ 	(OffsetOfBoot + 0x11)	;FAT32不使用
BPB_TotSec16		equ		(OffsetOfBoot + 0x13)	;扇区总数
BPB_Media			equ 	(OffsetOfBoot + 0x15)	;介质描述符
BPB_FATSz16			equ		(OffsetOfBoot + 0x16)	;每个FAT表的大小扇区数(FAT32不使用)
BPB_SecPerTrk		equ		(OffsetOfBoot + 0x18)	;每磁道扇区数
BPB_NumHeads		equ		(OffsetOfBoot + 0x1a)	;磁头数
BPB_HiddSec			equ	    (OffsetOfBoot + 0x1c)	;分区已使用扇区数
BPB_TotSec32		equ		(OffsetOfBoot + 0x20)	;文件系统大小扇区数
	
BS_SecPerFAT		equ		(OffsetOfBoot + 0x24)	;每个FAT表大小扇区数
BS_Flag				equ		(OffsetOfBoot + 0x28)	;标记
BS_Version			equ		(OffsetOfBoot + 0x2a)	;版本号
BS_RootClus			equ		(OffsetOfBoot + 0x2c)	;根目录簇号
BS_FsInfoSec 		equ		(OffsetOfBoot + 0x30)	;FSINFO扇区号
BS_BackBootSec 		equ	    (OffsetOfBoot + 0x32)	;备份引导扇区位置
BS_Unuse1			equ		(OffsetOfBoot + 0x34)	;未使用
;BS_Unuse2			equ		(OffsetOfBoot + 0x40)	;未使用	;deleted by mingxuan 2020-9-15
;BS_Unuse3			equ 	(OffsetOfBoot + 0x41)	;未使用	;deleted by mingxuan 2020-9-15
BS_DriveNum			equ		(OffsetOfBoot + 0x40)	;设备号
BS_Unuse4			equ		(OffsetOfBoot + 0x41)	;未使用
BS_ExtBootFlag		equ		(OffsetOfBoot + 0x42)	;扩展引导标志
BS_VolNum			equ		(OffsetOfBoot + 0x43)	;卷序列号
BS_VolName			equ		(OffsetOfBoot + 0x47)	;卷标
;==============================================================================================================================
	;org	07c00h	;deleted by mingxuan 2020-9-16
	org		(07c00h + BOOT_FAT32_INFO)  ;FAT322规范规定第90~512个字节(共423个字节)是引导程序
										;modified by mingxuan 2020-9-16

	;jmp	START		;deleted by mingxuan 2020-9-15
	;nop				;deleted by mingxuan 2020-9-15

	FAT_START_SECTOR 	DD 	0 ;FAT表的起始扇区号 ;added by mingxuan 2020-9-17
	DATA_START_SECTOR 	DD  0 ;数据区起始扇区号 ;added by mingxuan 2020-9-17

;deleted by mingxuan 2020-9-12
	;BS_OEM			DB	'mkfs.fat'	;文件系统标志

	;BPB_BytesPerSec	DW	0x200		;每扇区字节数
	;BPB_SecPerClu 	DB	1			;每簇扇区数 
	;BPB_RsvdSecCnt	DW	0x20		;保留扇区数
	;BPB_NumFATs		DB	2			;FAT表数
	;BPB_RootEntCnt	DW 	0			;FAT32不使用
	;BPB_TotSec16	DW	0			;扇区总数
	;BPB_Media		DB 	0xf8		;介质描述符
	;BPB_FATSz16		DW	0			;每个FAT表的大小扇区数
	;BPB_SecPerTrk	DW	0x3f		;每磁道扇区数
	;BPB_NumHeads	DW	0xff		;磁头数
	;BPB_HiddSec		DD	0			;分区已使用扇区数
	;BPB_TotSec32	DD	0x015791	;文件系统大小扇区数
	
	;BS_SecPerFAT	DD	0x02a5		;每个FAT表大小扇区数
	;BS_Flag			DW	0			;标记
	;BS_Version		DW	0			;版本号
	;BS_RootClus		DD	2			;根目录簇号
	;BS_FsInfoSec 	DW	1			;FSINFO扇区号
	;BS_BackBootSec 	DW	6			;备份引导扇区位置
	;BS_Unuse1		DD	0			;未使用
	;BS_Unuse2		DD	0			;未使用
	;BS_Unuse3		DD 	0			;未使用
	;BS_DriveNum		DB	0x80		;设备号
	;BS_Unuse4		DB	0x01		;未使用
	;BS_ExtBootFlag	DB	0x29		;扩展引导标志
	;BS_VolNum		DD	0xbe3a8ff5	;卷序列号
	;BS_VolName		DB	'MZY hd boot'	;卷标

START:	
	cld
	mov    	ax, cs
	mov		ds, ax
	mov		es, ax ;deleted by mingxuan 2020-9-13
	mov		ss, ax

	; 清屏
	; for test, added by mingxuan 2020-9-15
	; pusha
	; mov		dx, 0184fh		; 右下角: (80, 50)
	; int		10h				; int 10h
	; popa

	;added by mingxuan 2020-9-13
	;mov		ax, BaseOfBoot
	;mov		fs, ax 			;es存储BaseOfBoot，用于查找FAT32的配置信息

	;FAT_START_SECTOR  DD 	([fs:OffsetOfActiPartStartSec] + [fs:BPB_RsvdSecCnt]) 
	; 计算FAT表的起始扇区号 ; added by mingxuan 2020-9-17
	mov		eax, [ OffsetOfActiPartStartSec]
	add		ax, [ BPB_RsvdSecCnt ]
	mov 	[FAT_START_SECTOR], eax
	; 计算数据区起始扇区号 ; added by mingxuan 2020-9-17
	add		eax, [BS_SecPerFAT]
	add		eax, [BS_SecPerFAT]
	mov		[DATA_START_SECTOR], eax

	mov		sp, STACK_ADDR
	mov  	bp, BaseOfStack

	mov 	dword [bp - DAP_SECTOR_HIGH ],	00h
	;mov 	byte  [bp - DAP_RESERVED1   ], 	00h ;deleted by mingxuan 2020-9-17
	;mov 	byte  [bp - DAP_RESERVED2   ], 	00h ;deleted by mingxuan 2020-9-17
	mov 	byte  [bp - DAP_PACKET_SIZE ], 	10h
	mov		byte  [bp - DAP_READ_SECTORS],  01h
	mov		word  [bp - DAP_BUFFER_SEG  ],	01000h

	;for test, added by mingxuan 2020-9-4
	;call 	DispStr		; 
	;jmp		$

	jmp		_SEARCH_LOADER


_MISSING_LOADER:     ; 显示没有装载程序
	;for test, added by mingxuan 2020-9-10
	;call 	DispStr		; 
	;JMP  	$ ;for test, added by mingxuan 2020-9-10

	; 清屏
	; for test, added by mingxuan 2020-9-15
	;pusha
	;mov		dx, 0184fh		; 右下角: (80, 50)
	;int		10h				; int 10h
	;popa

_DISK_ERROR:      	 ; 显示磁盘错误信息
	;for test, added by mingxuan 2020-9-4

	; 清屏
	; for test, added by mingxuan 2020-9-15
	;pusha
	;mov		dx, 0184fh		; 右下角: (80, 50)
	;int		10h				; int 10h
	;popa

	JMP  	$

ReadSector:
	pusha
	
	mov		ah, 42h						;ah是功能号，扩展读对应0x42
	lea 	si, [BP - DAP_PACKET_SIZE]  ;使用扩展int13时DAP结构体首地址传给si
	
	;mov	dl, [BS_DriveNum]			;deleted by mingxuan 2020-9-13
	;mov	es, BaseOfBoot 				;modified by mingxuan 2020-9-13
	;mov	dl, [fs:BS_DriveNum]		;modified by mingxuan 2020-9-13
	mov		dl, [BS_DriveNum]		;modified by mingxuan 2020-9-17
	;mov	dl, 0x80

	;jmp 	$

	int 	13h

	jc 		_DISK_ERROR

	;jmp	$

	; 清屏
	; for test, added by mingxuan 2020-9-15
	;pusha
	;mov		dx, 0184fh		; 右下角: (80, 50)
	;int		10h				; int 10h
	;popa


	popa
	ret

_SEARCH_LOADER:
	mov		word [bp - DAP_BUFFER_OFF], DATA_BUF_OFF
	
	;mov	eax, dword [BS_RootClus]		;deleted by mingxuan 2020-9-13
	;mov	es, BaseOfBoot 					;modified by mingxuan 2020-9-13
	;mov	eax, dword [fs:BS_RootClus]		;modified by mingxuan 2020-9-13
	;mov		eax, dword [BS_RootClus]		;modified by mingxuan 2020-9-17
	mov		eax,  [BS_RootClus]

	mov		dword [bp - CURRENT_CLUSTER], eax

	;for test, added by mingxuan 2020-9-4
	;call 	DispStr		;
	;jmp $ 

_NEXT_ROOT_CLUSTER:
	; 根据簇号计算扇区号, mingxuan 2020-9-13
	dec 	eax
	dec 	eax

	xor		ebx, ebx

	;mov	bl, byte [BPB_SecPerClu]		;deleted by mingxuan 2020-9-13
	;mov	es, BaseOfBoot 					;modified by mingxuan 2020-9-13
	;mov	bl, byte [fs:BPB_SecPerClu]		;modified by mingxuan 2020-9-13
	mov		bl, [BPB_SecPerClu]				;modified by mingxuan 2020-9-17

	;jmp 	$

	mul		ebx
	
	;add		eax, DATA_START_SECTOR	;deleted by mingxuan 2020-9-17
	add		eax, [DATA_START_SECTOR]	;modified by mingxuan 2020-9-17
	mov		dword [BP - DAP_SECTOR_LOW], eax
	
	;mov	dl, [BPB_SecPerClu]				;deleted by mingxuan 2020-9-13
	;mov	dl, byte [es:BPB_SecPerClu]		;modified by mingxuan 2020-9-13
	;mov	dl, [fs:BPB_SecPerClu]			;modified by mingxuan 2020-9-15
	mov		dl, [BPB_SecPerClu]			;modified by mingxuan 2020-9-17
	;jmp $


_NEXT_ROOT_SECTOR:
	call	ReadSector

	;for test, added by mingxuan 2020-9-4
	;call 	DispStr		; 
	;jmp $

	mov		di, DATA_BUF_OFF
	mov		bl, DIR_PER_SECTOR

_NEXT_ROOT_ENTRY:
	cmp		byte [di], DIR_NAME_FREE
	jz		_MISSING_LOADER

	push 	di;
	mov		si, LoaderName
	;add	si, BOOT_FAT32_INFO ; added by mingxuan 2020-9-16
	;mov	si, 0x7df1 			; for test, added by mingxuan 2020-9-16
	
	mov		cx, 10
	repe	cmpsb
	jcxz	_FOUND_LOADER

	pop		di
	add		di, DIR_ENTRY_SIZE	
	dec		bl
	jnz		_NEXT_ROOT_ENTRY

	dec		dl
	jz 		_CHECK_NEXT_ROOT_CLUSTER

	inc		dword [bp - DAP_SECTOR_LOW]
	jmp 	_NEXT_ROOT_SECTOR

; Comments, added by mingxuan 2020-9-10
;====================================================================
; 检查是否还有下一个簇(读取FAT表的相关信息)
;  N = 数据簇号
;  FAT_BYTES(在FAT表中的偏移)  = N*4  (FAT32)
;  FAT_SECTOR = FAT_BYTES / BPB_BytesPerSec
;  FAT_OFFSET = FAT_BYTES % BPB_BytesPerSec
;====================================================================
_CHECK_NEXT_ROOT_CLUSTER: ; 检查是否还有下一个簇

	 ; 计算FAT表项所在的簇号和偏移 
	 ; FatOffset(在FAT表中的偏移) = ClusterNum(簇号) * 4
	 XOR  	EDX, EDX
	 MOV  	EAX, DWORD[BP - CURRENT_CLUSTER]
	 SHL  	EAX, 2 ;FatOffset = ClusterNum * 4

	 XOR  	ECX, ECX
	 ;MOV  	CX, WORD [ BPB_BytesPerSec ] 	;deleted by mingxuan 2020-9-13
	 ;mov	es, BaseOfBoot 					;modified by mingxuan 2020-9-13
	 ;mov	cx, word [fs:BPB_BytesPerSec]	;modified by mingxuan 2020-9-13
	 mov	cx, word [BPB_BytesPerSec]	;modified by mingxuan 2020-9-17
	 DIV  	ECX  ; EAX = Sector EDX = OFFSET

	 ; 计算FAT表的起始扇区号
	 ; added by mingxuan 2020-9-17
	 ;mov	ebx, dword [ fs:OffsetOfActiPartStartSec ]
	 ;add	bx, word [ fs:BPB_RsvdSecCnt ]

	 ; 设置缓冲区地址
	 ;ADD  	EAX, FAT_START_SECTOR ;deleted by mingxuan 2020-9-17
	 ADD  	EAX, [FAT_START_SECTOR] ;modified by mingxuan 2020-9-17
	 ;add	eax, ebx			  ;modified by mingxuan 2020-9-17
	 MOV  	DWORD [ BP - DAP_SECTOR_LOW ], EAX 
	   
	 call  ReadSector
	  
	 ; 检查下一个簇
	 MOV  	DI,DX
	 ADD  	DI,DATA_BUF_OFF
	 MOV  	EAX, DWORD[DI]  ; EAX = 下一个要读的簇号
	 AND  	EAX, CLUSTER_MASK
	 MOV  	DWORD[ BP - CURRENT_CLUSTER ],EAX
	 CMP  	EAX,CLUSTER_LAST  ; CX >= 0FFFFFF8H，则意味着没有更多的簇了

	 JB  _NEXT_ROOT_CLUSTER

	;for test, added by mingxuan 2020-9-10
	;call 	DispStr		;    

	 JMP  _MISSING_LOADER

_FOUND_LOADER:

	;for test, added by mingxuan 2020-9-10
	;call 	DispStr		; 
	;JMP  	$ ;for test, added by mingxuan 2020-9-10

	 ; 目录结构地址放在DI中
	 pop  di
	 xor  eax, eax
	 mov  ax, [di + OFF_START_CLUSTER_HIGH] ; 起始簇号高32位
	 shl  ax, 16
	 mov  ax, [di + OFF_START_CLUSTER_LOW]  ; 起始簇号低32位
	 mov  dword [ bp - CURRENT_CLUSTER ], eax
	 mov  cx, OSLOADER_SEG      ; CX  = 缓冲区段地址 

	; 清屏
	; for test, added by mingxuan 2020-9-15
	;pusha
	;mov		dx, 0184fh		; 右下角: (80, 50)
	;int		10h				; int 10h
	;popa


_NEXT_DATA_CLUSTER:
	 ; 根据簇号计算扇区号
	 DEC  EAX
	 DEC  EAX  
	 XOR  EBX,EBX 
	 
	 ;MOV  BL, BYTE [ BPB_SecPerClu ]	;deleted by mingxuan 2020-9-13
	 ;mov  es, BaseOfBoot 				;modified by mingxuan 2020-9-13
	 ;mov  bl, byte [fs:BPB_SecPerClu]	;modified by mingxuan 2020-9-13
	 mov  bl, [BPB_SecPerClu]	;modified by mingxuan 2020-9-17
	 
	 MUL  EBX 
	 ;ADD  EAX, DATA_START_SECTOR ;deleted by mingxuan 2020-9-17
	 ADD  EAX, [DATA_START_SECTOR] ;modified by mingxuan 2020-9-17
	 MOV  DWORD[ BP - DAP_SECTOR_LOW  ], EAX
	 
	 ;MOV  BL , BYTE [BPB_SecPerClu]
	 ;mov  bl, byte [fs:BPB_SecPerClu]	;modified by mingxuan 2020-9-13
	 mov  bl, byte [BPB_SecPerClu]	;modified by mingxuan 2020-9-17

	 ; 设置缓冲区
	 MOV  WORD [ BP - DAP_BUFFER_SEG   ], CX
	 MOV  WORD [ BP - DAP_BUFFER_OFF   ], OSLOADER_SEG_OFF

_NEXT_DATA_SECTOR:
	 ; 读取簇中的每个扇区(内层循环)
	 ; 注意 : 通过检查文件大小，可以避免读取最后一个不满簇的所有大小
	 call  ReadSector
	 
	 ; 更新地址，继续读取
	 ;MOV   AX, WORD [BPB_BytesPerSec] 		;deleted by mingxuan 2020-9-13
	 ;mov	es, BaseOfBoot 					;modified by mingxuan 2020-9-13
	 ;mov	ax, word [fs:BPB_BytesPerSec]	;modified by mingxuan 2020-9-13
	 mov	ax, word [BPB_BytesPerSec]	;modified by mingxuan 2020-9-17

	 ADD  WORD  [BP - DAP_BUFFER_OFF], ax 
	 INC  DWORD [BP - DAP_SECTOR_LOW]  ; 递增扇区号
	 DEC  BL        ; 内层循环计数

	 JNZ  _NEXT_DATA_SECTOR

	;for test, added by mingxuan 2020-9-10
	;call 	DispStr		; 
	;JMP  	$ ;for test, added by mingxuan 2020-9-10

	  
	 ; 更新读取下一个簇的缓冲区地址
	 ;MOV  	CL, BYTE [ BPB_SecPerClu ]
	 ;mov  	es, BaseOfBoot 				;modified by mingxuan 2020-9-13
	 ;mov  	cl, byte [fs:BPB_SecPerClu]	;modified by mingxuan 2020-9-13
	 mov  	cl, [BPB_SecPerClu]			;modified by mingxuan 2020-9-17

	 ;MOV   AX, WORD [BPB_BytesPerSec]		;deleted by mingxuan 2020-9-13
	 ;mov	es, BaseOfBoot 					;modified by mingxuan 2020-9-13
	 ;mov	ax, word [fs:BPB_BytesPerSec]	;modified by mingxuan 2020-9-13
	 mov	ax, [BPB_BytesPerSec]			;modified by mingxuan 2020-9-17

	 SHR  	AX, 4
	 MUL  	CL
	 ADD  	AX, WORD [ BP - DAP_BUFFER_SEG ] 
	 MOV  	CX, AX ; 保存下一个簇的缓冲区段地址
	 
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

	 ;MOV  BX,WORD [ BPB_BytesPerSec ]		;deleted by mingxuan 2020-9-13
	 ;mov	es, BaseOfBoot 					;modified by mingxuan 2020-9-13
	; mov	bx, word [fs:BPB_BytesPerSec]	;modified by mingxuan 2020-9-13
	 mov	bx, [BPB_BytesPerSec]			;modified by mingxuan 2020-9-17

	 DIV  	EBX   ; EAX = Sector  EDX = Offset
	 
	 ; 计算FAT表的起始扇区号
	 ; added by mingxuan 2020-9-17
	 ;mov	ebx, dword [ fs:OffsetOfActiPartStartSec ]
	 ;add	bx, word [ fs:BPB_RsvdSecCnt ]

	 ; 设置int 13h读取的绝对扇区号
	 ;ADD  EAX, FAT_START_SECTOR ;deleted by mingxuan 2020-9-17
	 ADD  EAX, [FAT_START_SECTOR] ;modified by mingxuan 2020-9-17
	 ;add  eax, ebx	;modified by mingxuan 2020-9-17
	 MOV  DWORD [ BP - DAP_SECTOR_LOW ], EAX

	 ; 设置int 13h写入内存的缓冲区地址 
	 MOV  WORD [BP - DAP_BUFFER_SEG  ], 01000H 
	 MOV  WORD [BP - DAP_BUFFER_OFF  ], DATA_BUF_OFF

	 ; 读取扇区 ; 把FAT表读进内存
	 CALL  ReadSector
	  
	 ; 检查下一个簇
	 MOV  DI,DX
	 ADD  DI,DATA_BUF_OFF
	 MOV  EAX,DWORD[DI]  ; EAX = 下一个要读的簇号
	 AND  EAX,CLUSTER_MASK
	 MOV  DWORD[ BP - CURRENT_CLUSTER ],EAX
	 CMP  EAX,CLUSTER_LAST  ; CX >= 0FFFFFF8H，则意味着没有更多的簇了

	;for test, added by mingxuan 2020-9-10
	;call 	DispStr		; 
	;JMP  	$ ;for test, added by mingxuan 2020-9-10

	; 清屏
	; for test, added by mingxuan 2020-9-15
	;pusha
	;mov		dx, 0184fh		; 右下角: (80, 50)
	;int		10h				; int 10h
	;popa


	 JB  _NEXT_DATA_CLUSTER

_RUN_LOADER: 

	;mov  	dl, [BS_DriveNum]			;deleted by mingxuan 2020-9-13
	;mov		es, BaseOfBoot 				;modified by mingxuan 2020-9-13
	;mov		dl, [fs:BS_DriveNum]		;modified by mingxuan 2020-9-13
	mov		dl, [BS_DriveNum]		;modified by mingxuan 2020-9-17

	;for test, added by mingxuan 2020-9-10
	;call	DispStr		; 
	;jmp $ ;for test 2020-9-10, mingxuan

	; 清屏
	; for test, added by mingxuan 2020-9-15
	;pusha
	;mov		dx, 0184fh		; 右下角: (80, 50)
	;int		10h				; int 10h
	;popa


	jmp  OSLOADER_SEG : OSLOADER_SEG_OFF


LoaderName     db "LOADER  BIN"       ; 第二阶段启动程序 FDOSLDR.BIN

; for display
; added by mingxuan 2020-9-4
;TestMessage:		db	"#"	; 27字节, 不够则用空格补齐. 序号 0

; for display
; added by mingxuan 2020-9-4
;DispStr:
;	pusha

;	mov		ax, TestMessage
;	mov		bp, ax
;	mov		cx, 1
;	mov		ax, 01301h		; AH = 13,  AL = 01h
;	mov		bx, 0007h		; 页号为0(BH = 0) 黑底白字(BL = 07h)

;	int		10h			; int 10h

;	popa
	
;	ret


times 	510-($-$$)	db	0	; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw 	0xaa55				; 结束标志
