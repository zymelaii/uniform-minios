BaseOfBoot 					equ	1000h 		; 段地址，mbr加载loader到这个段
OffsetOfBoot				equ	7c00h		; load Boot sector to BaseOfBoot:OffsetOfBoot 
OffsetOfActiPartStartSec	equ 7e00h		; 活动分区的起始扇区号相对于BaseOfBoot的偏移量	;added by mingxuan 2020-9-12 
											; 该变量来自分区表，保存在该内存地址，用于在os_boot和loader中查找FAT32文件

org 07c00h

LABEL_START:	
	mov		ax, cs
	mov		ds, ax
	mov		es, ax
	mov		ss, ax
	mov		sp, 7c00h

	; 清屏
	mov		ax, 0600h		; AH = 6,  AL = 0h
	mov		bx, 0700h		; 黑底白字(BL = 07h)
	mov		cx, 0			; 左上角: (0, 0)
	mov		dx, 0184fh		; 右下角: (80, 50)
	int		10h				; int 10h

	mov		dh, 0			; 
	call 	DispStr		; 
	
	
	xor		ah, ah	; ┓
	xor		dl, dl	; ┣ 软驱复位
	int		13h		; ┛

	;循环读取分区表
	mov		bx, 0				;每次循环累加16
	mov		dh, 1				;每次循环累加1

	jmp CHECK_PARTITION  ;added by mingxuan 2020-9-29

;added by mingxuan 2020-9-29
LABLE_END_EXTENDED:

	mov		byte [EndInExt], 0 ;将EndInExt置0，复位
	
	; 检查扩展分区表的第二项是否是空项。如果是空项则表明当前逻辑分区就是最后一个。
	add		bx, 16
	mov		cl, [es:7c00h+446+bx+4]		 ;分区类型
	;sub	bx, 16						;deleted by mingxuan 2020-9-30

	cmp		cl, 0
	jz		RESET_SecOffset_SELF

CHECK_PARTITION:
	mov		dl, [es:7c00h+446+bx]		 ;分区活跃标志地址

	;mov	ax, word [es:7c00h+446+bx+8]   ;分区起始扇区地址 ;deletd by mingxuan 2020-9-12
	mov		eax, dword [es:7c00h+446+bx+8] ;分区起始扇区地址 ;modified by mingxuan 2020-9-12
										   ;修改为eax的原因: 分区表用4个字节来表示起始扇区，而不是2个字节, mingxuan 

	;add	ax, [SecOffset]	 				;deletd by mingxuan 2020-9-12
	;add	eax, [SecOffset] 				;modified by mingxuan 2020-9-29
							 				;deleted by mingxuan 2020-9-29
	mov		cl, [es:7c00h+446+bx+4]		 	;分区类型
		
	cmp		cl, 5  				;extended partition type = 5
	;jz		LABLE_EXTENDED
	jz		LABLE_IN_EXTENDED	;modified by mingxuan 2020-9-29

	add		eax, [SecOffset_SELF]	;added by mingxuan 2020-9-29

	add		byte [CurPartNo], 1
	add		byte [CurPartNum], 1 ; added by mingxuan 2020-9-29	;deleted by mingxuan 2020-9-30
	
	cmp		dl, 80h
	jz		LABLE_ACTIVE

	; 检查当前的分区表是否是扩展分区的最后一个逻辑分区
	mov		cl, [EndInExt]
	cmp		cl, 1 				;added by mingxuan 2020-9-29
	jz		LABLE_END_EXTENDED	;added by mingxuan 2020-9-29

	cmp		dh, 4
	jz		LABLE_NOT_FOUND

	inc		dh
	add		bx, 16
	jmp		CHECK_PARTITION

	
RESET_SecOffset_SELF:

	; SecOffset_SELF置0的目的是以后又恢复到搜索主分区
	mov 	edx, 0
	mov 	[SecOffset_SELF], edx

	mov    	dl, [EXTNum]
	mov		al, 16
	mul		dl
	mov     bx, ax

	mov		ax, 0		;added by mingxuan 2020-9-30
	mov		es,	ax		;added by mingxuan 2020-9-30

	jmp 	CHECK_PARTITION


; added by mingxuan 2020-9-29
LABLE_IN_EXTENDED:					;在分区表中发现扩展分区后，跳转到这里来执行

	; 此时的eax有两种情况：
	; 1) 当第一次执行该程序时，eax是从主分区表里获得的扩展分区起始的绝对地址
	; 2) 之后的执行, eax是从扩展分区表的第二项获得的起始地址(相对于整个扩展分区起始地址偏移量)
	; 从扩展分区表的第二项获得的起始地址是相对于整个扩展分区起始地址偏移量，所以要加上基地址
	; 特殊情况: 当第一次进入该过程时，SecOffset_EXT的值是0。此时是从主分区表里获得的起始地址，这个地址是绝对地址。
	add		eax, [SecOffset_EXT] ;modified by mingxuan 2020-9-29

	; FirstInExt是标志位，判断是否是第一次进入该过程
	mov		cl, [FirstInExt]
	; 若为1，表示不是第一次进入该过程。
	cmp		cl, 1	
	jz      LABLE_EXTENDED

	; 以下是第一次进入该过程要执行的语句（仅执行一遍）
	; 要记录下扩展分区是主分区表的第几项
	add		byte [CurPartNum], 1		;added by mingxuan 2020-9-30
	mov		cl, [CurPartNum]
	mov    	[EXTNum], cl

	; 当第一次进入该过程时，要记录下扩展分区的起始地址，该地址是以后所有逻辑分区的偏移量的基地址
	mov		[SecOffset_EXT], eax
	mov		byte [FirstInExt], 1

