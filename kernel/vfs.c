#include <unios/vfs.h>
#include <unios/fs_const.h>
#include <unios/fs.h>
#include <unios/assert.h>
#include <unios/global.h>
#include <unios/proto.h>
#include <sys/defs.h>
#include <stdio.h>
#include <string.h>

file_desc_t  file_desc_table[NR_FILE_DESC];
superblock_t superblock_table[NR_SUPER_BLOCK];

static vfs_t               vfs_table[NR_FS];
static file_op_set_t       fs_op_table[NR_FS_OP];
static superblock_op_set_t sb_op_table[NR_SB_OP];

//! NOTE: used in get_next_dev_nr, generate available dev_nr from global counter
//! NOTE: auto zero is not available in kernel.bin, so mannually do it in
//! vfs_setup_and_init
int dev_nr_counter;

//! initial vfs assignment: {tty0, tty1, tty2, orange, fat32}
#define NR_TTY         (NR_CONSOLES)
#define NR_INITIAL_VFS (NR_TTY + 2)

//! vfs set
#define TTY_VFS(i) (vfs_table[i])
#define ORANGE_VFS (vfs_table[NR_TTY + 0])
#define FAT32_VFS  (vfs_table[NR_TTY + 1])

//! fs op set
#define TTY_FS_OP    (fs_op_table[0])
#define ORANGE_FS_OP (fs_op_table[1])
#define FAT32_FS_OP  (fs_op_table[2])

//! superblock set
#define TTY_SUPERBLOCK(i) (superblock_table[i])
#define ORANGE_SUPERBLOCK (superblock_table[NR_TTY + 0])
#define FAT32_SUPERBLOCK  (superblock_table[NR_TTY + 1])

//! superblock op set
#define NULL_SB_OP   (sb_op_table[0])
#define ORANGE_SB_OP (sb_op_table[1])

static int prefix_match(const char *src, const char *dst) {
    assert(src != NULL);
    assert(dst != NULL);
    const char *p = src;
    const char *q = dst;
    while (*p && *q) {
        if (*p != *q) { break; }
        ++p;
        ++q;
    }
    return p - src;
}

//! NOTE: use longest prefix matching alg
static int get_vfs_index(const char *path) {
    int nr     = -1; //<! vfs nr
    int maxlen = -1; //<! current longest prefix length
    for (int i = 0; i < NR_FS; ++i) {
        const char *vfs_name = vfs_table[i].name;
        if (vfs_name == NULL) {
            assert(vfs_table[i].nr_dev == -1 && "dirty invalid vfs entry");
            continue;
        }
        int matched = prefix_match(vfs_name, path);
        if (matched == 0) { continue; }
        if (!(path[matched] == '\0' || path[matched] == '/')) { continue; }
        assert(matched != maxlen && "duplicate vfs entry");
        if (matched > maxlen) {
            maxlen = matched;
            nr     = i;
        }
    }
    return nr;
}

static int get_vfs_index_and_relpath(const char *path, const char **p_relpath) {
    assert(p_relpath != NULL);
    *p_relpath = NULL;

    int index = get_vfs_index(path);
    if (index != -1) {
        int devlen = strlen(vfs_table[index].name);
        assert(devlen > 0);
        assert(path[devlen] == '/' || path[devlen] == '\0');
        //! get path relative to vfs device
        //! NOTE: relpath rules:
        //! 1. given path `/dev/a/cc/e`, dev `/dev/a`, then relpath is `/cc/e`
        //! 2. given path `/dev/a`, dev `/dev/a`, then relpath is `` (empty)
        *p_relpath = path + devlen;
    }

    return index;
}

static void init_superblock_table() {
    for (int i = 0; i < NR_TTY; ++i) {
        TTY_SUPERBLOCK(i).sb_dev  = DEV_CHAR_TTY;
        TTY_SUPERBLOCK(i).fs_type = TTY_FS_TYPE;
    }

    ORANGE_SUPERBLOCK.sb_dev  = DEV_HD;
    ORANGE_SUPERBLOCK.fs_type = ORANGE_TYPE;

    FAT32_SUPERBLOCK.sb_dev  = DEV_HD;
    FAT32_SUPERBLOCK.fs_type = FAT32_TYPE;
}

static int get_next_dev_nr() {
    int dev_nr = ++dev_nr_counter;
    assert(dev_nr > 0 && "dev nr out of range");
    return dev_nr;
}

