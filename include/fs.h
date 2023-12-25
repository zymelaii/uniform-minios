/**
 * fs.h
 * This file contains APIs of filesystem, it's used inside the kernel.
 * There is a seperate header file for user program's use. 
 * This file is added by xw. 18/6/17
 */

#ifndef	FS_H
#define	FS_H

/* APIs of file operation */
#define	O_CREAT		1
#define	O_RDWR		2

#define SEEK_SET	1
#define SEEK_CUR	2
#define SEEK_END	3

#define	MAX_PATH	128
#define	MAX_FILENAME_LEN	12

void init_fs();

//added by mingxuan 2019-5-17
int real_open(const char *pathname, int flags);
int real_close(int fd);
int real_read(int fd, void *buf, int count);
int real_write(int fd, const void *buf, int count);
int real_unlink(const char *pathname);
int real_lseek(int fd, int offset, int whence);

//added by mingxuan 2020-10-30
void read_super_block(int dev);
struct super_block* get_super_block(int dev);
int get_fs_dev(int drive, int fs_type);

#endif /* FS_H */
