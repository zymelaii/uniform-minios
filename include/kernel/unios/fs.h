#pragma once

/* APIs of file operation */
#define O_CREAT 1
#define O_RDWR  2

#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3

void init_fs();

int real_open(const char *pathname, int flags);
int real_close(int fd);
int real_read(int fd, void *buf, int count);
int real_write(int fd, const void *buf, int count);
int real_unlink(const char *pathname);
int real_lseek(int fd, int offset, int whence);

void                read_orange_superblock(int dev);
struct super_block *get_unique_superblock(int dev);
int                 get_fs_dev(int drive, int fs_type);
