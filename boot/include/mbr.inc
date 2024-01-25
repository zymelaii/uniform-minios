BaseOfMBR             equ 0000h
OffsetOfMBR           equ 7000h
StartOfMBRTable       equ 1beh
EndOfMBRTable         equ 1feh

MBRPartitionState     equ 00h
MBRFirstCHSAddress    equ 01h
MBRPartitionType      equ 04h
MBRLastCHSAddress     equ 05h
MBRFirstLBA           equ 08h
MBRSectorNumber       equ 0ch
MBRPartitionEntrySize equ 10h

SizeOfMBR             equ StartOfMBRTable
