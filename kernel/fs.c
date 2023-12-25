/// zcr copy from chapter9/d fs/main.c and modified it.

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "fs_misc.h"
#include "stdio.h"
#include "assert.h"

//added by xw, 18/8/28
/* data */
static struct inode* root_inode;

//static struct file_desc f_desc_table[NR_FILE_DESC];	//deleted by mingxuan 2020-10-30
extern struct file_desc f_desc_table[NR_FILE_DESC];	//modified by mingxuan 2020-10-30

static struct inode inode_table[NR_INODE];	

//static struct super_block super_block[NR_SUPER_BLOCK];	//deleted by mingxuan 2020-10-30
extern struct super_block super_block[NR_SUPER_BLOCK];	//modified by mingxuan 2020-10-30

/* functions */
static void mkfs();
//static void read_super_block(int dev);
 void read_super_block(int dev);	// modified by mingxuan 2020-10-30
//static struct super_block* get_super_block(int dev);
 struct super_block* get_super_block(int dev);	// modified by mingxuan 2020-10-30

static int do_open(MESSAGE *fs_msg);
static int do_close(int fd);
static int do_lseek(MESSAGE *fs_msg);
static int do_rdwt(MESSAGE *fs_msg);
static int do_unlink(MESSAGE *fs_msg);

int real_open(const char *pathname, int flags);	 //modified by mingxuan 2019-5-17
int real_close(int fd);	//modified by mingxuan 2019-5-17
int real_read(int fd, void *buf, int count); //注意:buf的类型被修改成char //modified by mingxuan 2019-5-17
int real_write(int fd, const void *buf, int count); //注意:buf的类型被修改成char //modified by mingxuan 2019-5-17
int real_unlink(const char *pathname);	//modified by mingxuan 2019-5-17
int real_lseek(int fd, int offset, int whence);	  //modified by mingxuan 2019-5-17

static int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf);
static int rw_sector_sched(int io_type, int dev, int pos, int bytes, int proc_nr, void* buf);

static int strip_path(char * filename, const char * pathname, struct inode** ppinode);
static int search_file(char *path);
static struct inode* create_file(char *path, int flags);
static struct inode* get_inode(int dev, int num);
static struct inode* get_inode_sched(int dev, int num);
static struct inode* new_inode(int dev, int inode_nr, int start_sect);
static void put_inode(struct inode *pinode);
static void sync_inode(struct inode *p);
static void new_dir_entry(struct inode *dir_inode,int inode_nr,char *filename);
static int alloc_imap_bit(int dev);
static int alloc_smap_bit(int dev, int nr_sects_to_alloc);

static int memcmp(const void *s1, const void *s2, int n);
//~xw
int get_fs_dev(int drive, int fs_type);	// added by mingxuan 2020-10-27


// added by mingxuan 2020-10-27
int get_fs_dev(int drive, int fs_type)
{

	int i=0;
	for(i=0; i < NR_PRIM_PER_DRIVE; i++)
	{
		if(hd_info[drive].primary[i].fs_type == fs_type)
		return ((DEV_HD << MAJOR_SHIFT) | i);
	}

	//added by mingxuan 2020-10-29
	for(i=0; i < NR_SUB_PER_DRIVE; i++)
	{
		if(hd_info[drive].logical[i].fs_type == fs_type)
		return ((DEV_HD << MAJOR_SHIFT) | (i + MINOR_hd1a)); // logic的下标i加上hd1a才是该逻辑分区的次设备号
	}
	return 0;
}

/// zcr added
void init_fs() 
{

	kprintf("Initializing file system...  ");
	
	int i;
	for (i = 0; i < NR_INODE; i++)
		memset(&inode_table[i], 0, sizeof(struct inode));
	struct super_block * sb = super_block;						//deleted by mingxuan 2020-10-30
	
	int orange_dev = get_fs_dev(PRIMARY_MASTER, ORANGE_TYPE);	//added by mingxuan 2020-10-27

	/* load super block of ROOT */
	read_super_block(orange_dev);		// modified by mingxuan 2020-10-27
	sb = get_super_block(orange_dev); 	// modified by mingxuan 2020-10-27

	kprintf("Superblock Address:0x%x \n", sb);

	if(sb->magic != MAGIC_V1) {	//deleted by mingxuan 2019-5-20
		mkfs();
		kprintf("Make file system Done.\n");
		read_super_block(orange_dev);	// modified by mingxuan 2020-10-27
	}

	root_inode = get_inode(orange_dev, ROOT_INODE);	// modified by mingxuan 2020-10-27
}

/*****************************************************************************
 *                                mkfs
 *****************************************************************************/
/**
 * <Ring 1> Make a available Orange'S FS in the disk. It will
 *          - Write a super block to sector 1.
 *          - Create three special files: dev_tty0, dev_tty1, dev_tty2
 *          - Create the inode map
 *          - Create the sector map
 *          - Create the inodes of the files
 *          - Create `/', the root directory
 *****************************************************************************/
