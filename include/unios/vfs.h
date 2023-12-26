#pragma once

#include <fs_misc.h>
#include <fs_const.h>

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

int do_vopen(const char *path, int flags);
int do_vclose(int fd);
int do_vread(int fd, char *buf, int count);
int do_vwrite(int fd, const char *buf, int count);
int do_vunlink(const char *path);
int do_vlseek(int fd, int offset, int whence);
int do_vcreate(const char *path);
int do_vdelete(const char *path);
int do_vopendir(const char *path);
int do_vcreatedir(const char *path);
int do_vdeletedir(const char *path);

int sys_open(void *uesp);
int sys_close(void *uesp);
int sys_read(void *uesp);
int sys_write(void *uesp);
int sys_lseek(void *uesp);
int sys_unlink(void *uesp);
int sys_create(void *uesp);
int sys_delete(void *uesp);
int sys_opendir(void *uesp);
int sys_createdir(void *uesp);
int sys_deletedir(void *uesp);
