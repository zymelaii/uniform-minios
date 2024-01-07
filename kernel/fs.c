#include <unios/config.h>
#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/global.h>
#include <unios/assert.h>
#include <unios/proto.h>
#include <unios/fs_const.h>
#include <unios/hd.h>
#include <unios/fs.h>
#include <unios/fs_misc.h>
#include <sys/defs.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

extern struct file_desc   file_desc_table[NR_FILE_DESC];
extern struct super_block superblock_table[NR_SUPER_BLOCK];

static struct inode *root_inode;
static struct inode  inode_table[NR_INODE];

static void mkfs();

static int
    rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf);
static int rw_sector_sched(
    int io_type, int dev, int pos, int bytes, int proc_nr, void *buf);

static int
    strip_path(char *filename, const char *pathname, struct inode **ppinode);
static int           search_file(char *path);
static struct inode *create_file(char *path, int flags);
static struct inode *get_inode(int dev, int num);
static struct inode *get_inode_sched(int dev, int num);
static struct inode *new_inode(int dev, int inode_nr, int start_sect);
static void          put_inode(struct inode *pinode);
static void          sync_inode(struct inode *p);
static void
           new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename);
static int alloc_imap_bit(int dev);
static int alloc_smap_bit(int dev, int nr_sects_to_alloc);

int get_fs_dev(int drive, int fs_type) {
    int i = 0;
    for (i = 0; i < NR_PRIM_PER_DRIVE; i++) {
        if (hd_info[drive].primary[i].fs_type == fs_type)
            return ((DEV_HD << MAJOR_SHIFT) | i);
    }

    for (i = 0; i < NR_SUB_PER_DRIVE; i++) {
        if (hd_info[drive].logical[i].fs_type == fs_type)
            // logic的下标i加上hd1a才是该逻辑分区的次设备号
            return ((DEV_HD << MAJOR_SHIFT) | (i + MINOR_hd1a));
    }
    return 0;
}