static void mkfs()
{
	MESSAGE driver_msg;
	int i, j;

	int bits_per_sect = SECTOR_SIZE * 8; /* 8 bits per byte */

	//local array, to substitute global fsbuf. added by xw, 18/12/27
	char fsbuf[SECTOR_SIZE];

	int orange_dev = get_fs_dev(PRIMARY_MASTER, ORANGE_TYPE);	//added by mingxuan 2020-10-27

	/* get the geometry of ROOTDEV */
	struct part_info geo;
	driver_msg.type		= DEV_IOCTL;
	//driver_msg.DEVICE	= MINOR(ROOT_DEV);		// deleted by mingxuan 2020-10-27
	driver_msg.DEVICE	= MINOR(orange_dev);	// modified by mingxuan 2020-10-27
	driver_msg.REQUEST	= DIOCTL_GET_GEO;
	driver_msg.BUF		= &geo;
	driver_msg.PROC_NR	= proc2pid(p_proc_current);
	// assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	// send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);
	hd_ioctl(&driver_msg);

	kprintf("dev size: 0x%x sectors\n", geo.size);

	/************************/
	/*      super block     */
	/************************/
	struct super_block sb;
	sb.magic	  	  = MAGIC_V1;
	sb.nr_inodes	  = bits_per_sect;
	sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE;
	sb.nr_sects	      = geo.size; /* partition size in sector */
	sb.nr_imap_sects  = 1;
	sb.nr_smap_sects  = sb.nr_sects / bits_per_sect + 1;
	sb.n_1st_sect	  = 1 + 1 +   /* boot sector & super block */
							sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects;
	sb.root_inode	  = ROOT_INODE;
	sb.inode_size	  = INODE_SIZE;
	
	struct inode x;
	sb.inode_isize_off= (int)&x.i_size - (int)&x;
	sb.inode_start_off= (int)&x.i_start_sect - (int)&x;
	sb.dir_ent_size	  = DIR_ENTRY_SIZE;
	
	struct dir_entry de;
	sb.dir_ent_inode_off = (int)&de.inode_nr - (int)&de;
	sb.dir_ent_fname_off = (int)&de.name - (int)&de;

	sb.sb_dev = orange_dev;		//added by mingxuan 2020-10-30
	sb.fs_type = ORANGE_TYPE;	//added by mingxuan 2020-10-30

	memset(fsbuf, 0x90, SECTOR_SIZE);
	memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);

	/* write the super block */
	WR_SECT(orange_dev, 1, fsbuf);	// modified by mingxuan 2020-10-27

	kprintf("devbase:0x%x00", (geo.base + 0) * 2);
	kprintf(" sb:0x%x00", (geo.base + 1) * 2);
	kprintf(" imap:0x%x00", (geo.base + 2) * 2);
	kprintf(" smap:0x%x00\n", (geo.base + 2 + sb.nr_imap_sects) * 2);
	kprintf("        inodes:0x%x00", (geo.base + 2 + sb.nr_imap_sects + sb.nr_smap_sects) * 2);
	kprintf(" 1st_sector:0x%x00\n", (geo.base + sb.n_1st_sect) * 2);

	/************************/
	/*       inode map      */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 0; i < (NR_CONSOLES + 3); i++)	  //modified by mingxuan 2019-5-22
		fsbuf[0] |= 1 << i;

	
	WR_SECT(orange_dev, 2, fsbuf);	//modified by mingxuan 2020-10-27

	/************************/
	/*      secter map      */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = NR_DEFAULT_FILE_SECTS + 1;
	for (i = 0; i < nr_sects / 8; i++)
		fsbuf[i] = 0xFF;

	for (j = 0; j < nr_sects % 8; j++)
		fsbuf[i] |= (1 << j);

	WR_SECT(orange_dev, 2 + sb.nr_imap_sects, fsbuf);	//modified by mingxuan 2020-10-27

	/* zeromemory the rest sector-map */
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 1; i < sb.nr_smap_sects; i++)
		WR_SECT(orange_dev, 2 + sb.nr_imap_sects + i, fsbuf);	//modified by mingxuan 2020-10-27

	/* app.tar */
	//added by mingxuan 2019-5-19
	int bit_offset = INSTALL_START_SECTOR -
		sb.n_1st_sect + 1; /* sect M <-> bit (M - sb.n_1stsect + 1) */
	int bit_off_in_sect = bit_offset % (SECTOR_SIZE * 8);
	int bit_left = INSTALL_NR_SECTORS;
	int cur_sect = bit_offset / (SECTOR_SIZE * 8);
	RD_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf);//modified by mingxuan 2020-10-27
	while (bit_left) {
		int byte_off = bit_off_in_sect / 8;
		/* this line is ineffecient in a loop, but I don't care */
		fsbuf[byte_off] |= 1 << (bit_off_in_sect % 8);
		bit_left--;
		bit_off_in_sect++;
		if (bit_off_in_sect == (SECTOR_SIZE * 8)) {
			WR_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf);	//modified by mingxuan 2020-10-27
			cur_sect++;
			RD_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf);	//modified by mingxuan 2020-10-27
			bit_off_in_sect = 0;
		}
	}
	WR_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf);	//modified by mingxuan 2020-10-27

	/************************/
	/*       inodes         */
	/************************/
	/* inode of `/' */
	memset(fsbuf, 0, SECTOR_SIZE);
	struct inode * pi = (struct inode*)fsbuf;
	pi->i_mode = I_DIRECTORY;

	//modified by mingxuan 2019-5-21
	pi->i_size = DIR_ENTRY_SIZE * 5; /* 5 files: (预定义5个文件)
					  * `.',
					  * `dev_tty0', `dev_tty1', `dev_tty2','app.tar'
					  */

	pi->i_start_sect = sb.n_1st_sect;
	pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;
	
	/* inode of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) {
		pi = (struct inode*)(fsbuf + (INODE_SIZE * (i + 1)));
		pi->i_mode = I_CHAR_SPECIAL;
		pi->i_size = 0;
		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
		pi->i_nr_sects = 0;
	}

	/* inode of /app.tar */
	//added by mingxuan 2019-5-19
	pi = (struct inode*)(fsbuf + (INODE_SIZE * (NR_CONSOLES + 1)));
	pi->i_mode = I_REGULAR;
	pi->i_size = INSTALL_NR_SECTORS * SECTOR_SIZE;
	pi->i_start_sect = INSTALL_START_SECTOR;
	pi->i_nr_sects = INSTALL_NR_SECTORS;

	WR_SECT(orange_dev, 2 + sb.nr_imap_sects + sb.nr_smap_sects, fsbuf);	//modified by mingxuan 2020-10-27

	/************************/
	/*          `/'         */
	/************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry * pde = (struct dir_entry *)fsbuf;

	pde->inode_nr = 1;
	strcpy(pde->name, ".");

	/* dir entries of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) {
		pde++;
		pde->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
		switch(i) {
			case 0:	
				strcpy(pde->name, "dev_tty0"); 
				break;
			case 1:
				strcpy(pde->name, "dev_tty1"); 
				break;
			case 2:
				strcpy(pde->name, "dev_tty2"); 
				break;
		}
	}

	(++pde)->inode_nr = NR_CONSOLES + 2; //added by mingxuan 2019-5-19
	strcpy(pde->name, INSTALL_FILENAME); //added by mingxuan 2019-5-19

	//WR_SECT(ROOT_DEV, sb.n_1st_sect, fsbuf);	//modified by xw, 18/12/27	//deleted by mingxuan 2020-10-27
	WR_SECT(orange_dev, sb.n_1st_sect, fsbuf);	//modified by mingxuan 2020-10-27
}

/*****************************************************************************
 *                                rw_sector
 *****************************************************************************/