static void init_vfs_table() {
    //! FIXME: minios doesn't support strdup yet, use literal instead
    //! FIXME: should i use the absolute path or dev name only?
    const char *tty_name_list[NR_TTY] = {
        "/dev_tty0",
        "/dev_tty1",
        "/dev_tty2",
    };

    for (int i = 0; i < NR_TTY; ++i) {
        TTY_VFS(i).name   = tty_name_list[i];
        TTY_VFS(i).nr_dev = get_next_dev_nr();
        TTY_VFS(i).ops    = &TTY_FS_OP;
        TTY_VFS(i).sb     = &TTY_SUPERBLOCK(i);
        TTY_VFS(i).sb_ops = &NULL_SB_OP;
    }

    ORANGE_VFS.name   = "/orange";
    ORANGE_VFS.nr_dev = get_next_dev_nr();
    ORANGE_VFS.ops    = &ORANGE_FS_OP;
    ORANGE_VFS.sb     = &ORANGE_SUPERBLOCK;
    ORANGE_VFS.sb_ops = &NULL_SB_OP;

    FAT32_VFS.name   = "/fat0";
    FAT32_VFS.nr_dev = get_next_dev_nr();
    FAT32_VFS.ops    = &FAT32_FS_OP;
    FAT32_VFS.sb     = &FAT32_SUPERBLOCK;
    FAT32_VFS.sb_ops = &NULL_SB_OP;
}

static void init_fs_op_table() {
    TTY_FS_OP.open   = real_open;
    TTY_FS_OP.close  = real_close;
    TTY_FS_OP.write  = real_write;
    TTY_FS_OP.lseek  = real_lseek;
    TTY_FS_OP.unlink = real_unlink;
    TTY_FS_OP.read   = real_read;

    ORANGE_FS_OP.open   = real_open;
    ORANGE_FS_OP.close  = real_close;
    ORANGE_FS_OP.write  = real_write;
    ORANGE_FS_OP.lseek  = real_lseek;
    ORANGE_FS_OP.unlink = real_unlink;
    ORANGE_FS_OP.read   = real_read;

    FAT32_FS_OP.create    = CreateFile;
    FAT32_FS_OP.delete    = DeleteFile;
    FAT32_FS_OP.open      = OpenFile;
    FAT32_FS_OP.close     = CloseFile;
    FAT32_FS_OP.write     = WriteFile;
    FAT32_FS_OP.read      = ReadFile;
    FAT32_FS_OP.opendir   = OpenDir;
    FAT32_FS_OP.createdir = CreateDir;
    FAT32_FS_OP.deletedir = DeleteDir;
}

static void _null_sb_op_read(int unused) {}

static superblock_t *_null_sb_op_get(int unused) {
    return NULL;
}

static void init_sb_op_table() {
    NULL_SB_OP.read = _null_sb_op_read;
    NULL_SB_OP.get  = _null_sb_op_get;

    ORANGE_SB_OP.read = read_orange_superblock;
    ORANGE_SB_OP.get  = get_unique_superblock;
}

void vfs_setup_and_init() {
    //! manually reset dev_nr_counter
    dev_nr_counter = 0;

    //! init fd table
    const int nr_file_desc = sizeof(file_desc_table) / sizeof(file_desc_t);
    memset(file_desc_table, 0, sizeof(file_desc_table));

    //! init superblock table
    const int nr_superblock = sizeof(superblock_table) / sizeof(superblock_t);
    memset(superblock_table, 0, sizeof(superblock_table));
    for (int i = 0; i < nr_superblock; ++i) {
        superblock_table[i].sb_dev  = NO_DEV;
        superblock_table[i].fs_type = NO_FS_TYPE;
    }
    init_superblock_table();

    //! init vfs table
    const int nr_fs = sizeof(vfs_table) / sizeof(vfs_t);
    memset(vfs_table, 0, sizeof(vfs_table));
    for (int i = 0; i < nr_fs; ++i) { vfs_table[i].nr_dev = -1; }
    init_vfs_table();

    //! init fs op table
    const int nr_fs_op = sizeof(fs_op_table) / sizeof(file_op_set_t);
    memset(fs_op_table, 0, sizeof(fs_op_table));
    init_fs_op_table();

    //! init superblock op table
    const int nr_sb_op = sizeof(sb_op_table) / sizeof(superblock_op_set_t);
    memset(sb_op_table, 0, sizeof(sb_op_table));
    init_sb_op_table();
}

int do_vopen(const char *path, int flags) {
    const char *relpath = NULL;
    int         index   = get_vfs_index_and_relpath(path, &relpath);
    if (index == -1) {
        trace_logging("filesystem error: invalid vfs path: %s\n", path);
        return -1;
    }

    if (relpath[0] == '\0') {
        //! a tty file
        relpath = path + 1;
    }

    int fd = vfs_table[index].ops->open(relpath, flags);
    if (fd != -1) {
        p_proc_current->pcb.filp[fd]->dev_index = index;
    } else {
        trace_logging("filesystem error: invalid path: %s\n", path);
    }

    return fd;
}