void init_fs() {
    trace_logging("-----initialize filesystem-----\n");
    memset(inode_table, 0, sizeof(inode_table));
    superblock_t *sb = superblock_table;

    int orange_dev = get_fs_dev(PRIMARY_MASTER, ORANGE_TYPE);

    //! load super block of ROOT
    read_orange_superblock(orange_dev);
    superblock_t *sb_root = get_unique_superblock(orange_dev);

    trace_logging("Superblock Address: 0x%x\n", sb_root);

    if (sb_root->magic != MAGIC_V1) {
        mkfs();
        trace_logging("-----make filesystem done-----\n");
        read_orange_superblock(orange_dev);
    }

    root_inode = get_inode(orange_dev, ROOT_INODE);
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
static void mkfs() {
    int bits_per_sect = SECTOR_SIZE * 8; /* 8 bits per byte */

    // local array, to substitute global fsbuf. added by xw, 18/12/27
    char fsbuf[SECTOR_SIZE];

    int orange_dev = get_fs_dev(PRIMARY_MASTER, ORANGE_TYPE);

    //! get device geometry
    MESSAGE     msg = {};
    part_info_t geo = {};
    msg.type        = DEV_IOCTL;
    msg.DEVICE      = MINOR(orange_dev);
    msg.REQUEST     = DIOCTL_GET_GEO;
    msg.BUF         = &geo;
    msg.PROC_NR     = proc2pid(p_proc_current);
    hd_ioctl(&msg);

    trace_logging("-----make orange filesystem-----\n");
    trace_logging("device size: 0x%x sectors\n", geo.size);

    superblock_t sb   = {};
    sb.magic          = MAGIC_V1;
    sb.nr_inodes      = bits_per_sect;
    sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE;
    sb.nr_sects       = geo.size; /* partition size in sector */
    sb.nr_imap_sects  = 1;
    sb.nr_smap_sects  = sb.nr_sects / bits_per_sect + 1;
    //! include boot sector & superblock
    sb.n_1st_sect = sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects + 2;
    sb.root_inode = ROOT_INODE;
    sb.inode_size = INODE_SIZE;

    struct inode x     = {};
    sb.inode_isize_off = (int)&x.i_size - (int)&x;
    sb.inode_start_off = (int)&x.i_start_sect - (int)&x;
    sb.dir_ent_size    = DIR_ENTRY_SIZE;

    struct dir_entry de  = {};
    sb.dir_ent_inode_off = (void *)&de.inode_nr - (void *)&de;
    sb.dir_ent_fname_off = (void *)&de.name - (void *)&de;

    sb.sb_dev  = orange_dev;
    sb.fs_type = ORANGE_TYPE;

    //! FIXME: 0x90 is opcode of nop, but why?
    memset(fsbuf, 0x90, SECTOR_SIZE);
    //! FIXME: size should ignore mem member field
    memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);
    WR_SECT(orange_dev, 1, fsbuf);

    trace_logging(
        "orange geometry {\n"
        "  device base: 0x%x00,\n"
        "  superbock: 0x%x00,\n"
        "  inode map: 0x%x00,\n"
        "  sector map: 0x%x00,\n"
        "  inodes: 0x%x00,\n"
        "  first sector: 0x%x00,\n"
        "}\n",
        (geo.base + 0) * 2,
        (geo.base + 1) * 2,
        (geo.base + 2) * 2,
        (geo.base + 2 + sb.nr_imap_sects) * 2,
        (geo.base + 2 + sb.nr_imap_sects + sb.nr_smap_sects) * 2,
        (geo.base + sb.n_1st_sect) * 2);

    //! resolve inode map
    //! include root, curdir, tty0, tty1, tty2, orange
    memset(fsbuf, 0, SECTOR_SIZE);
    for (int i = 0; i < (NR_CONSOLES + 3); ++i) { fsbuf[0] |= 1 << i; };
    WR_SECT(orange_dev, 2, fsbuf);

    //! resolve sector map
    //! FIXME: why assume 1 sector is enough for nr_sects in sector map
    int nr_sects = NR_DEFAULT_FILE_SECTS + 1;
    memset(fsbuf, 0, SECTOR_SIZE);
    //! NOTE: the following 2 lines fills nr_sects bits
    memset(fsbuf, 0xff, nr_sects / 8);
    fsbuf[nr_sects / 8] |= (1 << (nr_sects % 8 + 1)) - 1;
    //! write first sector map
    WR_SECT(orange_dev, 2 + sb.nr_imap_sects, fsbuf);
    //! zero the rest sector map
    memset(fsbuf, 0, SECTOR_SIZE);
    for (int i = 1; i < sb.nr_smap_sects; ++i) {
        WR_SECT(orange_dev, 2 + sb.nr_imap_sects + i, fsbuf);
    }

    //! resolve app.tar
    //! sect M <-> bit (M - sb.n_1stsect + 1)
    int bit_offset      = INSTALL_START_SECTOR - sb.n_1st_sect + 1;
    int bit_off_in_sect = bit_offset % (SECTOR_SIZE * 8);
    int bit_left        = INSTALL_NR_SECTORS;
    int cur_sect        = bit_offset / (SECTOR_SIZE * 8);
    RD_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf);
    while (bit_left) {
        int byte_off = bit_off_in_sect / 8;
        /* this line is ineffecient in a loop, but I don't care */
        fsbuf[byte_off] |= 1 << (bit_off_in_sect % 8);
        --bit_left;
        ++bit_off_in_sect;
        if (bit_off_in_sect == (SECTOR_SIZE * 8)) {
            WR_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf);
            ++cur_sect;
            RD_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf);
            bit_off_in_sect = 0;
        }
    }
    WR_SECT(orange_dev, 2 + sb.nr_imap_sects + cur_sect, fsbuf);

    /************************/
    /*       inodes         */
    /************************/
    /* inode of `/' */
    memset(fsbuf, 0, SECTOR_SIZE);
    struct inode *pi = (struct inode *)fsbuf;
    pi->i_mode       = I_DIRECTORY;

    //! 预定义 5 个文件
    //! 1. 当前目录 .
    //! 2. TTY0 dev_tty0
    //! 3. TTY1 dev_tty1
    //! 4. TTY2 dev_tty2
    //! 5. app.tar
    pi->i_size = DIR_ENTRY_SIZE * 5;

    pi->i_start_sect = sb.n_1st_sect;
    pi->i_nr_sects   = NR_DEFAULT_FILE_SECTS;

    /* inode of `/dev_tty0~2' */
    for (int i = 0; i < NR_CONSOLES; ++i) {
        pi               = (struct inode *)(fsbuf + (INODE_SIZE * (i + 1)));
        pi->i_mode       = I_CHAR_SPECIAL;
        pi->i_size       = 0;
        pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
        pi->i_nr_sects   = 0;
    }

    /* inode of /app.tar */
    pi         = (struct inode *)(fsbuf + (INODE_SIZE * (NR_CONSOLES + 1)));
    pi->i_mode = I_REGULAR;
    pi->i_size = INSTALL_NR_SECTORS * SECTOR_SIZE;
    pi->i_start_sect = INSTALL_START_SECTOR;
    pi->i_nr_sects   = INSTALL_NR_SECTORS;

    WR_SECT(orange_dev, 2 + sb.nr_imap_sects + sb.nr_smap_sects, fsbuf);

    /************************/
    /*          `/'         */
    /************************/
    memset(fsbuf, 0, SECTOR_SIZE);
    struct dir_entry *pde = (struct dir_entry *)fsbuf;

    pde->inode_nr = 1;
    strcpy(pde->name, ".");

    //! assign dir entry for tty
    for (int i = 0; i < NR_CONSOLES; i++) {
        ++pde;
        pde->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
        snprintf(pde->name, sizeof(pde->name), "dev_tty%d", i);
    }

    //! assign dir entrie
    (++pde)->inode_nr = NR_CONSOLES + 2;
    strcpy(pde->name, INSTALL_FILENAME);
    WR_SECT(orange_dev, sb.n_1st_sect, fsbuf);
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
static int rw_sector(
    int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf) {
    MESSAGE driver_msg;

    driver_msg.type     = io_type;
    driver_msg.DEVICE   = MINOR(dev);
    driver_msg.POSITION = pos;
    driver_msg.CNT      = bytes; /// hu is: 512
    driver_msg.PROC_NR  = proc_nr;
    driver_msg.BUF      = buf;

    hd_rdwt(&driver_msg);
    return 0;
}