/**
 * <Ring 1> R/W a sector via messaging with the corresponding driver.
 * 
 * @param io_type  DEV_READ or DEV_WRITE
 * @param dev      device nr
 * @param pos      Byte offset from/to where to r/w.
 * @param bytes    r/w count in bytes.
 * @param proc_nr  To whom the buffer belongs.
 * @param buf      r/w buffer.
 * 
 * @return Zero if success.
 *****************************************************************************/
/// zcr: change the "u64 pos" to "int pos"
static int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
{
	MESSAGE driver_msg;
	
	driver_msg.type		= io_type;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= pos;
	driver_msg.CNT		= bytes;	/// hu is: 512
	driver_msg.PROC_NR	= proc_nr;
	driver_msg.BUF		= buf;

	hd_rdwt(&driver_msg);
	return 0;
}

//added by xw, 18/8/27
static int rw_sector_sched(int io_type, int dev, int pos, int bytes, int proc_nr, void* buf)
{
	MESSAGE driver_msg;
	
	driver_msg.type		= io_type;
	driver_msg.DEVICE	= MINOR(dev);

	driver_msg.POSITION	= pos;
	driver_msg.CNT		= bytes;	/// hu is: 512
	driver_msg.PROC_NR	= proc_nr;
	driver_msg.BUF		= buf;
	
	hd_rdwt_sched(&driver_msg);
	return 0;
}
//~xw

// added by zcr from chapter9/e/lib/open.c and modified it.

/*****************************************************************************
 *                                open
 *****************************************************************************/
/**
 * open/create a file.
 * 
 * @param pathname  The full path of the file to be opened/created.
 * @param flags     O_CREAT, O_RDWR, etc.
 * 
 * @return File descriptor if successful, otherwise -1.
 *****************************************************************************/
//open is a syscall interface now. added by xw, 18/6/18
//  int open(const char *pathname, int flags)
// static int real_open(const char *pathname, int flags)	//deleted by mingxuan 2019-5-17
int real_open(const char *pathname, int flags)	//modified by mingxuan 2019-5-17
{
	//added by xw, 18/8/27
	MESSAGE fs_msg;

	fs_msg.type	= OPEN;
	fs_msg.PATHNAME	= (void*)pathname;
	fs_msg.FLAGS	= flags;
	fs_msg.NAME_LEN	= strlen(pathname);
	fs_msg.source = proc2pid(p_proc_current);

	int fd = do_open(&fs_msg);

	return fd;
}

/*****************************************************************************
 *                                do_open
 *****************************************************************************/
/**
 * Open a file and return the file descriptor.
 * 
 * @return File descriptor if successful, otherwise a negative error code.
 *****************************************************************************/
/// zcr modified.
static int do_open(MESSAGE *fs_msg)
{
	/*caller_nr is the process number of the */
	int fd = -1;		/* return value */

	char pathname[MAX_PATH];

	/* get parameters from the message */
	int flags = fs_msg->FLAGS;	/* access mode */
	int name_len = fs_msg->NAME_LEN;	/* length of filename */
	int src = fs_msg->source;	/* caller proc nr. */

	memcpy((void*)va2la(src, pathname), (void*)va2la(src, fs_msg->PATHNAME), name_len);
	pathname[name_len] = 0;

	/* find a free slot in PROCESS::filp[] */
	int i;
	for (i = 0; i < NR_FILES; i++) { //modified by mingxuan 2019-5-20
		if (p_proc_current->task.filp[i] == 0) {
			fd = i;
			break;
		}
	}

	assert(0 <= fd && fd < NR_FILES);

	/* find a free slot in f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++)
		//modified by mingxuan 2019-5-17
		if (f_desc_table[i].flag == 0)
			break;
	
	assert(i < NR_FILE_DESC);

	int inode_nr = search_file(pathname);
	struct inode * pin = 0;
	if (flags & O_CREAT) {
		if (inode_nr) {
			return -1;
		}
		else {
			pin = create_file(pathname, flags);
		}
	}
	else {
		char filename[MAX_PATH];
		struct inode * dir_inode;
		if (strip_path(filename, pathname, &dir_inode) != 0)
			return -1;
		pin = get_inode(dir_inode->i_dev, inode_nr); //modified by mingxuan 2019-5-20
	}

	if (pin) {
		/* connects proc with file_descriptor */
		p_proc_current->task.filp[fd] = &f_desc_table[i];
		
		f_desc_table[i].flag = 1;	//added by mingxuan 2019-5-17

		/* connects file_descriptor with inode */
		f_desc_table[i].fd_node.fd_inode = pin;	//modified by mingxuan 2019-5-17

		f_desc_table[i].fd_mode = flags;
		f_desc_table[i].fd_pos = 0;

		int imode = pin->i_mode & I_TYPE_MASK;

		if (imode == I_CHAR_SPECIAL) {
			// MESSAGE driver_msg;

			// int dev = pin->i_start_sect;
			//?
		}
		else if (imode == I_DIRECTORY) {
			if(pin->i_num != ROOT_INODE) {
				panic("pin->i_num != ROOT_INODE");
			}
		}
		else {
			if(pin->i_mode != I_REGULAR) {
				panic("Panic: pin->i_mode != I_REGULAR");
			}
		}
	}
	else {
		return -1;
	}

	return fd;
}

/*****************************************************************************
 *                                create_file
 *****************************************************************************/
/**
 * Create a file and return it's inode ptr.
 *
 * @param[in] path   The full path of the new file
 * @param[in] flags  Attribiutes of the new file
 *
 * @return           Ptr to i-node of the new file if successful, otherwise 0.
 * 
 * @see open()
 * @see do_open()
 *****************************************************************************/
static struct inode * create_file(char * path, int flags)
{

	char filename[MAX_PATH];
	struct inode * dir_inode;

	if (strip_path(filename, path, &dir_inode) != 0)
		return 0;

	int inode_nr = alloc_imap_bit(dir_inode->i_dev);

	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev, NR_DEFAULT_FILE_SECTS);

	struct inode *newino = new_inode(dir_inode->i_dev, inode_nr, free_sect_nr);

	new_dir_entry(dir_inode, newino->i_num, filename);

	return newino;
}

/// zcr copied from ch9/e/fs/misc.c

/*****************************************************************************
 *                                memcmp
 *****************************************************************************/
/**
 * Compare memory areas.
 * 
 * @param s1  The 1st area.
 * @param s2  The 2nd area.
 * @param n   The first n bytes will be compared.
 * 
 * @return  an integer less than, equal to, or greater than zero if the first
 *          n bytes of s1 is found, respectively, to be less than, to match,
 *          or  be greater than the first n bytes of s2.
 *****************************************************************************/
