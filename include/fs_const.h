/**
 * fs_const.h
 * This file contains consts and macros associated with filesystem.
 * The code is added by zcr, and the file is added by xw. 18/6/17
 */

/* TTY */
#define NR_CONSOLES	3	/* consoles */

/* max() & min() */
#define	max(a,b)	((a) > (b) ? (a) : (b))
#define	min(a,b)	((a) < (b) ? (a) : (b))

/* macros for messages */
#define	FD			u.m3.m3i1 
#define	PATHNAME	u.m3.m3p1 
#define	FLAGS		u.m3.m3i1 
#define	NAME_LEN	u.m3.m3i2 
#define	CNT			u.m3.m3i2
#define	REQUEST		u.m3.m3i2
#define	PROC_NR		u.m3.m3i3
#define	DEVICE		u.m3.m3i4
#define	POSITION	u.m3.m3l1
#define	BUF			u.m3.m3p2
#define	OFFSET		u.m3.m3i2 
#define	WHENCE		u.m3.m3i3 

#define	RETVAL		u.m3.m3i1


#define	DIOCTL_GET_GEO	1

/* Hard Drive */
#define SECTOR_SIZE		512
#define SECTOR_BITS		(SECTOR_SIZE * 8)
#define SECTOR_SIZE_SHIFT	9

/* major device numbers (corresponding to kernel/global.c::dd_map[]) */
#define	NO_DEV			0
#define	DEV_FLOPPY		1
#define	DEV_CDROM		2
#define	DEV_HD			3
#define	DEV_CHAR_TTY	4
#define	DEV_SCSI		5

/* make device number from major and minor numbers */
#define	MAJOR_SHIFT		8
#define	MAKE_DEV(a,b)		((a << MAJOR_SHIFT) | b)

/* separate major and minor numbers from device number */
#define	MAJOR(x)		((x >> MAJOR_SHIFT) & 0xFF)
#define	MINOR(x)		(x & 0xFF)

#define	INVALID_INODE		0
#define	ROOT_INODE			1


#define	MAX_DRIVES			2
#define	NR_PART_PER_DRIVE	4	// 每块硬盘(驱动器)只能有4个主分区, mingxuan
#define	NR_SUB_PER_PART		16	// 每个扩展分区最多有16个逻辑分区, mingxuan
#define	NR_SUB_PER_DRIVE	(NR_SUB_PER_PART * NR_PART_PER_DRIVE) //每块硬盘(驱动器)最多有16 * 4 = 64个逻辑分区, mingxuan
#define	NR_PRIM_PER_DRIVE	(NR_PART_PER_DRIVE + 1) // 表示的是hd[0～4]这5个分区，因为有些代码中我们把整块硬盘（hd0）和主分区（hd[1～4]）放在一起看待, mingxuan

/**
 * @def MAX_PRIM
 * Defines the max minor number of the primary partitions.
 * If there are 2 disks, prim_dev ranges in hd[0-9], this macro will
 * equals 9.
 */
// MAX_PRIM定义的是主分区的最大值，比如若有两块硬盘，那第一块硬盘的主分区为hd[1～4]，第二块硬盘的主分区为hd[6～9]
// 所以MAX_PRIM为9，我们定义的hd1a的设备号应大于它，这样通过与MAX_PRIM比较，我们就可以知道一个设备是主分区还是逻辑分区
// mingxuan
#define	MAX_PRIM			(MAX_DRIVES * NR_PRIM_PER_DRIVE - 1)	// MAX_PRIM = 2 * 4 - 1 = 9
#define	MAX_SUBPARTITIONS	(NR_SUB_PER_DRIVE * MAX_DRIVES) 		// MAX_SUBPARTITIONS = 64 * 2 = 128

/*	name of drives	*/
//	added by mingxuan 2020-10-27
#define PRIMARY_MASTER		0x0
#define PRIMARY_SLAVE		0x1
#define SECONDARY_MASTER	0x2
#define SECONDARY_SLAVE		0x3

/* device numbers of hard disk */
#define MINOR_hd0		0x0	// added by mingxuan 2020-10-9
#define MINOR_hd1		0x1	// added by mingxuan 2020-10-12
#define MINOR_hd2		0x2	// added by mingxuan 2020-10-12

#define	MINOR_hd1a		0x10
#define	MINOR_hd2a		(MINOR_hd1a + NR_SUB_PER_PART) // MINOR_hd2a = 16 + 16

#define MAJOR_FAT		0x4		//modified by mingxuan 2020-10-22	

#define	MINOR_BOOT		MINOR_hd2		//modified by mingxuan 2020-10-12

#define	P_PRIMARY	0
#define	P_EXTENDED	1

#define ORANGES_PART	0x99	/* Orange'S partition */
#define NO_PART		0x00	/* unused entry */
#define EXT_PART	0x05	/* extended partition */

#define	NR_FILE_DESC	128	/* FIXME */	//modified by mingxuan 2019-5-19
#define	NR_INODE	64	/* FIXME */
#define	NR_SUPER_BLOCK	8


/* INODE::i_mode (octal, lower 32 bits reserved) */
#define I_TYPE_MASK     0170000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY     0040000
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE	0010000

#define	is_special(m)	((((m) & I_TYPE_MASK) == I_BLOCK_SPECIAL) ||	\
			 (((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))

#define	NR_DEFAULT_FILE_SECTS	2048 /* 2048 * 512 = 1MB */

#define FSBUF_SIZE	0x100000		//added by mingxuan 2019-5-17