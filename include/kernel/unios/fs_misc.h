#pragma once

#include <stdint.h>
#include <limits.h>

#define OK                1 // 正常返回
#define SYSERROR          2 // 系统错误
#define VDISKERROR        3 // 虚拟磁盘错误
#define INSUFFICIENTSPACE 4 // 虚拟磁盘空间不足
#define WRONGPATH         5 // 路径有误
#define NAMEEXIST         6 // 文件或目录名已存在
#define ACCESSDENIED      7 // 读写权限不对拒绝访问

#define C  1 // 创建	//added by mingxuan 2019-5-18
#define R  5 // 读
#define W  6 // 写
#define RW 2 // 读写	//added by mingxuan 2019-5-18

#define F 1 // 文件
#define D 0 // 目录

typedef int STATE; // 函数返回状态

typedef unsigned char  BYTE;  // 字节
typedef unsigned short WORD;  // 双字节
typedef unsigned long  DWORD; // 四字节
typedef unsigned int   UINT;  // 无符号整型
typedef char           CHAR;  // 字符类型

typedef unsigned char  *PBYTE;
typedef unsigned short *PWORD;
typedef unsigned long  *PDWORD; // 四字节指针
typedef unsigned int   *PUINT;  // 无符号整型指针
typedef char           *PCHAR;  // 字符指针

typedef struct // 定义目录项：占32个字节
{
    BYTE  filename[8];      // 文件名：占8个字节
    BYTE  extension[3];     // 扩展名：占3个字节
    BYTE  proByte;          // 属性字节：占1个字节
    BYTE  sysReserved;      // 系统保留：占1个字节
    BYTE  createMsecond;    // 创建时间的10毫秒位：占1个字节
    WORD  createTime;       // 文件创建时间：占2个字节
    WORD  createDate;       // 文件创建日期：占2个字节
    WORD  lastAccessDate;   // 文件最后访问日期：占2个字节
    WORD  highClusterNum;   // 文件的起始簇号的高16位：占2个字节
    WORD  lastModifiedTime; // 文件的最近修改时间
    WORD  lastModifiedDate; // 文件的最近修改日期
    WORD  lowClusterNum;    // 文件的起始簇号的低16位：占2个字节
    DWORD filelength;       // 文件的长度：占4个字节
} Record, *PRecord;

typedef struct // 定义长文件名目录项：占32个字节
{
    BYTE proByte;      // 属性字节
    BYTE name1[10];    // 长文件名第1段
    BYTE longNameFlag; // 长文件名目录项标志，取值0FH
    BYTE sysReserved;  // 系统保留
    BYTE name2[19];    // 长文件名第2段
} LONGNAME;

typedef struct {       // 文件类型
    CHAR  parent[256]; // 父路径
    CHAR  name[256];   // 文件名
    DWORD start;       // 起始地址
    DWORD off;         // 总偏移量,以字节为单位
    DWORD size;        // 文件的大小，以字节为单位
    UINT  flag;        // 文件读写标志
} File, *PFile;

typedef struct { // 动态数组元素类型，用于存储文件或目录的基本信息
    CHAR fullpath[256]; // 绝对路径
    CHAR name[256];     // 文件名或目录名
    UINT tag;           // 1表示文件，0表示目录
} DArrayElem;

typedef struct {           // 动态数组类型
    DArrayElem *base;      // 数组基地址
    UINT        offset;    // 读取数组时的偏移量
    UINT        used;      // 数组当前已使用的容量
    UINT        capacity;  // 数组的总容量
    UINT        increment; // 当数组容量不足时，动态增长的步长
} DArray;

typedef struct {        // 文件或目录属性类型
    BYTE type;          // 0x10表示目录，否则表示文件
    CHAR name[256];     // 文件或目录名称
    CHAR location[256]; // 文件或目录的位置，绝对路径
    DWORD size; // 文件的大小或整个目录中(包括子目录中)的文件总大小
    CHAR createTime[20];       // 创建时间型如：yyyy-MM-dd hh:mm:ss类型
    CHAR lastModifiedTime[20]; // 最后修改时间

    union {
        CHAR lastAccessDate[11]; // 最后访问时间，当type为文件值有效
        UINT contain[2]; // 目录中的文件个数和子目录的个数，当type为目录时有效
    } share;
} Properties;

/**
 * @struct dev_drv_map fs.h "include/sys/fs.h"
 * @brief  The Device_nr.\ - Driver_nr.\ MAP.
 */
struct dev_drv_map {
    int driver_nr; /**< The proc nr.\ of the device driver. */
};

/**
 * @def   MAGIC_V1
 * @brief Magic number of FS v1.0
 */
#define MAGIC_V1 0x111

/**
 * @struct super_block fs.h "include/fs.h"
 * @brief  The 2nd sector of the FS
 *
 * Remember to change SUPER_BLOCK_SIZE if the members are changed.
 */
typedef struct super_block {
    u32 magic;             /**< Magic number */
    u32 nr_inodes;         /**< How many inodes */
    u32 nr_sects;          /**< How many sectors */
    u32 nr_imap_sects;     /**< How many inode-map sectors */
    u32 nr_smap_sects;     /**< How many sector-map sectors */
    u32 n_1st_sect;        /**< Number of the 1st data sector */
    u32 nr_inode_sects;    /**< How many inode sectors */
    u32 root_inode;        /**< Inode nr of root directory */
    u32 inode_size;        /**< INODE_SIZE */
    u32 inode_isize_off;   /**< Offset of `struct inode::i_size' */
    u32 inode_start_off;   /**< Offset of `struct inode::i_start_sect' */
    u32 dir_ent_size;      /**< DIR_ENTRY_SIZE */
    u32 dir_ent_inode_off; /**< Offset of `struct dir_entry::inode_nr' */
    u32 dir_ent_fname_off; /**< Offset of `struct dir_entry::name' */

    /*
     * the following item(s) are only present in memory
     */
    int sb_dev;  /**< the super block's home device */
    int fs_type; // added by mingxuan 2020-10-30
} superblock_t;