static int memcmp(const void * s1, const void *s2, int n)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = (const char *)s1;
	const char * p2 = (const char *)s2;
	int i;
	for (i = 0; i < n; i++,p1++,p2++) {
		if (*p1 != *p2) {
			return (*p1 - *p2);
		}
	}
	return 0;
}


/*****************************************************************************
 *                                search_file
 *****************************************************************************/
/**
 * Search the file and return the inode_nr.
 *
 * @param[in] path The full path of the file to search.
 * @return         Ptr to the i-node of the file if successful, otherwise zero.
 * 
 * @see open()
 * @see do_open()
 *****************************************************************************/
static int search_file(char * path)
{
	int i, j;

	char filename[MAX_PATH];
	memset(filename, 0, MAX_FILENAME_LEN);
	struct inode * dir_inode;
	if (strip_path(filename, path, &dir_inode) != 0)
		return 0;

	if (filename[0] == 0)	/* path: "/" */
		return dir_inode->i_num;

	/**
	 * Search the dir for the file.
	 */
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nr_dir_entries =
	  dir_inode->i_size / DIR_ENTRY_SIZE; /**
					       * including unused slots
					       * (the file has been deleted
					       * but the slot is still there)
					       */
	int m = 0;
	struct dir_entry * pde;
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27
	for (i = 0; i < nr_dir_blks; i++) {
		//RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by mingxuan 2019-5-20
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
				return pde->inode_nr;
			if (++m > nr_dir_entries)
				break;
		}
		if (m > nr_dir_entries) /* all entries have been iterated */
			break;
	}

	/* file not found */
	return 0;
}

/*****************************************************************************
 *                                strip_path
 *****************************************************************************/
/**
 * Get the basename from the fullpath.
 *
 * In Orange'S FS v1.0, all files are stored in the root directory.
 * There is no sub-folder thing.
 *
 * This routine should be called at the very beginning of file operations
 * such as open(), read() and write(). It accepts the full path and returns
 * two things: the basename and a ptr of the root dir's i-node.
 *
 * e.g. After stip_path(filename, "/blah", ppinode) finishes, we get:
 *      - filename: "blah"
 *      - *ppinode: root_inode
 *      - ret val:  0 (successful)
 *
 * Currently an acceptable pathname should begin with at most one `/'
 * preceding a filename.
 *
 * Filenames may contain any character except '/' and '\\0'.
 *
 * @param[out] filename The string for the result.
 * @param[in]  pathname The full pathname.
 * @param[out] ppinode  The ptr of the dir's inode will be stored here.
 * 
 * @return Zero if success, otherwise the pathname is not valid.
 *****************************************************************************/
static int strip_path(char * filename, const char * pathname, struct inode** ppinode)
{
	const char * s = pathname;
	char * t = filename;

	if (s == 0)
		return -1;

	if (*s == '/')
		s++;

	while (*s) {		/* check each character */
		if (*s == '/')
			return -1;
		*t++ = *s++;
		/* if filename is too long, just truncate it */
		if (t - filename >= MAX_FILENAME_LEN)
			break;
	}
	*t = 0;

	*ppinode = root_inode;

	return 0;
}

/*****************************************************************************
 *                                read_super_block
 *****************************************************************************/
/**
 * <Ring 1> Read super block from the given device then write it into a free
 *          super_block[] slot.
 * 
 * @param dev  From which device the super block comes.
 *****************************************************************************/
//static void read_super_block(int dev)
void read_super_block(int dev)	//modified by mingxuan 2020-10-30
{
	int i;
	MESSAGE driver_msg;
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27

	driver_msg.type		= DEV_READ;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= SECTOR_SIZE * 1;
	driver_msg.BUF		= fsbuf;
	driver_msg.CNT		= SECTOR_SIZE;
	driver_msg.PROC_NR	= proc2pid(p_proc_current);///TASK_A

	hd_rdwt(&driver_msg);

	for (i = 0; i < NR_SUPER_BLOCK; i++)
		if (super_block[i].fs_type == ORANGE_TYPE)
			break;
	if (i == NR_SUPER_BLOCK)
		panic("Cannot find orange's superblock!");

	struct super_block * psb = (struct super_block *)fsbuf;

	super_block[i] = *psb;

	super_block[i].sb_dev = dev;
	super_block[i].fs_type = ORANGE_TYPE;	//added by mingxuan 2020-10-30
}


/*****************************************************************************
 *                                get_super_block
 *****************************************************************************/
/**
 * <Ring 1> Get the super block from super_block[].
 * 
 * @param dev Device nr.
 * 
 * @return Super block ptr.
 *****************************************************************************/
struct super_block * get_super_block(int dev)	//modified by mingxuan 2020-10-30
{
	struct super_block * sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++){
		if (sb->sb_dev == dev)
			return sb;
	}
	return 0;
}



/*****************************************************************************
 *                                get_inode
 *****************************************************************************/
/**
 * <Ring 1> Get the inode ptr of given inode nr. A cache -- inode_table[] -- is
 * maintained to make things faster. If the inode requested is already there,
 * just return it. Otherwise the inode will be read from the disk.
 * 
 * @param dev Device nr.
 * @param num I-node nr.
 * 
 * @return The inode ptr requested.
 *****************************************************************************/
static struct inode * get_inode(int dev, int num)
{
	if (num == 0)
		return 0;