static int rw_sector_sched(
    int io_type, int dev, int pos, int bytes, int proc_nr, void *buf) {
    MESSAGE driver_msg;

    driver_msg.type   = io_type;
    driver_msg.DEVICE = MINOR(dev);

    driver_msg.POSITION = pos;
    driver_msg.CNT      = bytes; /// hu is: 512
    driver_msg.PROC_NR  = proc_nr;
    driver_msg.BUF      = buf;

    hd_rdwt_sched(&driver_msg);
    return 0;
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
static struct inode *create_file(char *path, int flags) {
    char          filename[PATH_MAX];
    struct inode *dir_inode;
    if (strip_path(filename, path, &dir_inode) != 0) return 0;
    int inode_nr     = alloc_imap_bit(dir_inode->i_dev);
    int free_sect_nr = alloc_smap_bit(dir_inode->i_dev, NR_DEFAULT_FILE_SECTS);
    struct inode *newino = new_inode(dir_inode->i_dev, inode_nr, free_sect_nr);
    new_dir_entry(dir_inode, newino->i_num, filename);
    return newino;
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
static int search_file(char *path) {
    int i, j;

    char filename[PATH_MAX];
    memset(filename, 0, FILENAME_MAX);
    struct inode *dir_inode;
    if (strip_path(filename, path, &dir_inode) != 0) return 0;

    //! path must begin with root dir "/"
    if (filename[0] == 0) { return dir_inode->i_num; }

    //! Search the dir for the file.
    int dir_blk0_nr = dir_inode->i_start_sect;
    int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    int nr_dir_entries =
        dir_inode->i_size / DIR_ENTRY_SIZE; /**
                                             * including unused slots
                                             * (the file has been deleted
                                             * but the slot is still there)
                                             */
    int               m = 0;
    struct dir_entry *pde;
    char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added
                             // by xw, 18/12/27
    for (i = 0; i < nr_dir_blks; i++) {
        // RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified
        // by xw, 18/12/27
        RD_SECT(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);
        pde = (struct dir_entry *)fsbuf;
        for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++) {
            if (memcmp(filename, pde->name, FILENAME_MAX) == 0)
                return pde->inode_nr;
            if (++m > nr_dir_entries) break;
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

static int strip_path(char *file, const char *path, struct inode **ppinode) {
    const char *s = path;
    char       *t = file;
    if (s == 0) { return -1; }
    if (*s == '/') { s++; }
    while (*s) { /* check each character */
        if (*s == '/') { return -1; }
        *t++ = *s++;
        /* if file is too long, just truncate it */
        if (t - file >= FILENAME_MAX) { break; }
    }
    *t       = 0;
    *ppinode = root_inode;
    return 0;
}

void read_orange_superblock(int dev) {
    char fsbuf[SECTOR_SIZE] = {};

    MESSAGE driver_msg  = {};
    driver_msg.type     = DEV_READ;
    driver_msg.DEVICE   = MINOR(dev);
    driver_msg.POSITION = SECTOR_SIZE * 1;
    driver_msg.BUF      = fsbuf;
    driver_msg.CNT      = SECTOR_SIZE;
    driver_msg.PROC_NR  = proc2pid(p_proc_current); /// TASK_A
    hd_rdwt(&driver_msg);

    int index = 0;
    while (index < NR_SUPER_BLOCK) {
        if (superblock_table[index].fs_type == ORANGE_TYPE) { break; }
        ++index;
    }
    assert(index < NR_SUPER_BLOCK && "orange's surblock not exists");

    superblock_t *sb = &superblock_table[index];
    memcpy(sb, fsbuf, sizeof(superblock_t));
    sb->sb_dev  = dev;
    sb->fs_type = ORANGE_TYPE;
}

superblock_t *get_unique_superblock(int dev) {
    superblock_t *sb = NULL;
    for (int i = 0; i < NR_SUPER_BLOCK; ++i) {
        if (superblock_table[i].sb_dev != dev) { continue; }
        //! expected device is not unique
        if (sb != NULL) { return NULL; }
        sb = &superblock_table[i];
    }
    return sb;
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
static struct inode *get_inode(int dev, int num) {
    if (num == 0) return 0;

    struct inode *p;
    struct inode *q = 0;
    for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) {
        if (p->i_cnt) { /* not a free slot */
            if ((p->i_dev == dev) && (p->i_num == num)) {
                /* this is the inode we want */
                p->i_cnt++;
                return p;
            }
        } else {       /* a free slot */
            if (!q)    /* q hasn't been assigned yet */
                q = p; /* q <- the 1st free slot */
        }
    }

    if (!q) panic("the inode table is full");

    q->i_dev = dev;
    q->i_num = num;
    q->i_cnt = 1;

    superblock_t *sb     = get_unique_superblock(dev);
    int           blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects
               + ((num - 1) / (SECTOR_SIZE / INODE_SIZE));
    char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf.
    RD_SECT(dev, blk_nr, fsbuf);
    struct inode *pinode =
        (struct inode
             *)((u8 *)fsbuf + ((num - 1) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE);
    q->i_mode       = pinode->i_mode;
    q->i_size       = pinode->i_size;
    q->i_start_sect = pinode->i_start_sect;
    q->i_nr_sects   = pinode->i_nr_sects;
    return q;
}

static struct inode *get_inode_sched(int dev, int num) {
    if (num == 0) return 0;

    struct inode *p;
    struct inode *q = 0;
    for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) {
        if (p->i_cnt) { /* not a free slot */
            if ((p->i_dev == dev) && (p->i_num == num)) {
                /* this is the inode we want */
                p->i_cnt++;
                return p;
            }
        } else {       /* a free slot */
            if (!q)    /* q hasn't been assigned yet */
                q = p; /* q <- the 1st free slot */
        }
    }

    if (!q) panic("the inode table is full");

    q->i_dev = dev;
    q->i_num = num;
    q->i_cnt = 1;

    superblock_t *sb     = get_unique_superblock(dev);
    int           blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects
               + ((num - 1) / (SECTOR_SIZE / INODE_SIZE));
    char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added
                             // by xw, 18/12/27
    RD_SECT_SCHED(dev, blk_nr, fsbuf);
    struct inode *pinode =
        (struct inode
             *)((u8 *)fsbuf + ((num - 1) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE);
    q->i_mode       = pinode->i_mode;
    q->i_size       = pinode->i_size;
    q->i_start_sect = pinode->i_start_sect;
    q->i_nr_sects   = pinode->i_nr_sects;
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
static void put_inode(struct inode *pinode) {
    assert(pinode->i_cnt > 0);
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
static void sync_inode(struct inode *p) {
    struct inode *pinode;
    superblock_t *sb     = get_unique_superblock(p->i_dev);
    int           blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects
               + ((p->i_num - 1) / (SECTOR_SIZE / INODE_SIZE));
    char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added
                             // by xw, 18/12/27
    RD_SECT(p->i_dev, blk_nr, fsbuf);
    pinode =
        (struct inode
             *)((u8 *)fsbuf + (((p->i_num - 1) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE));
    pinode->i_mode       = p->i_mode;
    pinode->i_size       = p->i_size;
    pinode->i_start_sect = p->i_start_sect;
    pinode->i_nr_sects   = p->i_nr_sects;
    WR_SECT(p->i_dev, blk_nr, fsbuf);
}

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
static struct inode *new_inode(int dev, int inode_nr, int start_sect) {
    // struct inode * new_inode = get_inode_sched(dev, inode_nr);	//modified
    // by xw, 18/8/28
    struct inode *new_inode = get_inode(dev, inode_nr);

    new_inode->i_mode       = I_REGULAR;
    new_inode->i_size       = 0;
    new_inode->i_start_sect = start_sect;
    new_inode->i_nr_sects   = NR_DEFAULT_FILE_SECTS;

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
static void
    new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename) {
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
    int               m = 0;
    struct dir_entry *pde;
    struct dir_entry *new_de = 0;

    int  i, j;
    char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added
                             // by xw, 18/12/27
    for (i = 0; i < nr_dir_blks; i++) {
        RD_SECT(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);

        pde = (struct dir_entry *)fsbuf;
        for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++) {
            if (++m > nr_dir_entries) break;

            if (pde->inode_nr == 0) { /* it's a free slot */
                new_de = pde;
                break;
            }
        }
        if (m > nr_dir_entries || /* all entries have been iterated or */
            new_de)               /* free slot is found */
            break;
    }
    if (!new_de) { /* reached the end of the dir */
        new_de            = pde;
        dir_inode->i_size += DIR_ENTRY_SIZE;
    }
    new_de->inode_nr = inode_nr;
    strcpy(new_de->name, filename);

    /* write dir block -- ROOT dir block */
    WR_SECT(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);

    /* update dir inode */
    sync_inode(dir_inode);
}

static int alloc_imap_bit(int dev) {
    int inode_nr = 0;
    int i, j, k;

    int           imap_blk0_nr = 1 + 1; /* 1 boot sector & 1 super block */
    superblock_t *sb           = get_unique_superblock(dev);

    char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added
                             // by xw, 18/12/27

    for (i = 0; i < sb->nr_imap_sects; i++) {
        RD_SECT(dev, imap_blk0_nr + i, fsbuf);

        for (j = 0; j < SECTOR_SIZE; j++) {
            if (fsbuf[j] == '\xFF') continue;

            /* skip `1' bits */
            for (k = 0; ((fsbuf[j] >> k) & 1) != 0; k++) {}

            /* i: sector index; j: byte index; k: bit index */
            inode_nr = (i * SECTOR_SIZE + j) * 8 + k;
            fsbuf[j] |= (1 << k);

            /* write the bit to imap */
            WR_SECT(dev, imap_blk0_nr + i, fsbuf);
            // 2019-5-20
            break;
        }

        return inode_nr;
    }

    /* no free bit in imap */
    panic("inode-map is probably full.\n");

    return 0;
}

//! \return 1st sector nr allocated.
static int alloc_smap_bit(int dev, int nr_sects_to_alloc) {
    /* int nr_sects_to_alloc = NR_DEFAULT_FILE_SECTS; */

    int i; /* sector index */
    int j; /* byte index */
    int k; /* bit index */

    superblock_t *sb = get_unique_superblock(dev);

    int  smap_blk0_nr = 1 + 1 + sb->nr_imap_sects;
    int  free_sect_nr = 0;
    char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added
                             // by xw, 18/12/27

    for (i = 0; i < sb->nr_smap_sects; i++) { /* smap_blk0_nr + i :
                             current sect nr. */
        // RD_SECT_SCHED(dev, smap_blk0_nr + i, fsbuf);	//modified by xw,
        // 18/12/27
        RD_SECT(dev, smap_blk0_nr + i, fsbuf);

        /* byte offset in current sect */
        for (j = 0; j < SECTOR_SIZE && nr_sects_to_alloc > 0; j++) {
            k = 0;
            if (!free_sect_nr) {
                //! loop until a free bit is found
                // if (fsbuf[j] == 0xFF) continue;
                if (fsbuf[j] == '\xFF') continue;
                for (; ((fsbuf[j] >> k) & 1) != 0; k++) {}
                free_sect_nr =
                    (i * SECTOR_SIZE + j) * 8 + k - 1 + sb->n_1st_sect;
            }

            //! repeat till enough bits are set
            for (; k < 8; k++) {
                fsbuf[j] |= (1 << k);
                if (--nr_sects_to_alloc == 0) break;
            }
        }

        //! free bit found, write the bits to smap
        if (free_sect_nr) { WR_SECT(dev, smap_blk0_nr + i, fsbuf); }
        if (nr_sects_to_alloc == 0) { break; }
    }
    return free_sect_nr;
}

static int do_open(MESSAGE *fs_msg) {
    //! FIXME: minios do not recursively setup fs from the stroage, but do_open
    //! will try to read infomation from sectors, then inconsistency occurs

    /*caller_nr is the process number of the */
    int fd = -1;

    char pathname[PATH_MAX];

    /* get parameters from the message */
    int flags    = fs_msg->FLAGS;    /* access mode */
    int name_len = fs_msg->NAME_LEN; /* length of filename */
    int src      = fs_msg->source;   /* caller proc nr. */

    memcpy(
        (void *)va2la(src, pathname),
        (void *)va2la(src, fs_msg->PATHNAME),
        name_len);
    pathname[name_len] = 0;

    //! find a free slot in proc file_desc
    for (int i = 0; i < NR_FILES; i++) {
        if (p_proc_current->pcb.filp[i] == 0) {
            fd = i;
            break;
        }
    }
    assert(0 <= fd && fd < NR_FILES);

    //! find a free slot in file_desc_table
    int index = 0;
    while (index < NR_FILE_DESC) {
        if (file_desc_table[index].flag == 0) { break; }
        ++index;
    }
    assert(index < NR_FILE_DESC);

    int           inode_nr = search_file(pathname);
    struct inode *pin      = 0;
    if (flags & O_CREAT) {
        if (inode_nr) { return -1; }
        pin = create_file(pathname, flags);
    } else {
        char          filename[PATH_MAX] = {};
        struct inode *dir_inode          = NULL;
        if (strip_path(filename, pathname, &dir_inode) != 0) { return -1; }
        pin = get_inode(dir_inode->i_dev, inode_nr);
    }

    if (pin) {
        /* connects proc with file_descriptor */
        p_proc_current->pcb.filp[fd] = &file_desc_table[index];

        file_desc_table[index].flag = 1;

        /* connects file_descriptor with inode */
        file_desc_table[index].fd_node.fd_inode = pin;

        file_desc_table[index].fd_mode = flags;
        file_desc_table[index].fd_pos  = 0;

        int imode = pin->i_mode & I_TYPE_MASK;

        if (imode == I_CHAR_SPECIAL) {
            // MESSAGE driver_msg;
            // int dev = pin->i_start_sect;
        } else if (imode == I_DIRECTORY) {
            if (pin->i_num != ROOT_INODE) { panic("pin->i_num != ROOT_INODE"); }
        } else {
            if (pin->i_mode != I_REGULAR) {
                panic("Panic: pin->i_mode != I_REGULAR");
            }
        }
    } else {
        return -1;
    }

    return fd;
}

static int do_close(int fd) {
    //! FIXME: check succeed or not
    file_desc_t **p_desc = &p_proc_current->pcb.filp[fd];
    put_inode((*p_desc)->fd_node.fd_inode);
    (*p_desc)->fd_node.fd_inode = NULL;
    (*p_desc)->flag             = 0;
    *p_desc                     = NULL;
    return 0;
}

//! NOTE: Sector map is not needed to update, since the sectors for the file
//! have been allocated and the bits are set when the file was created.
static int do_rdwt(MESSAGE *fs_msg) {
    int   fd  = fs_msg->FD;  /**< file descriptor. */
    void *buf = fs_msg->BUF; /**< r/w buffer */
    int   len = fs_msg->CNT; /**< r/w bytes */

    //! caller proc nr
    int caller = fs_msg->source;

    if (!(p_proc_current->pcb.filp[fd]->fd_mode & O_RDWR)) return -1;

    int pos = p_proc_current->pcb.filp[fd]->fd_pos;

    struct inode *pin = p_proc_current->pcb.filp[fd]->fd_node.fd_inode;

    int imode = pin->i_mode & I_TYPE_MASK;

    if (imode == I_CHAR_SPECIAL) {
        int t        = fs_msg->type == READ ? DEV_READ : DEV_WRITE;
        fs_msg->type = t;

        int dev    = pin->i_start_sect;
        int nr_tty = MINOR(dev);
        if (MAJOR(dev) != 4) { panic("Error: MAJOR(dev) == 4\n"); }

        if (fs_msg->type == DEV_READ) {
            fs_msg->CNT = tty_read(&tty_table[nr_tty], buf, len);
        } else if (fs_msg->type == DEV_WRITE) {
            tty_write(&tty_table[nr_tty], buf, len);
        }

        return fs_msg->CNT;
    }

    int pos_end;
    if (fs_msg->type == READ)
        pos_end = MIN(pos + len, pin->i_size);
    else /* WRITE */
        pos_end = MIN(pos + len, pin->i_nr_sects * SECTOR_SIZE);

    int off         = pos % SECTOR_SIZE;
    int rw_sect_min = pin->i_start_sect + (pos >> SECTOR_SIZE_SHIFT);
    int rw_sect_max = pin->i_start_sect + (pos_end >> SECTOR_SIZE_SHIFT);

    int chunk =
        MIN(rw_sect_max - rw_sect_min + 1, SECTOR_SIZE >> SECTOR_SIZE_SHIFT);

    int bytes_rw   = 0;
    int bytes_left = len;
    int i;

    char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf.

    for (i = rw_sect_min; i <= rw_sect_max; i += chunk) {
        /* read/write this amount of bytes every time */
        int bytes = MIN(bytes_left, chunk * SECTOR_SIZE - off);
        rw_sector(
            DEV_READ,
            pin->i_dev,
            i * SECTOR_SIZE,
            chunk * SECTOR_SIZE,
            proc2pid(p_proc_current), /// TASK_FS
            fsbuf);

        if (fs_msg->type == READ) {
            memcpy(
                (void *)va2la(caller, buf + bytes_rw),
                (void *)va2la(proc2pid(p_proc_current), fsbuf + off),
                bytes);
        } else { /* WRITE */
            memcpy(
                (void *)va2la(proc2pid(p_proc_current), fsbuf + off),
                (void *)va2la(caller, buf + bytes_rw),
                bytes);

            rw_sector(
                DEV_WRITE,
                pin->i_dev,
                i * SECTOR_SIZE,
                chunk * SECTOR_SIZE,
                proc2pid(p_proc_current),
                fsbuf);
        }
        off                                  = 0;
        bytes_rw                             += bytes;
        p_proc_current->pcb.filp[fd]->fd_pos += bytes;
        bytes_left                           -= bytes;
    }

    if (p_proc_current->pcb.filp[fd]->fd_pos > pin->i_size) {
        /* update inode::size */
        pin->i_size = p_proc_current->pcb.filp[fd]->fd_pos;
        sync_inode(pin);
    }

    return bytes_rw;
}

//! NOTE: We clear the i-node in inode_array[] although it is not really needed.
//! We don't clear the data bytes so the file is recoverable.
static int do_unlink(MESSAGE *fs_msg) {
    char pathname[PATH_MAX];

    /* get parameters from the message */
    int name_len = fs_msg->NAME_LEN; /* length of filename */
    int src      = fs_msg->source;   /* caller proc nr. */
    // assert(name_len < PATH_MAX);
    memcpy(
        (void *)va2la(proc2pid(p_proc_current), pathname),
        (void *)va2la(src, fs_msg->PATHNAME),
        name_len);
    pathname[name_len] = 0;

    if (strcmp(pathname, "/") == 0) {
        kprintf("FS:do_unlink():: cannot unlink the root\n");
        return -1;
    }

    int inode_nr = search_file(pathname);
    if (inode_nr == INVALID_INODE) { /* file not found */
        kprintf(
            "FS::do_unlink():: search_file() returns invalid inode: %s\n",
            pathname);
        return -1;
    }

    char          filename[PATH_MAX];
    struct inode *dir_inode;
    if (strip_path(filename, pathname, &dir_inode) != 0) return -1;

    struct inode *pin = get_inode_sched(dir_inode->i_dev, inode_nr);

    if (pin->i_mode != I_REGULAR) { /* can only remove regular files */
        kprintf(
            "cannot remove file %s, because it is not a regular file.\n",
            pathname);
        return -1;
    }

    if (pin->i_cnt > 1) { /* the file was opened */
        kprintf(
            "cannot remove file %s, because pin->i_cnt is %d\n",
            pathname,
            pin->i_cnt);
        return -1;
    }

    superblock_t *sb = get_unique_superblock(pin->i_dev);

    /*************************/
    /* free the bit in i-map */
    /*************************/
    int byte_idx = inode_nr / 8;
    int bit_idx  = inode_nr % 8;
    // assert(byte_idx < SECTOR_SIZE);	/* we have only one i-map sector */
    /* read sector 2 (skip bootsect and superblk): */
    char fsbuf[SECTOR_SIZE]; // local array, to substitute global fsbuf. added
                             // by xw, 18/12/27
    RD_SECT_SCHED(pin->i_dev, 2, fsbuf);
    // assert(fsbuf[byte_idx % SECTOR_SIZE] & (1 << bit_idx));
    fsbuf[byte_idx % SECTOR_SIZE] &= ~(1 << bit_idx);
    WR_SECT_SCHED(pin->i_dev, 2, fsbuf);

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
    bit_idx       = pin->i_start_sect - sb->n_1st_sect + 1;
    byte_idx      = bit_idx / 8;
    int bits_left = pin->i_nr_sects;
    int byte_cnt  = (bits_left - (8 - (bit_idx % 8))) / 8;

    /* current sector nr. */
    int s = 2 /* 2: bootsect + superblk */
          + sb->nr_imap_sects + byte_idx / SECTOR_SIZE;

    RD_SECT_SCHED(pin->i_dev, s, fsbuf);

    int i;
    /* clear the first byte */
    for (i = bit_idx % 8; (i < 8) && bits_left; i++, bits_left--) {
        // assert((fsbuf[byte_idx % SECTOR_SIZE] >> i & 1) == 1);
        fsbuf[byte_idx % SECTOR_SIZE] &= ~(1 << i);
    }

    /* clear bytes from the second byte to the second to last */
    int k;
    i = (byte_idx % SECTOR_SIZE) + 1; /* the second byte */
    for (k = 0; k < byte_cnt; k++, i++, bits_left -= 8) {
        if (i == SECTOR_SIZE) {
            i = 0;
            WR_SECT_SCHED(pin->i_dev, s, fsbuf);
            RD_SECT_SCHED(pin->i_dev, ++s, fsbuf);
        }
        // assert(fsbuf[i] == 0xFF);
        fsbuf[i] = 0;
    }

    /* clear the last byte */
    if (i == SECTOR_SIZE) {
        i = 0;
        WR_SECT_SCHED(pin->i_dev, s, fsbuf);
        RD_SECT_SCHED(pin->i_dev, ++s, fsbuf);
    }
    // assert((fsbuf[i] & mask) == mask);
    fsbuf[i] &= (~0) << bits_left;
    WR_SECT_SCHED(pin->i_dev, s, fsbuf);

    /***************************/
    /* clear the i-node itself */
    /***************************/
    pin->i_mode       = 0;
    pin->i_size       = 0;
    pin->i_start_sect = 0;
    pin->i_nr_sects   = 0;
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
    int               m        = 0;
    struct dir_entry *pde      = 0;
    int               flg      = 0;
    int               dir_size = 0;

    for (i = 0; i < nr_dir_blks; i++) {
        RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);

        pde = (struct dir_entry *)fsbuf;
        int j;
        for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++) {
            if (++m > nr_dir_entries) break;

            if (pde->inode_nr == inode_nr) {
                /* pde->inode_nr = 0; */
                memset(pde, 0, DIR_ENTRY_SIZE);
                WR_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);
                flg = 1;
                break;
            }

            if (pde->inode_nr != INVALID_INODE) dir_size += DIR_ENTRY_SIZE;
        }

        if (m > nr_dir_entries || /* all entries have been iterated OR */
            flg)                  /* file is found */
            break;
    }
    // assert(flg);
    if (m == nr_dir_entries) { /* the file is the last one in the dir */
        dir_inode->i_size = dir_size;
        sync_inode(dir_inode);
    }

    return 0;
}

static int do_lseek(MESSAGE *fs_msg) {
    int fd     = fs_msg->FD;
    int off    = fs_msg->OFFSET;
    int whence = fs_msg->WHENCE;

    int pos    = p_proc_current->pcb.filp[fd]->fd_pos;
    int f_size = p_proc_current->pcb.filp[fd]->fd_node.fd_inode->i_size;

    if (whence == SEEK_SET) {
        pos = off;
    } else if (whence == SEEK_CUR) {
        pos += off;
    } else if (whence == SEEK_END) {
        //! here off is expected to be negative
        pos = f_size + off;
    } else {
        //! invalid whence
        return -1;
    }

    if (pos < 0 || pos > f_size) { return -1; }
    p_proc_current->pcb.filp[fd]->fd_pos = pos;
    return pos;
}

int real_open(const char *pathname, int flags) {
    MESSAGE fs_msg  = {};
    fs_msg.type     = OPEN;
    fs_msg.PATHNAME = (void *)pathname;
    fs_msg.FLAGS    = flags;
    fs_msg.NAME_LEN = strlen(pathname);
    fs_msg.source   = proc2pid(p_proc_current);
    return do_open(&fs_msg);
}

int real_close(int fd) {
    return do_close(fd);
}

int real_read(int fd, void *buf, int count) {
    MESSAGE fs_msg = {};
    fs_msg.type    = READ;
    fs_msg.FD      = fd;
    fs_msg.BUF     = buf;
    fs_msg.CNT     = count;
    fs_msg.source  = proc2pid(p_proc_current);
    int total_rd   = do_rdwt(&fs_msg);
    assert(total_rd == fs_msg.CNT);
    return total_rd;
}

int real_write(int fd, const void *buf, int count) {
    MESSAGE fs_msg = {};
    fs_msg.type    = WRITE;
    fs_msg.FD      = fd;
    fs_msg.BUF     = (void *)buf;
    fs_msg.CNT     = count;
    fs_msg.source  = proc2pid(p_proc_current);
    int total_wr   = do_rdwt(&fs_msg);
    assert(total_wr == fs_msg.CNT);
    return total_wr;
}

int real_unlink(const char *pathname) {
    MESSAGE fs_msg  = {};
    fs_msg.type     = UNLINK;
    fs_msg.PATHNAME = (void *)pathname;
    fs_msg.NAME_LEN = strlen(pathname);
    fs_msg.source   = proc2pid(p_proc_current);
    return do_unlink(&fs_msg);
}

int real_lseek(int fd, int offset, int whence) {
    MESSAGE fs_msg = {};
    fs_msg.FD      = fd;
    fs_msg.OFFSET  = offset;
    fs_msg.WHENCE  = whence;
    return do_lseek(&fs_msg);
}
