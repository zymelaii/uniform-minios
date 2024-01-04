	;deleted by mingxuan 2020-9-16
	;BS_OEM			DB	'mkfs.fat'
	;BPB_BytesPerSec	DW	0x200
	;BPB_SecPerClu 	DB	1
	;BPB_RsvdSecCnt	DW	0x20
	;BPB_NumFATs		DB	2
	;BPB_RootEntCnt	DW 	0
	;BPB_TotSec16	DW	0
	;BPB_Media		DB 	0xf8
	;BPB_FATSz16		DW	0
	;BPB_SecPerTrk	DW	0x20
	;BPB_NumHeads	DW	0x40
	;BPB_HiddSec		DD	0
	;BPB_TotSec32	DD	0x015791
	;BS_SecPerFAT	DD	0x02a5
	;BS_Flag			DW	0
	;BS_Version		DW	0
	;BS_RootClus		DD	2
	;BS_FsInfoSec 	DW	1
	;BS_BackBootSec 	DW	6
	;BS_Unuse1		DD	0
	;BS_Unuse2		DD	0
	;BS_Unuse3		DD 	0
	;BS_DriveNum		DB	0x80
	;BS_Unuse4		DB	0
	;BS_ExtBootFlag	DB	0x29
	;BS_VolNum		DD	0xbe3a8ff5
	;BS_VolName		DB	'MZY hd boot'

; added by mingxuan 2020-9-12
;BaseOfBoot 			equ	1000h 		; added by mingxuan 2020-9-12
;OffsetOfBoot		equ	7c00h		; load Boot sector to BaseOfBoot:OffsetOfBoot

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

