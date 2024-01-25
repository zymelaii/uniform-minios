BaseOfLoader		equ 1000h
OffsetOfLoader		equ 0000h
BaseOfLoaderPhyAddr	equ BaseOfLoader * 10h + OffsetOfLoader

BaseOfDeviceInfo	equ 9000h
OffsetOfVideoCard	equ 00h
OffsetOfVGAInfo		equ 04h
OffsetOfARDSCount	equ 0ah
OffsetOfARDSBuffer	equ 0eh

StackTopInProtect	equ	BaseOfLoaderPhyAddr

SelectorFlatC	equ 0x08
SelectorFlatRW	equ 0x10
