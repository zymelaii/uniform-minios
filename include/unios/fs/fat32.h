#pragma once

#include <stdint.h>

//! fs io result
#define OK                1 //<! 成功
#define SYSERROR          2 //<! 系统错误
#define VDISKERROR        3 //<! 虚拟磁盘错误
#define INSUFFICIENTSPACE 4 //<! 虚拟磁盘空间不足
#define WRONGPATH         5 //<! 路径有误
#define NAMEEXIST         6 //<! 文件或目录名已存在
#define ACCESSDENIED      7 //<! 读写权限不对拒绝访问

//! fs io flags
#define C  1 //<! 创建
#define R  5 //<! 读
#define W  6 //<! 写
#define RW 2 //<! 读写

//! fs entry type
#define F 1 //<! 文件
#define D 0 //<! 目录

enum fat32_entry_attr {
    ATTR_RW      = 0,      //<! read & write
    ATTR_RO      = 1 << 0, //<! read only
    ATTR_HID     = 1 << 1, //<! hidden
    ATTR_SYS     = 1 << 2, //<! system
    ATTR_VOL     = 1 << 3, //<! volume
    ATTR_SUBDIR  = 1 << 4, //<! sub directory
    ATTR_ARCHIVE = 1 << 5, //<! archive
};

typedef struct fat32_entry {
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

typedef struct fat32_lfn_entry {
    u8  attr;       //<! attribute, compound with fat32_entry_attr
    u8  name1[10];  //<! 1st name part
    u8  flag;       //<! lfn entry flag, always 0x0f
    u8  reserved;   //<! reserved for system
    u8  checksum;   //<! checksum of short name
    u8  name2[12];  //<! 2nd name part
    u16 start_clus; //<! start cluster, always 0 currently
    u8  name3[4];   //<! 3rd name path
} fat32_lfn_entry_t;