	struct inode * p;
	struct inode * q = 0;
	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) {
		if (p->i_cnt) {	/* not a free slot */
			if ((p->i_dev == dev) && (p->i_num == num)) {
				/* this is the inode we want */
				p->i_cnt++;
				return p;
			}
		}
		else {		/* a free slot */
			if (!q) /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		panic("the inode table is full");

	q->i_dev = dev;
	q->i_num = num;
	q->i_cnt = 1;

	struct super_block * sb = get_super_block(dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((num - 1) / (SECTOR_SIZE / INODE_SIZE));
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27
	RD_SECT(dev, blk_nr, fsbuf);	//added by xw, 18/12/27
	struct inode * pinode =
		(struct inode*)((u8*)fsbuf +
				((num - 1 ) % (SECTOR_SIZE / INODE_SIZE))
				 * INODE_SIZE);
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode->i_start_sect;
	q->i_nr_sects = pinode->i_nr_sects;
	return q;
}

//added by xw, 18/8/27
static struct inode * get_inode_sched(int dev, int num)
{
	if (num == 0)
		return 0;

	struct inode * p;
	struct inode * q = 0;
	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) {
		if (p->i_cnt) {	/* not a free slot */
			if ((p->i_dev == dev) && (p->i_num == num)) {
				/* this is the inode we want */
				p->i_cnt++;
				return p;
			}
		}
		else {		/* a free slot */
			if (!q) /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		panic("the inode table is full");

	q->i_dev = dev;
	q->i_num = num;
	q->i_cnt = 1;

	struct super_block * sb = get_super_block(dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((num - 1) / (SECTOR_SIZE / INODE_SIZE));
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27
	RD_SECT_SCHED(dev, blk_nr, fsbuf);	//added by xw, 18/12/27
	struct inode * pinode =
		(struct inode*)((u8*)fsbuf +
				((num - 1 ) % (SECTOR_SIZE / INODE_SIZE))
				 * INODE_SIZE);
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode->i_start_sect;
	q->i_nr_sects = pinode->i_nr_sects;
	return q;
}

/*****************************************************************************
 *                                put_inode
 *****************************************************************************/
/**
 * Decrease the reference nr of a slot in inode_table[]. When the nr reaches
 * zero, it means the inode is not used any more and can be overwritten by
 * a new inode.
 * 
 * @param pinode I-node ptr.
 *****************************************************************************/
static void put_inode(struct inode * pinode)
{
	// assert(pinode->i_cnt > 0);
	pinode->i_cnt--;
}

/*****************************************************************************
 *                                sync_inode
 *****************************************************************************/
/**
 * <Ring 1> Write the inode back to the disk. Commonly invoked as soon as the
 *          inode is changed.
 * 
 * @param p I-node ptr.
 *****************************************************************************/
static void sync_inode(struct inode * p)
{
	struct inode * pinode;
	struct super_block * sb = get_super_block(p->i_dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((p->i_num - 1) / (SECTOR_SIZE / INODE_SIZE));
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27
	RD_SECT(p->i_dev, blk_nr, fsbuf);	//modified by mingxuan 2019-5-20
	pinode = (struct inode*)((u8*)fsbuf +
				 (((p->i_num - 1) % (SECTOR_SIZE / INODE_SIZE))
				  * INODE_SIZE));
	pinode->i_mode = p->i_mode;
	pinode->i_size = p->i_size;
	pinode->i_start_sect = p->i_start_sect;
	pinode->i_nr_sects = p->i_nr_sects;
	WR_SECT(p->i_dev, blk_nr, fsbuf);	//modified by mingxuan 2019-5-20
}

/// added by zcr (from ch9/e/fs/open.c)
/*****************************************************************************
 *                                new_inode
 *****************************************************************************/
/**
 * Generate a new i-node and write it to disk.
 * 
 * @param dev  Home device of the i-node.
 * @param inode_nr  I-node nr.
 * @param start_sect  Start sector of the file pointed by the new i-node.
 * 
 * @return  Ptr of the new i-node.
 *****************************************************************************/
static struct inode * new_inode(int dev, int inode_nr, int start_sect)
{
	//struct inode * new_inode = get_inode_sched(dev, inode_nr);	//modified by xw, 18/8/28
	struct inode * new_inode = get_inode(dev, inode_nr); //modified by mingxuan 2019-5-20

	new_inode->i_mode = I_REGULAR;
	new_inode->i_size = 0;
	new_inode->i_start_sect = start_sect;
	new_inode->i_nr_sects = NR_DEFAULT_FILE_SECTS;

	new_inode->i_dev = dev;
	new_inode->i_cnt = 1;
	new_inode->i_num = inode_nr;

	/* write to the inode array */
	sync_inode(new_inode);

	return new_inode;
}

/*****************************************************************************
 *                                new_dir_entry
 *****************************************************************************/
/**
 * Write a new entry into the directory.
 * 
 * @param dir_inode  I-node of the directory.
 * @param inode_nr   I-node nr of the new file.
 * @param filename   Filename of the new file.
 *****************************************************************************/
static void new_dir_entry(struct inode *dir_inode,int inode_nr,char *filename)
{
	/* write the dir_entry */
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE) / SECTOR_SIZE;
	int nr_dir_entries =
		dir_inode->i_size / DIR_ENTRY_SIZE; /**
						     * including unused slots
						     * (the file has been
						     * deleted but the slot
						     * is still there)
						     */
	int m = 0;
	struct dir_entry * pde;
	struct dir_entry * new_de = 0;

	int i, j;
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27
	for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by mingxuan 2019-5-20

		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			if (++m > nr_dir_entries)
				break;

			if (pde->inode_nr == 0) { /* it's a free slot */
				new_de = pde;
				break;
			}
		}
		if (m > nr_dir_entries ||/* all entries have been iterated or */
		    new_de)              /* free slot is found */
			break;
	}
	if (!new_de) { /* reached the end of the dir */
		new_de = pde;
		dir_inode->i_size += DIR_ENTRY_SIZE;
	}
	new_de->inode_nr = inode_nr;
	strcpy(new_de->name, filename);

	/* write dir block -- ROOT dir block */
	WR_SECT(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//added by mingxuan 2019-5-20

	/* update dir inode */
	sync_inode(dir_inode);
}


/*****************************************************************************
 *                                alloc_imap_bit
 *****************************************************************************/
/**
 * Allocate a bit in inode-map.
 * 
 * @param dev  In which device the inode-map is located.
 * 
 * @return  I-node nr.
 *****************************************************************************/
static int alloc_imap_bit(int dev)
{
	int inode_nr = 0;
	int i, j, k;

	int imap_blk0_nr = 1 + 1; /* 1 boot sector & 1 super block */
	struct super_block * sb = get_super_block(dev);

	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27

	for (i = 0; i < sb->nr_imap_sects; i++) {
		RD_SECT(dev, imap_blk0_nr + i, fsbuf);	//modified by mingxuan 2019-5-20

		for (j = 0; j < SECTOR_SIZE; j++) {
			if (fsbuf[j] == '\xFF')		//modified by xw, 18/12/28
				continue;

			/* skip `1' bits */
			for (k = 0; ((fsbuf[j] >> k) & 1) != 0; k++) {}

			/* i: sector index; j: byte index; k: bit index */
			inode_nr = (i * SECTOR_SIZE + j) * 8 + k;
			fsbuf[j] |= (1 << k);

			/* write the bit to imap */
			WR_SECT(dev, imap_blk0_nr + i, fsbuf);	//added by mingxuan 2019-5-20
			break;
		}

		return inode_nr;
	}

	/* no free bit in imap */
	panic("inode-map is probably full.\n");

	return 0;
}

/*****************************************************************************
 *                                alloc_smap_bit
 *****************************************************************************/
/**
 * Allocate a bit in sector-map.
 * 
 * @param dev  In which device the sector-map is located.
 * @param nr_sects_to_alloc  How many sectors are allocated.
 * 
 * @return  The 1st sector nr allocated.
 *****************************************************************************/
static int alloc_smap_bit(int dev, int nr_sects_to_alloc)
{
	/* int nr_sects_to_alloc = NR_DEFAULT_FILE_SECTS; */

	int i; /* sector index */
	int j; /* byte index */
	int k; /* bit index */

	struct super_block * sb = get_super_block(dev);

	int smap_blk0_nr = 1 + 1 + sb->nr_imap_sects;
	int free_sect_nr = 0;
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27

	for (i = 0; i < sb->nr_smap_sects; i++) { /* smap_blk0_nr + i :
						     current sect nr. */
		//RD_SECT_SCHED(dev, smap_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
		RD_SECT(dev, smap_blk0_nr + i, fsbuf);	//modified by mingxuan 2019-5-20

		/* byte offset in current sect */
		for (j = 0; j < SECTOR_SIZE && nr_sects_to_alloc > 0; j++) {
			k = 0;
			if (!free_sect_nr) {
				/* loop until a free bit is found */
				//if (fsbuf[j] == 0xFF) continue;
				if (fsbuf[j] == '\xFF') continue;	//modified by xw, 18/12/28
				for (; ((fsbuf[j] >> k) & 1) != 0; k++) {}
				free_sect_nr = (i * SECTOR_SIZE + j) * 8 +
					k - 1 + sb->n_1st_sect;
			}

			for (; k < 8; k++) { /* repeat till enough bits are set */
				// assert(((fsbuf[j] >> k) & 1) == 0);
				fsbuf[j] |= (1 << k);
				if (--nr_sects_to_alloc == 0)
					break;
			}
		}

		if (free_sect_nr) /* free bit found, write the bits to smap */
			//WR_SECT_SCHED(dev, smap_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
			WR_SECT(dev, smap_blk0_nr + i, fsbuf); //added by mingxuan 2019-5-20

		if (nr_sects_to_alloc == 0)
			break;
	}

	// assert(nr_sects_to_alloc == 0);

	return free_sect_nr;
}

/// zcr added and modified from ch9/e/fs.open.c and close.c
/*****************************************************************************
 *                                close
 *****************************************************************************/
/**
 * Close a file descriptor.
 * 
 * @param fd  File descriptor.
 * 
 * @return Zero if successful, otherwise -1.
 *****************************************************************************/
//close is a syscall interface now. added by xw, 18/6/18
//  int close(int fd)
//static int real_close(int fd)	//deleted by mingxuan 2019-5-17
int real_close(int fd)	//modified by mingxuan 2019-5-17
{
	return do_close(fd);	// terrible(always returns 0)
}

/*****************************************************************************
 *                                do_close
 *****************************************************************************/
/**
 * Handle the message CLOSE.
 * 
 * @return Zero if success.
 *****************************************************************************/
static int do_close(int fd)
{
	put_inode(p_proc_current->task.filp[fd]->fd_node.fd_inode); //modified by mingxuan 2019-5-17
	p_proc_current->task.filp[fd]->fd_node.fd_inode = 0; //modified by mingxuan 2019-5-17
	p_proc_current->task.filp[fd]->flag = 0; //added by mingxuan 2019-5-17
	p_proc_current->task.filp[fd] = 0;

	return 0;
}

/// zcr copied from ch9/f/fs/read_write.c and modified it.

/*****************************************************************************
 *                                read
 *****************************************************************************/
/**
 * Read from a file descriptor.
 * 
 * @param fd     File descriptor.
 * @param buf    Buffer to accept the bytes read.
 * @param count  How many bytes to read.
 * 
 * @return  On success, the number of bytes read are returned.
 *          On error, -1 is returned.
 *****************************************************************************/
int real_read(int fd, void *buf, int count)	//注意:buf的类型被修改成char //modified by mingxuan 2019-5-17
{
	//added by xw, 18/8/27
	MESSAGE fs_msg;
	
	fs_msg.type = READ;
	fs_msg.FD   = fd;
	fs_msg.BUF  = buf;
	fs_msg.CNT  = count;
	fs_msg.source = proc2pid(p_proc_current);	//added by xw, 18/12/22

	// send_recv(BOTH, TASK_FS, &msg);
	do_rdwt(&fs_msg);

	return fs_msg.CNT;
}

/*****************************************************************************
 *                                write
 *****************************************************************************/
/**
 * Write to a file descriptor.
 * 
 * @param fd     File descriptor.
 * @param buf    Buffer including the bytes to write.
 * @param count  How many bytes to write.
 * 
 * @return  On success, the number of bytes written are returned.
 *          On error, -1 is returned.
 *****************************************************************************/
int real_write(int fd, const void *buf, int count)	//注意:buf的类型被修改成char //modified by mingxuan 2019-5-17
{
	//added by xw, 18/8/27
	MESSAGE fs_msg;
	
	fs_msg.type = WRITE;
	fs_msg.FD   = fd;
	fs_msg.BUF  = (void*)buf;
	fs_msg.CNT  = count;
	fs_msg.source = proc2pid(p_proc_current);	//added by xw, 18/12/22

	// send_recv(BOTH, TASK_FS, &msg);
	/// zcr added
	do_rdwt(&fs_msg);
	
	return fs_msg.CNT;
}


/*****************************************************************************
 *                                do_rdwt
 *****************************************************************************/
/**
 * Read/Write file and return byte count read/written.
 *
 * Sector map is not needed to update, since the sectors for the file have been
 * allocated and the bits are set when the file was created.
 * 
 * @return How many bytes have been read/written.
 *****************************************************************************/
static int do_rdwt(MESSAGE *fs_msg)
{
	int fd = fs_msg->FD;	/**< file descriptor. */
	void * buf = fs_msg->BUF;/**< r/w buffer */
	int len = fs_msg->CNT;	/**< r/w bytes */

	int src = fs_msg->source;		/* caller proc nr. */

	if (!(p_proc_current->task.filp[fd]->fd_mode & O_RDWR))
		return -1;

	int pos = p_proc_current->task.filp[fd]->fd_pos;

	//struct inode * pin = p_proc_current->task.filp[fd]->fd_inode;	//deleted by mingxuan 2019-5-17
	struct inode * pin = p_proc_current->task.filp[fd]->fd_node.fd_inode;	//modified by mingxuan 2019-5-17

	int imode = pin->i_mode & I_TYPE_MASK;

	if (imode == I_CHAR_SPECIAL) {
		int t = fs_msg->type == READ ? DEV_READ : DEV_WRITE;
		fs_msg->type = t;

		//added by mingxuan 2019-5-19
		int dev = pin->i_start_sect;
		int nr_tty = MINOR(dev);
		if(MAJOR(dev) != 4) {
			panic("Error: MAJOR(dev) == 4\n");
		}

		if(fs_msg->type == DEV_READ){
			fs_msg->CNT =tty_read(&tty_table[nr_tty],buf,len);
		}else if(fs_msg->type==DEV_WRITE){
			tty_write(&tty_table[nr_tty],buf,len);
		}

		return fs_msg->CNT;
	}
	else {
		int pos_end;
		if (fs_msg->type == READ)
			pos_end = min(pos + len, pin->i_size);
		else		/* WRITE */
			pos_end = min(pos + len, pin->i_nr_sects * SECTOR_SIZE);

		int off = pos % SECTOR_SIZE;
		int rw_sect_min = pin->i_start_sect + (pos>>SECTOR_SIZE_SHIFT);
		int rw_sect_max = pin->i_start_sect + (pos_end>>SECTOR_SIZE_SHIFT);

		//modified by xw, 18/12/27
		int chunk = min(rw_sect_max - rw_sect_min + 1,
				SECTOR_SIZE >> SECTOR_SIZE_SHIFT);
		
		int bytes_rw = 0;
		int bytes_left = len;
		int i;
		
		char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27

		for (i = rw_sect_min; i <= rw_sect_max; i += chunk) {
			/* read/write this amount of bytes every time */
			int bytes = min(bytes_left, chunk * SECTOR_SIZE - off);
			rw_sector(DEV_READ,	//modified by mingxuan 2019-5-21
				  pin->i_dev,
				  i * SECTOR_SIZE,
				  chunk * SECTOR_SIZE,
				  proc2pid(p_proc_current),	/// TASK_FS
				  fsbuf);

			if (fs_msg->type == READ) {
				memcpy((void*)va2la(src, buf + bytes_rw),
					  (void*)va2la(proc2pid(p_proc_current), fsbuf + off),
					  bytes);
			}
			else {	/* WRITE */
				memcpy((void*)va2la(proc2pid(p_proc_current), fsbuf + off),
					  (void*)va2la(src, buf + bytes_rw),
					  bytes);

				rw_sector(DEV_WRITE,	//modified by mingxuan 2019-5-21
					  pin->i_dev,
					  i * SECTOR_SIZE,
					  chunk * SECTOR_SIZE,
					  proc2pid(p_proc_current),
					  fsbuf);
			}
			off = 0;
			bytes_rw += bytes;
			p_proc_current->task.filp[fd]->fd_pos += bytes;
			bytes_left -= bytes;
		}

		if (p_proc_current->task.filp[fd]->fd_pos > pin->i_size) {
			/* update inode::size */
			pin->i_size = p_proc_current->task.filp[fd]->fd_pos;
			sync_inode(pin);
		}

		return bytes_rw;
	}
}

/// zcr copied from ch9/h/lib/unlink.c and modified it

/*****************************************************************************
 *                                unlink
 *****************************************************************************/
/**
 * Delete a file.
 * 
 * @param pathname  The full path of the file to delete.
 * 
 * @return Zero if successful, otherwise -1.
 *****************************************************************************/
int real_unlink(const char * pathname)	//modified by mingxuan 2019-5-17
{
	//added by xw, 18/8/27
	MESSAGE fs_msg;
	
	fs_msg.type   = UNLINK;
	fs_msg.PATHNAME	= (void*)pathname;
	fs_msg.NAME_LEN	= strlen(pathname);
	fs_msg.source = proc2pid(p_proc_current);	//added by xw, 18/12/22

	// send_recv(BOTH, TASK_FS, &msg);

	// return fs_msg.RETVAL;
	/// zcr added
	return do_unlink(&fs_msg);
}

/// zcr copied from the ch9/h/fs/link.c and modified it
/*****************************************************************************
 *                                do_unlink
 *****************************************************************************/
/**
 * Remove a file.
 *
 * @note We clear the i-node in inode_array[] although it is not really needed.
 *       We don't clear the data bytes so the file is recoverable.
 * 
 * @return On success, zero is returned.  On error, -1 is returned.
 *****************************************************************************/
static int do_unlink(MESSAGE *fs_msg)
{
	char pathname[MAX_PATH];

	/* get parameters from the message */
	int name_len = fs_msg->NAME_LEN;	/* length of filename */
	int src = fs_msg->source;	/* caller proc nr. */
	// assert(name_len < MAX_PATH);
	memcpy((void*)va2la(proc2pid(p_proc_current), pathname),
		  (void*)va2la(src, fs_msg->PATHNAME),
		  name_len);
	pathname[name_len] = 0;

	if (strcmp(pathname , "/") == 0) {
		kprintf("FS:do_unlink():: cannot unlink the root\n");
		return -1;
	}

	int inode_nr = search_file(pathname);
	if (inode_nr == INVALID_INODE) {	/* file not found */
		kprintf("FS::do_unlink():: search_file() returns invalid inode: %s\n", pathname);
		return -1;
	}

	char filename[MAX_PATH];
	struct inode * dir_inode;
	if (strip_path(filename, pathname, &dir_inode) != 0)
		return -1;

	struct inode * pin = get_inode_sched(dir_inode->i_dev, inode_nr);	//modified by xw, 18/8/28

	if (pin->i_mode != I_REGULAR) { /* can only remove regular files */
		kprintf("cannot remove file %s, because it is not a regular file.\n", pathname);
		return -1;
	}

	if (pin->i_cnt > 1) {	/* the file was opened */
		kprintf("cannot remove file %s, because pin->i_cnt is %d\n", pathname, pin->i_cnt);
		return -1;
	}

	struct super_block * sb = get_super_block(pin->i_dev);

	/*************************/
	/* free the bit in i-map */
	/*************************/
	int byte_idx = inode_nr / 8;
	int bit_idx = inode_nr % 8;
	// assert(byte_idx < SECTOR_SIZE);	/* we have only one i-map sector */
	/* read sector 2 (skip bootsect and superblk): */
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27
	RD_SECT_SCHED(pin->i_dev, 2, fsbuf);		//modified by xw, 18/12/27
	// assert(fsbuf[byte_idx % SECTOR_SIZE] & (1 << bit_idx));
	fsbuf[byte_idx % SECTOR_SIZE] &= ~(1 << bit_idx);
	WR_SECT_SCHED(pin->i_dev, 2, fsbuf);	//modified by xw, 18/12/27

	/**************************/
	/* free the bits in s-map */
	/**************************/
	/*
	 *           bit_idx: bit idx in the entire i-map
	 *     ... ____|____
	 *                  \        .-- byte_cnt: how many bytes between
	 *                   \      |              the first and last byte
	 *        +-+-+-+-+-+-+-+-+ V +-+-+-+-+-+-+-+-+
	 *    ... | | | | | |*|*|*|...|*|*|*|*| | | | |
	 *        +-+-+-+-+-+-+-+-+   +-+-+-+-+-+-+-+-+
	 *         0 1 2 3 4 5 6 7     0 1 2 3 4 5 6 7
	 *  ...__/
	 *      byte_idx: byte idx in the entire i-map
	 */
	bit_idx  = pin->i_start_sect - sb->n_1st_sect + 1;
	byte_idx = bit_idx / 8;
	int bits_left = pin->i_nr_sects;
	int byte_cnt = (bits_left - (8 - (bit_idx % 8))) / 8;

	/* current sector nr. */
	int s = 2  /* 2: bootsect + superblk */
		+ sb->nr_imap_sects + byte_idx / SECTOR_SIZE;

	RD_SECT_SCHED(pin->i_dev, s, fsbuf);		//modified by xw, 18/12/27

	int i;
	/* clear the first byte */
	for (i = bit_idx % 8; (i < 8) && bits_left; i++,bits_left--) {
		// assert((fsbuf[byte_idx % SECTOR_SIZE] >> i & 1) == 1);
		fsbuf[byte_idx % SECTOR_SIZE] &= ~(1 << i);
	}

	/* clear bytes from the second byte to the second to last */
	int k;
	i = (byte_idx % SECTOR_SIZE) + 1;	/* the second byte */
	for (k = 0; k < byte_cnt; k++,i++,bits_left-=8) {
		if (i == SECTOR_SIZE) {
			i = 0;
			WR_SECT_SCHED(pin->i_dev, s, fsbuf);		//modified by xw, 18/12/27
			RD_SECT_SCHED(pin->i_dev, ++s, fsbuf);		//modified by xw, 18/12/27
		}
		// assert(fsbuf[i] == 0xFF);
		fsbuf[i] = 0;
	}

	/* clear the last byte */
	if (i == SECTOR_SIZE) {
		i = 0;
		WR_SECT_SCHED(pin->i_dev, s, fsbuf);			//modified by xw, 18/12/27
		RD_SECT_SCHED(pin->i_dev, ++s, fsbuf);			//modified by xw, 18/12/27
	}
	// assert((fsbuf[i] & mask) == mask);
	fsbuf[i] &= (~0) << bits_left;
	WR_SECT_SCHED(pin->i_dev, s, fsbuf);				//modified by xw, 18/12/27

	/***************************/
	/* clear the i-node itself */
	/***************************/
	pin->i_mode = 0;
	pin->i_size = 0;
	pin->i_start_sect = 0;
	pin->i_nr_sects = 0;
	sync_inode(pin);
	/* release slot in inode_table[] */
	put_inode(pin);

	/************************************************/
	/* set the inode-nr to 0 in the directory entry */
	/************************************************/
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE) / SECTOR_SIZE;
	int nr_dir_entries =
		dir_inode->i_size / DIR_ENTRY_SIZE; /* including unused slots
						     * (the file has been
						     * deleted but the slot
						     * is still there)
						     */
	int m = 0;
	struct dir_entry * pde = 0;
	int flg = 0;
	int dir_size = 0;

	for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27

		pde = (struct dir_entry *)fsbuf;
		int j;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			if (++m > nr_dir_entries)
				break;

			if (pde->inode_nr == inode_nr) {
				/* pde->inode_nr = 0; */
				memset(pde, 0, DIR_ENTRY_SIZE);
				WR_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
				flg = 1;
				break;
			}

			if (pde->inode_nr != INVALID_INODE)
				dir_size += DIR_ENTRY_SIZE;
		}

		if (m > nr_dir_entries || /* all entries have been iterated OR */
		    flg) /* file is found */
			break;
	}
	// assert(flg);
	if (m == nr_dir_entries) { /* the file is the last one in the dir */
		dir_inode->i_size = dir_size;
		sync_inode(dir_inode);
	}

	return 0;
}