LABLE_EXTENDED:

	;mov	[SecOffset], ax			;deleted by mingxuan 2020-9-12
	;mov	[SecOffset], eax		;deleted by mingxuan 2020-9-12
	;要记录下每个逻辑分区的起始地址，因为每次找自身时需要用这个地址做基地址。（该变量每次都要更新）
	mov		[SecOffset_SELF], eax	;added by mingxuan 2020-9-29

	add		byte [EbrNum], 1
	cmp		byte [EbrNum], 1
	jz		._add_CurPartNo	
._read_ebr:	
	mov 	cl, 1
	mov		bx, BaseOfBoot
	mov 	es, bx
	mov 	bx, OffsetOfBoot
	call 	ReadSector
	mov		bx, 0
	mov		dh, 1

	; 将EndInExt置1，表示此时正在扫描扩展分区，用于之后程序判断当前是否是扩展分区的终点
	mov		[EndInExt], dh		;added by mingxuan 2020-9-29

	jmp 	CHECK_PARTITION
._add_CurPartNo:
	add		byte [CurPartNo], 1
	jmp		._read_ebr
	

LABLE_ACTIVE:
	
	mov 	cl, 1 			 ;要读取的扇区个数

	mov		bx, BaseOfBoot
	mov 	es, bx

	mov     dword [es:OffsetOfActiPartStartSec], eax 	;此时eax中存放活动分区的起始扇区号
												;added by mingxuan 2020-9-12

	mov 	bx, OffsetOfBoot ;对应扇区会被加载到内存的 es:bx 处

	call 	ReadSector
	
	;mov	dh, 1
	;call	DispStr
	;mov	dh, 2
	;call 	DispStr
	
	; mov	ah,0h
	; int	16h

	jmp 	BaseOfBoot:OffsetOfBoot
	
LABLE_NOT_FOUND:
	mov		dh, 3
	call 	DispStr
	jmp	$
	

;SecOffset 		dw 	0 ;deletd by mingxuan 2020-9-12
;SecOffset 		dd 	0 ;modifed by mingxuan 2020-9-29
SecOffset_SELF 	dd 	0 ;modifed by mingxuan 2020-9-29
SecOffset_EXT 	dd 	0 ;modifed by mingxuan 2020-9-29

EbrNum		db	0

FirstInExt	db  0 ;added by mingxuan 2020-9-29
EndInExt	db	0 ;added by mingxuan 2020-9-29
EXTNum		db  0 ;added by mingxuan 2020-9-29
CurPartNum  db  0 ;added by mingxuan 2020-9-29

MessageLength		equ	27
BootMessage:		db	"Finding active partition..."	; 27字节, 不够则用空格补齐. 序号 0

Message1			db	"    partition "
CurPartNo			db	"0"
					db	":     active"
;Message2			db	"press any key to continue  "	
Message3			db	"active partition not found!"	 

DispStr:
	push	ax
	push	dx
	mov		ax, MessageLength
	mul		dh
	add		ax, BootMessage
	mov		bp, ax			; ┓
	mov		ax, ds			; ┣ ES:BP = 串地址
	mov		es, ax			; ┛
	mov		cx, MessageLength	; CX = 串长度
	mov		ax, 01301h		; AH = 13,  AL = 01h
	mov		bx, 0007h		; 页号为0(BH = 0) 黑底白字(BL = 07h)
	mov		dl, 0
	int		10h			; int 10h
	pop		dx
	pop 	ax
	ret


;----------------------------------------------------------------------------
; 函数名: ReadSector (使用扩展int13 ah=42)
;----------------------------------------------------------------------------
; 作用:
;	从第 ax 个 Sector 开始, 将 cl 个 Sector 读入 es:bx 中

DAPS:   
	DB 0x10               		; size of packet 
    DB 0                  		; Always 0
	D_CL	DW 1          		; number of sectors to transfer
	D_BX	DW OffsetOfBoot     ; transfer buffer (16 bit segment:16 bit offset) 
	D_ES	DW BaseOfBoot	
	LBA_Lo	DD 1	      		; lower 32-bits of 48-bit starting LBA
	LBA_Hi	DD 0	      		; upper 32-bits of 48-bit starting LBAs

ReadSector:
	mov	[D_CL],   cl
	mov	[D_BX],   bx
	mov	[D_ES],   es
	;mov	[LBA_Lo], ax	;deleted by mingxuan 2020-9-17
	mov	[LBA_Lo], eax		;modified by mingxuan 2020-9-17
							;修改为eax的原因: 分区表用4个字节来表示起始扇区，而不是2个字节, mingxuan 
	mov	dl, 0x80		

.GoOnReading:
	mov		ah, 42h			
	mov 	si, DAPS
	int		13h
	jc	.GoOnReading		; 如果读取错误 CF 会被置为 1, 这时就不停地读, 直到正确为止

	ret


times 444 -($-$$) db 0
dw 0x0000