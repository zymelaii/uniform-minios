; [in]: OffsetOfBPB
BS_jmpBoot     equ OffsetOfBPB + 0
BS_OEMName     equ OffsetOfBPB + 3
BPB_BytsPerSec equ OffsetOfBPB + 11
BPB_SecPerClus equ OffsetOfBPB + 13
BPB_RsvdSecCnt equ OffsetOfBPB + 14
BPB_NumFATs    equ OffsetOfBPB + 16
BPB_RootEntCnt equ OffsetOfBPB + 17
BPB_TotSec16   equ OffsetOfBPB + 19
BPB_Media      equ OffsetOfBPB + 21
BPB_FATSz16    equ OffsetOfBPB + 22
BPB_SecPerTrk  equ OffsetOfBPB + 24
BPB_NumHeads   equ OffsetOfBPB + 26
BPB_HiddSec    equ OffsetOfBPB + 28
BPB_TotSec32   equ OffsetOfBPB + 32
BPB_FATSz32    equ OffsetOfBPB + 36
BPB_ExtFlags   equ OffsetOfBPB + 40
BPB_FSVer      equ OffsetOfBPB + 42
BPB_RootClus   equ OffsetOfBPB + 44
BPB_FSInfo     equ OffsetOfBPB + 48
BPB_BkBootSec  equ OffsetOfBPB + 50
BPB_Reserved   equ OffsetOfBPB + 52
BS_DrvNum      equ OffsetOfBPB + 64
BS_Reserved1   equ OffsetOfBPB + 65
BS_BootSig     equ OffsetOfBPB + 66
BS_VolID       equ OffsetOfBPB + 67
BS_VolLab      equ OffsetOfBPB + 71
BS_FilSysType  equ OffsetOfBPB + 82
EndOfBPB       equ OffsetOfBPB + 90

DIR_Name         equ 0
DIR_Attr         equ 11
DIR_NTRes        equ 12
DIR_CrtTimeTenth equ 13
DIR_CrtTime      equ 14
DIR_CrtDate      equ 16
DIR_LstAccDate   equ 18
DIR_FstClusHI    equ 20
DIR_WrtTime      equ 22
DIR_WrtDate      equ 24
DIR_FstClusLO    equ 26
DIR_FileSize     equ 28
SizeOfDIR        equ 32

CLUSTER_Free equ 00000000h
CLUSTER_Base equ 00000002h
CLUSTER_Bad  equ 0FFFFFF7h
CLUSTER_Last equ 0FFFFFFFh
CLUSTER_Mask equ 0FFFFFFFh