int real_lseek(int fd, int offset, int whence)	//modified by mingxuan 2019-5-17
{
	//added by xw, 18/8/27
	MESSAGE fs_msg;
	
	fs_msg.FD = fd;
	fs_msg.OFFSET = offset;
	fs_msg.WHENCE = whence;

	return do_lseek(&fs_msg);
}
/// zcr copied from ch9/j/fs/open.c
/*****************************************************************************
 *                                do_lseek
 *****************************************************************************/
/**
 * Handle the message LSEEK.
 * 
 * @return The new offset in bytes from the beginning of the file if successful,
 *         otherwise a negative number.
 *****************************************************************************/
static int do_lseek(MESSAGE *fs_msg)
{
	int fd = fs_msg->FD;
	int off = fs_msg->OFFSET;
	int whence = fs_msg->WHENCE;

	int pos = p_proc_current->task.filp[fd]->fd_pos;
	//int f_size = p_proc_current->task.filp[fd]->fd_inode->i_size; //deleted by mingxuan 2019-5-17
	int f_size = p_proc_current->task.filp[fd]->fd_node.fd_inode->i_size; //modified by mingxuan 2019-5-17

	switch (whence) {
	case SEEK_SET:
		pos = off;
		break;
	case SEEK_CUR:
		pos += off;
		break;
	case SEEK_END:
		pos = f_size + off;
		break;
	default:
		return -1;
		break;
	}
	if ((pos > f_size) || (pos < 0)) {
		return -1;
	}
	p_proc_current->task.filp[fd]->fd_pos = pos;
	return pos;
}