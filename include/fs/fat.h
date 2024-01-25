#pragma once

#include <stdint.h>

//! fs io result
#define OK                1 //<! succeed
#define SYSERROR          2 //<! system error
#define VDISKERROR        3 //<! virtual disk error
#define INSUFFICIENTSPACE 4 //<! virtual disk out of memory
#define WRONGPATH         5 //<! invalid path
#define NAMEEXIST         6 //<! entry already exists
#define ACCESSDENIED      7 //<! bad access permission

//! fs io flags
#define C  1 //<! create
#define R  5 //<! read
#define W  6 //<! write
#define RW 2 //<! read & write

//! fs entry type
#define F 1 //<! file
#define D 0 //<! directory

enum fat32_entry_attr {
    ATTR_RW      = 0,      //<! read & write
    ATTR_RO      = 1 << 0, //<! read only
    ATTR_HID     = 1 << 1, //<! hidden
    ATTR_SYS     = 1 << 2, //<! system
    ATTR_VOL     = 1 << 3, //<! volume
    ATTR_SUBDIR  = 1 << 4, //<! sub directory
    ATTR_ARCHIVE = 1 << 5, //<! archive
};

typedef struct {
    u8  BS_jmpBoot[3];
    u8  BS_OEMName[8];
    u16 BPB_BytsPerSec;
    u8  BPB_SecPerClus;
    u16 BPB_RsvdSecCnt;
    u8  BPB_NumFATs;
    u16 BPB_RootEntCnt;
    u16 BPB_TotSec16;
    u8  BPB_Media;
    u16 BPB_FATSz16;
    u16 BPB_SecPerTrk;
    u16 BPB_NumHeads;
    u32 BPB_HiddSec;
    u32 BPB_TotSec32;
    u32 BPB_FATSz32;
    u16 BPB_ExtFlags;
    u16 BPB_FSVer;
    u32 BPB_RootClus;
    u16 BPB_FSInfo;
    u16 BPB_BkBootSec;
    u8  BPB_Reserved[12];
    u8  BS_DrvNum;
    u8  BS_Reserved1;
    u8  BS_BootSig;
    u32 BS_VolID;
    u8  BS_VolLabp[11];
    u8  BS_FilSysType[8];
    u8  zero[420];
    u16 Signature_word;
} __attribute__((packed)) fat32_bpb_t;

typedef struct {
    u8  name[8];            //<! file name
    u8  ext[3];             //<! extension
    u8  attr;               //<! attribute, compound with fat32_entry_attr
    u8  reserved;           //<! reserved for system
    u8  creation_ms;        //<! creation time, in 10ms
    u16 creation_time;      //<! creation time, include hour/min/sec
    u16 creation_date;      //<! creation date, include year/month/day
    u16 last_access_date;   //<! last access date, include year/month/day
    u16 start_clus_hi;      //<! higher 16 bit of start cluster
    u16 last_modified_time; //<! last modified time, hour/min/sec
    u16 last_modified_date; //<! last modified date, include year/month/day
    u16 start_clus_lo;      //<! lower 16 bit of start cluster
    u32 size;               //<! file size, in bytes
} fat32_entry_t;

typedef struct {
    u8  attr;       //<! attribute, compound with fat32_entry_attr
    u8  name1[10];  //<! 1st name part
    u8  flag;       //<! lfn entry flag, always 0x0f
    u8  reserved;   //<! reserved for system
    u8  checksum;   //<! checksum of short name
    u8  name2[12];  //<! 2nd name part
    u16 start_clus; //<! start cluster, always 0 currently
    u8  name3[4];   //<! 3rd name path
} fat32_lfn_entry_t;
