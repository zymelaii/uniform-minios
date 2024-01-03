#pragma once

#include "fs_misc.h"
#include "fs_const.h"

#define NR_FS    10 //<! 最大 fs 数
#define NR_FS_OP 3  //<! 最大 fs 操作表数
#define NR_SB_OP 2  //<! 最大 sb 操作表数

#ifndef NR_FILE_DESC
#define NR_FILE_DESC 128 //<! 最大同时操作的 fd 数
#endif
#ifndef NR_SUPER_BLOCK
#define NR_SUPER_BLOCK 8 //<! 最大 superblock 数
#endif

typedef struct file_op_set {
    int (*create)(const char *);
    int (*open)(const char *, int);
    int (*close)(int);
    int (*read)(int, void *, int);
    int (*write)(int, const void *, int);
    int (*lseek)(int, int, int);
    int (*unlink)(const char *);
    int (*delete)(const char *);
    int (*opendir)(const char *);
    int (*createdir)(const char *);
    int (*deletedir)(const char *);
} file_op_set_t;

typedef struct superblock_op_set {
    void (*read)(int);
    superblock_t *(*get)(int);
} superblock_op_set_t;

typedef struct vfs {
    const char          *name;   //<! 设备名
    int                  nr_dev; //<! 设备号
    file_op_set_t       *ops;    //<! 操作表
    superblock_t        *sb;     //<! 设备 superblock
    superblock_op_set_t *sb_ops; //<! superblock 操作表
} vfs_t;

void vfs_setup_and_init();
