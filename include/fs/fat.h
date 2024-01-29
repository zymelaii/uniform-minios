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
    uint8_t  BS_jmpBoot[3];
    uint8_t  BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t  BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t  BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t  BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t  BPB_Reserved[12];
    uint8_t  BS_DrvNum;
    uint8_t  BS_Reserved1;
    uint8_t  BS_BootSig;
    uint32_t BS_VolID;
    uint8_t  BS_VolLabp[11];
    uint8_t  BS_FilSysType[8];
    uint8_t  zero[420];
    uint16_t Signature_word;
} __attribute__((packed)) fat32_bpb_t;

typedef struct {
    uint8_t  name[8];            //<! file name
    uint8_t  ext[3];             //<! extension
    uint8_t  attr;               //<! attribute, compound with fat32_entry_attr
    uint8_t  reserved;           //<! reserved for system
    uint8_t  creation_ms;        //<! creation time, in 10ms
    uint16_t creation_time;      //<! creation time, include hour/min/sec
    uint16_t creation_date;      //<! creation date, include year/month/day
    uint16_t last_access_date;   //<! last access date, include year/month/day
    uint16_t start_clus_hi;      //<! higher 16 bit of start cluster
    uint16_t last_modified_time; //<! last modified time, hour/min/sec
    uint16_t last_modified_date; //<! last modified date, include year/month/day
    uint16_t start_clus_lo;      //<! lower 16 bit of start cluster
    uint32_t size;               //<! file size, in bytes
} fat32_entry_t;

typedef struct {
    uint8_t  attr;       //<! attribute, compound with fat32_entry_attr
    uint8_t  name1[10];  //<! 1st name part
    uint8_t  flag;       //<! lfn entry flag, always 0x0f
    uint8_t  reserved;   //<! reserved for system
    uint8_t  checksum;   //<! checksum of short name
    uint8_t  name2[12];  //<! 2nd name part
    uint16_t start_clus; //<! start cluster, always 0 currently
    uint8_t  name3[4];   //<! 3rd name path
} fat32_lfn_entry_t;
