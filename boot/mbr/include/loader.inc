;==============================================================================================================================
BaseOfStack		equ 	0x0100
STACK_ADDR		equ		0x0ea
SEG_ADDR		equ		0x09000
DATA_BUF_OFF	equ 	0x09000

BaseOfBoot 					equ	1000h 		; added by mingxuan 2020-9-17
OffsetOfBoot				equ	7c00h		; load Boot sector to BaseOfBoot:OffsetOfBoot
OffsetOfActiPartStartSec	equ 7e00h		; 活动分区的起始扇区号相对于BaseOfBoot的偏移量	;added by mingxuan 2020-9-17 
											; 该变量来自分区表，保存在该内存地址，用于在os_boot和loader中查找FAT32文件

;FAT_START_SECTOR 	equ  	0x820	  	; FAT表的起始扇区号  DWORD
;FAT_START_SECTOR 	equ  	0x1020	  	; FAT表的起始扇区号  DWORD ; for test 2020-9-12, mingxuan
;DATA_START_SECTOR 	equ  	0xd6a	  	; 数据区起始扇区号  DWORD
;DATA_START_SECTOR 	equ  	0x156a	  	; 数据区起始扇区号  DWORD ; for test 2020-9-12, mingxuan
;DATA_START_SECTOR 	equ  	0x13a4	  	; 数据区起始扇区号  DWORD ; for test 2020-9-16, mingxuan

DIR_PER_SECTOR  	equ  	0x10  		; 每个扇区所容纳的目录 BYTE

; 扩展磁盘服务所使用的地址包
DAP_SECTOR_HIGH  	equ  	4  		; 起始扇区号的高32位 ( 每次调用需要重置 ) DWORD
DAP_SECTOR_LOW  	equ  	8  		; 起始扇区号的低32位 ( 每次调用需要重置 ) DWORD
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

BaseOfLoader		equ	 09000h	; LOADER.BIN 被加载到的位置 ----  段地址
OffsetOfLoader		equ	  0100h	; LOADER.BIN 被加载到的位置 ---- 偏移地址

BaseOfLoaderPhyAddr	equ	BaseOfLoader * 10h	; LOADER.BIN 被加载到的位置 ---- 物理地址 (= BaseOfLoader * 10h)

BaseOfKernelFile	equ	 07000h	; KERNEL.BIN 被加载到的位置 ----  段地址
OffsetOfKernelFile	equ	     0h	; KERNEL.BIN 被加载到的位置 ---- 偏移地址

BaseOfKernelFilePhyAddr	equ	BaseOfKernelFile * 10h

BaseOfEchoFile	equ		  07E0h	; KERNEL.BIN 被加载到的位置 ----  段地址
OffsetOfEchoFile	equ	     0h	; KERNEL.BIN 被加载到的位置 ---- 偏移地址

BaseOfEchoFilePhyAddr	equ	BaseOfKernelFile * 10h

KernelEntryPointPhyAddr	equ	0xC0030400	; 注意：1、必须与 MAKEFILE 中参数 -Ttext 的值相等!!
						;       2、这是个地址而非仅仅是个偏移

PageDirBase		equ	200000h	; 页目录开始地址:		2M
PageTblBase		equ	201000h	; 页表开始地址:			2M + 4K

PageTblNumAddr		equ 500h;页表数量放在这个位置	delete by visual 2016.4.28

FMIBuff			equ	007ff000h

;==============================================================================================================================