/**
 * @def   SUPER_BLOCK_SIZE
 * @brief The size of super block \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
// #define	SUPER_BLOCK_SIZE	56
#define SUPER_BLOCK_SIZE 64 // modified by mingxuan 2020-10-30

/**
 * @struct inode
 * @brief  i-node
 *
 * The \c start_sect and\c nr_sects locate the file in the device,
 * and the size show how many bytes is used.
 * If <tt> size < (nr_sects * SECTOR_SIZE) </tt>, the rest bytes
 * are wasted and reserved for later writing.
 *
 * \b NOTE: Remember to change INODE_SIZE if the members are changed
 */
struct inode {
    u32 i_mode;       /**< Accsess mode */
    u32 i_size;       /**< File size */
    u32 i_start_sect; /**< The first sector of the data */
    u32 i_nr_sects;   /**< How many sectors the file occupies */
    u8  _unused[16];  /**< Stuff for alignment */

    /* the following items are only present in memory */
    int i_dev;
    int i_cnt; /**< How many procs share this inode  */
    int i_num; /**< inode nr.  */
};

/**
 * @def   INODE_SIZE
 * @brief The size of i-node stored \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define INODE_SIZE 32

/**
 * @struct dir_entry
 * @brief  Directory Entry
 */
struct dir_entry {
    int  inode_nr;           /**< inode nr. */
    char name[FILENAME_MAX]; /**< Filename */
};

/**
 * @def   DIR_ENTRY_SIZE
 * @brief The size of directory entry in the device.
 *
 * It is as same as the size in memory.
 */
#define DIR_ENTRY_SIZE sizeof(struct dir_entry)

/**
 * @struct file_desc
 * @brief  File Descriptor
 */

// added by mingxuan 2019-5-17
union ptr_node {
    struct inode *fd_inode; /**< Ptr to the i-node */
    PFile         fd_file;  // 指向fat32的file结构体
};

typedef struct file_desc {
    int fd_mode; /**< R or W */
    int fd_pos;  /**< Current position for R/W. */
    // struct inode*	fd_inode;	/**< Ptr to the i-node */	//deleted by
    // mingxuan 2019-5-17

    // added by mingxuan 2019-5-17
    union ptr_node fd_node;
    int            flag; // 用于标志描述符是否被使用
    int            dev_index;
} file_desc_t;

/**
 * Since all invocations of `rw_sector()' in FS look similar (most of the
 * params are the same), we use this macro to make code more readable.
 */
#define RD_SECT(dev, sect_nr, fsbuf)                 \
 rw_sector(                                          \
     DEV_READ,                                       \
     dev,                                            \
     (sect_nr)*SECTOR_SIZE,                          \
     SECTOR_SIZE,              /* read one sector */ \
     proc2pid(p_proc_current), /*TASK_A*/            \
     fsbuf);

#define WR_SECT(dev, sect_nr, fsbuf)     \
 rw_sector(                              \
     DEV_WRITE,                          \
     dev,                                \
     (sect_nr)*SECTOR_SIZE,              \
     SECTOR_SIZE, /* write one sector */ \
     proc2pid(p_proc_current),           \
     fsbuf);

// modified by mingxuan 2020-10-27
#define RD_SECT_FAT(dev, buf, sect_nr)               \
 rw_sector_fat(                                      \
     DEV_READ,                                       \
     dev,                                            \
     (sect_nr)*SECTOR_SIZE,                          \
     SECTOR_SIZE,              /* read one sector */ \
     proc2pid(p_proc_current), /*TASK_A*/            \
     buf);

// modified by mingxuan 2020-10-27
#define WR_SECT_FAT(dev, buf, sect_nr)   \
 rw_sector_fat(                          \
     DEV_WRITE,                          \
     dev,                                \
     (sect_nr)*SECTOR_SIZE,              \
     SECTOR_SIZE, /* write one sector */ \
     proc2pid(p_proc_current),           \
     buf);

// added by xw, 18/8/27
#define RD_SECT_SCHED(dev, sect_nr, fsbuf)           \
 rw_sector_sched(                                    \
     DEV_READ,                                       \
     dev,                                            \
     (sect_nr)*SECTOR_SIZE,                          \
     SECTOR_SIZE,              /* read one sector */ \
     proc2pid(p_proc_current), /*TASK_A*/            \
     fsbuf);

#define WR_SECT_SCHED(dev, sect_nr, fsbuf) \
 rw_sector_sched(                          \
     DEV_WRITE,                            \
     dev,                                  \
     (sect_nr)*SECTOR_SIZE,                \
     SECTOR_SIZE, /* write one sector */   \
     proc2pid(p_proc_current),             \
     fsbuf);

// modified by mingxuan 2020-10-27
#define RD_SECT_SCHED_FAT(dev, buf, sect_nr)         \
 rw_sector_sched_fat(                                \
     DEV_READ,                                       \
     dev,                                            \
     (sect_nr)*SECTOR_SIZE,                          \
     SECTOR_SIZE,              /* read one sector */ \
     proc2pid(p_proc_current), /*TASK_A*/            \
     buf);

// modified by mingxuan 2020-10-27
#define WR_SECT_SCHED_FAT(dev, buf, sect_nr) \
 rw_sector_sched_fat(                        \
     DEV_WRITE,                              \
     dev,                                    \
     (sect_nr)*SECTOR_SIZE,                  \
     SECTOR_SIZE, /* write one sector */     \
     proc2pid(p_proc_current),               \
     buf);