int do_vclose(int fd) {
    assert(fd != -1 && "invalid fd");
    int index = p_proc_current->pcb.filp[fd]->dev_index;
    assert(index != -1 && "invalid vfs index");
    return vfs_table[index].ops->close(fd);
}

int do_vread(int fd, char *buf, int count) {
    assert(fd != -1 && "invalid fd");
    int index = p_proc_current->pcb.filp[fd]->dev_index;
    assert(index != -1 && "invalid vfs index");
    return vfs_table[index].ops->read(fd, buf, count);
}

int do_vwrite(int fd, const char *buf, int count) {
    assert(fd != -1 && "invalid fd");
    int index = p_proc_current->pcb.filp[fd]->dev_index;
    assert(index != -1 && "invalid vfs index");

    char wrbuf[512] = {};
    int  left       = count;
    while (left > 0) {
        //! copy to kernel buffer
        //! FIXME: really necessary?
        int nbytes = MIN(512, left);
        memcpy(wrbuf, buf, nbytes);
        buf += nbytes;

        int resp = vfs_table[index].ops->write(fd, wrbuf, nbytes);
        if (resp != nbytes) {
            assert(resp < 0);
            return resp;
        }
        left -= nbytes;
    }
    assert(left == 0);

    return count;
}

int do_vunlink(const char *path) {
    const char *relpath = NULL;
    int         index   = get_vfs_index_and_relpath(path, &relpath);
    if (index == -1) {
        trace_logging("filesystem error: invalid vfs path: %s\n", path);
        return -1;
    }
    return vfs_table[index].ops->unlink(relpath);
}

int do_vlseek(int fd, int offset, int whence) {
    assert(fd != -1 && "invalid fd");
    int index = p_proc_current->pcb.filp[fd]->dev_index;
    assert(index != -1 && "invalid vfs index");
    return vfs_table[index].ops->lseek(fd, offset, whence);
}

int do_vcreate(const char *path) {
    const char *relpath = NULL;
    int         index   = get_vfs_index_and_relpath(path, &relpath);
    if (index == -1) {
        trace_logging("filesystem error: invalid vfs path: %s\n", path);
        return -1;
    }
    return vfs_table[index].ops->create(relpath);
}

int do_vdelete(const char *path) {
    const char *relpath = NULL;
    int         index   = get_vfs_index_and_relpath(path, &relpath);
    if (index == -1) {
        trace_logging("filesystem error: invalid vfs path: %s\n", path);
        return -1;
    }
    return vfs_table[index].ops->delete (relpath);
}

int do_vopendir(const char *path) {
    const char *relpath = NULL;
    int         index   = get_vfs_index_and_relpath(path, &relpath);
    if (index == -1) {
        trace_logging("filesystem error: invalid vfs path: %s\n", path);
        return -1;
    }
    return vfs_table[index].ops->opendir(relpath);
}

int do_vcreatedir(const char *path) {
    const char *relpath = NULL;
    int         index   = get_vfs_index_and_relpath(path, &relpath);
    if (index == -1) {
        trace_logging("filesystem error: invalid vfs path: %s\n", path);
        return -1;
    }
    return vfs_table[index].ops->createdir(relpath);
}

int do_vdeletedir(const char *path) {
    const char *relpath = NULL;
    int         index   = get_vfs_index_and_relpath(path, &relpath);
    if (index == -1) {
        trace_logging("filesystem error: invalid vfs path: %s\n", path);
        return -1;
    }
    return vfs_table[index].ops->deletedir(relpath);
}

int do_open(const char *path, int flags) {
    return do_vopen(path, flags);
}

int do_close(int fd) {
    return do_vclose(fd);
}

int do_read(int fd, char *buf, int count) {
    return do_vread(fd, buf, count);
}

int do_write(int fd, const char *buf, int count) {
    return do_vwrite(fd, buf, count);
}

int do_lseek(int fd, int offset, int whence) {
    return do_vlseek(fd, offset, whence);
}

int do_unlink(const char *path) {
    return do_vunlink(path);
}

int do_create(const char *path) {
    return do_vcreate(path);
}

int do_delete(const char *path) {
    return do_vdelete(path);
}

int do_opendir(const char *path) {
    return do_vopendir(path);
}

int do_createdir(const char *path) {
    return do_vcreatedir(path);
}

int do_deletedir(const char *path) {
    return do_vdeletedir(path);
}
