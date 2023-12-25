/**********************************************************
*	vfs.c       //added by mingxuan 2019-5-17
***********************************************************/

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
#include "vfs.h"
#include "fat32.h"
#include "stdio.h"

//static struct device  device_table[NR_DEV];  //deleted by mingxuan 2020-10-18
static struct vfs  vfs_table[NR_FS];   //modified by mingxuan 2020-10-18

struct file_desc f_desc_table[NR_FILE_DESC];
struct super_block super_block[NR_SUPER_BLOCK]; //added by mingxuan 2020-10-30

//static struct file_op f_op_table[NR_fs]; //文件系统操作表
static struct file_op f_op_table[NR_FS_OP]; //modified by mingxuan 2020-10-18
static struct sb_op   sb_op_table[NR_SB_OP];   //added by mingxuan 2020-10-30

//static void init_dev_table();//deleted by mingxuan 2020-10-30
static void init_vfs_table();  //modified by mingxuan 2020-10-30
void init_file_desc_table();   //added by mingxuan 2020-10-30
void init_fileop_table();
void init_super_block_table();  //added by mingxuan 2020-10-30
void init_sb_op_table();

static int get_index(char path[]);

void init_vfs()
{

    init_file_desc_table();
    init_fileop_table();
  
    init_super_block_table();
    init_sb_op_table(); //added by mingxuan 2020-10-30

    //init_dev_table(); //deleted by mingxuan 2020-10-30
    init_vfs_table();   //modified by mingxuan 2020-10-30
}

//added by mingxuan 2020-10-30
void init_file_desc_table()
{
    int i;
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));
}

void init_fileop_table()
{
    // table[0] for tty 
    f_op_table[0].open = real_open;
    f_op_table[0].close = real_close;
    f_op_table[0].write = real_write;
    f_op_table[0].lseek = real_lseek;
    f_op_table[0].unlink = real_unlink;
    f_op_table[0].read = real_read;

    // table[1] for orange 
    f_op_table[1].open = real_open;
    f_op_table[1].close = real_close;
    f_op_table[1].write = real_write;
    f_op_table[1].lseek = real_lseek;
    f_op_table[1].unlink = real_unlink;
    f_op_table[1].read = real_read;

    // table[2] for fat32
    f_op_table[2].create = CreateFile;
    f_op_table[2].delete = DeleteFile;
    f_op_table[2].open = OpenFile;
    f_op_table[2].close = CloseFile;
    f_op_table[2].write = WriteFile;
    f_op_table[2].read = ReadFile;
    f_op_table[2].opendir = OpenDir;
    f_op_table[2].createdir = CreateDir;
    f_op_table[2].deletedir = DeleteDir;

}

//added by mingxuan 2020-10-30
void init_super_block_table(){
    struct super_block * sb = super_block;						//deleted by mingxuan 2020-10-30

    //super_block[0] is tty0, super_block[1] is tty1, uper_block[2] is tty2
    for(; sb < &super_block[3]; sb++) {   
        sb->sb_dev =  DEV_CHAR_TTY;
        sb->fs_type = TTY_FS_TYPE;
    }

    //super_block[3] is orange's superblock
    sb->sb_dev = DEV_HD;
    sb->fs_type = ORANGE_TYPE; 
    sb++;

    //super_block[4] is fat32's superblock
    sb->sb_dev = DEV_HD;
    sb->fs_type = FAT32_TYPE; 
    sb++;

    //another super_block are free
    for (; sb < &super_block[NR_SUPER_BLOCK]; sb++) {
	sb->sb_dev = NO_DEV;
        sb->fs_type = NO_FS_TYPE;
    } 
}

//added by mingxuan 2020-10-30
void init_sb_op_table(){
    //orange
    sb_op_table[0].read_super_block = read_super_block;
    sb_op_table[0].get_super_block = get_super_block;

    //fat32 and tty
    sb_op_table[1].read_super_block = NULL;
    sb_op_table[1].get_super_block = NULL;
}

//static void init_dev_table(){
static void init_vfs_table(){  // modified by mingxuan 2020-10-30

    // 我们假设每个tty就是一个文件系统
    // tty0
    // device_table[0].dev_name="dev_tty0";
    // device_table[0].op = &f_op_table[0];
    memset(vfs_table, 0, sizeof(vfs_table));
    vfs_table[0].fs_name = "dev_tty0"; //modifed by mingxuan 2020-10-18
    vfs_table[0].op = &f_op_table[0];
    vfs_table[0].sb = &super_block[0];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[0].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30

    // tty1
    //device_table[1].dev_name="dev_tty1";
    //device_table[1].op =&f_op_table[0];
    vfs_table[1].fs_name = "dev_tty1"; //modifed by mingxuan 2020-10-18
    vfs_table[1].op = &f_op_table[0];
    vfs_table[1].sb = &super_block[1];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[1].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30

    // tty2
    //device_table[2].dev_name="dev_tty2";
    //device_table[2].op=&f_op_table[0];
    vfs_table[2].fs_name = "dev_tty2"; //modifed by mingxuan 2020-10-18
    vfs_table[2].op = &f_op_table[0];
    vfs_table[2].sb = &super_block[2];  //每个tty都有一个superblock //added by mingxuan 2020-10-30
    vfs_table[2].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30

    // fat32
    //device_table[3].dev_name="fat0";
    //device_table[3].op=&f_op_table[2];
    vfs_table[3].fs_name = "fat0"; //modifed by mingxuan 2020-10-18
    vfs_table[3].op = &f_op_table[2];
    vfs_table[3].sb = &super_block[4];      //added by mingxuan 2020-10-30
    vfs_table[3].s_op = &sb_op_table[1];    //added by mingxuan 2020-10-30

    // orange
    //device_table[4].dev_name="orange";
    //device_table[4].op=&f_op_table[1];
    vfs_table[4].fs_name = "orange"; //modifed by mingxuan 2020-10-18
    vfs_table[4].op = &f_op_table[1];
    vfs_table[4].sb = &super_block[3];      //added by mingxuan 2020-10-30
    vfs_table[4].s_op = &sb_op_table[0];    //added by mingxuan 2020-10-30

}

static int get_index(char path[]){

    int pathlen = strlen(path);
    //char dev_name[DEV_NAME_LEN];
    char fs_name[DEV_NAME_LEN];   //modified by mingxuan 2020-10-18
    int len = (pathlen < DEV_NAME_LEN) ? pathlen : DEV_NAME_LEN;
    
    int i,a=0;
    for(i=0;i<len;i++){
        if( path[i] == '/'){
            a=i;
            a++;
            break;
        }
        else {
            //dev_name[i] = path[i];
            fs_name[i] = path[i];   //modified by mingxuan 2020-10-18
        }
    }
    //dev_name[i] = '\0';
    fs_name[i] = '\0';  //modified by mingxuan 2020-10-18
    for(i=0;i<pathlen-a;i++)
        path[i] = path[i+a];
    path[pathlen-a] = '\0';

    //for(i=0;i<NR_DEV;i++)
    for(i=0;i<NR_FS;i++)    //modified by mingxuan 2020-10-29
    {
        // if(!strcmp(dev_name, device_table[i].dev_name))
        if(vfs_table[i].fs_name == NULL) continue;
        if(!strcmp(fs_name, vfs_table[i].fs_name)) //modified by mingxuan 2020-10-18
            return i;
    }

    return -1;
}


/*======================================================================*
                              sys_* 系列函数
 *======================================================================*/

int sys_open(void *uesp)
{
    return do_vopen((const char *)get_arg(uesp, 1), get_arg(uesp, 2));
}

int sys_close(void *uesp)
{
    return do_vclose(get_arg(uesp, 1));
}

int sys_read(void *uesp)
{
    return do_vread(get_arg(uesp, 1), (char *)get_arg(uesp, 2), get_arg(uesp, 3));
}

int sys_write(void *uesp)
{
    return do_vwrite(get_arg(uesp, 1), (const char *)get_arg(uesp, 2), get_arg(uesp, 3));
}

int sys_lseek(void *uesp)
{
    return do_vlseek(get_arg(uesp, 1), get_arg(uesp, 2), get_arg(uesp, 3));
}

int sys_unlink(void *uesp) {
    return do_vunlink((const char *)get_arg(uesp, 1));
}

int sys_create(void *uesp) {
    return do_vcreate((char *)get_arg(uesp, 1));
}

int sys_delete(void *uesp) {
    return do_vdelete((char *)get_arg(uesp, 1));
}

int sys_opendir(void *uesp) {
    return do_vopendir((char *)get_arg(uesp, 1));
}

int sys_createdir(void *uesp) {
    return do_vcreatedir((char *)get_arg(uesp, 1));
}

int sys_deletedir(void *uesp) {
    return do_vdeletedir((char *)get_arg(uesp, 1));
}


/*======================================================================*
                              do_v* 系列函数
 *======================================================================*/

int do_vopen(const char *path, int flags) {

    int pathlen = strlen(path);
    char pathname[MAX_PATH];
    
    strcpy(pathname,(char *)path);
    pathname[pathlen] = 0;

    int index;
    int fd = -1;
    index = get_index(pathname);
    if(index == -1){
        kprintf("pathname error! path: %s\n", path);
        return -1;
    }

    fd = vfs_table[index].op->open(pathname, flags);    //modified by mingxuan 2020-10-18
    if(fd != -1)
    {
        p_proc_current -> task.filp[fd] -> dev_index = index;
    } else {
        kprintf("          error!\n");
    }
                   
    return fd;    
}


int do_vclose(int fd) {
    int index = p_proc_current->task.filp[fd]->dev_index;
    return vfs_table[index].op->close(fd);  //modified by mingxuan 2020-10-18
}

int do_vread(int fd, char *buf, int count) {
    int index = p_proc_current->task.filp[fd]->dev_index;
    return vfs_table[index].op->read(fd, buf, count);   //modified by mingxuan 2020-10-18
}

int do_vwrite(int fd, const char *buf, int count) {
    //modified by mingxuan 2019-5-23
    char s[512];
    int index = p_proc_current->task.filp[fd]->dev_index;
    const char *fsbuf = buf;
    int f_len = count;
    int bytes;
    while(f_len)
    {
        int iobytes = min(512, f_len);
        int i=0;
        for(i=0; i<iobytes; i++)
        {
            s[i] = *fsbuf;
            fsbuf++;
        }
        //bytes = device_table[index].op->write(fd,s,iobytes);
        bytes = vfs_table[index].op->write(fd,s,iobytes);   //modified by mingxuan 2020-10-18
        if(bytes != iobytes)
        {
            return bytes;
        }
        f_len -= bytes;
    }
    return count;
}

int do_vunlink(const char *path) {
    int pathlen = strlen(path);
    char pathname[MAX_PATH];
    
    strcpy(pathname,(char *)path);
    pathname[pathlen] = 0;

    int index;
    index = get_index(pathname);
    if(index==-1){
        kprintf("pathname error!\n");
        return -1;
    }
    
    //return device_table[index].op->unlink(pathname);
    return vfs_table[index].op->unlink(pathname);   //modified by mingxuan 2020-10-18
}

int do_vlseek(int fd, int offset, int whence) {
    int index = p_proc_current->task.filp[fd]->dev_index;

    //return device_table[index].op->lseek(fd, offset, whence);
    return vfs_table[index].op->lseek(fd, offset, whence);  //modified by mingxuan 2020-10-18

}

//int do_vcreate(char *pathname) {
int do_vcreate(char *filepath) { //modified by mingxuan 2019-5-17
    //added by mingxuan 2019-5-17  
    int state;
    const char *path = filepath;

    int pathlen = strlen(path);
    char pathname[MAX_PATH];
    
    strcpy(pathname,(char *)path);
    pathname[pathlen] = 0;

    int index;
    index = get_index(pathname);
    if(index == -1){
        kprintf("pathname error! path: %s\n", path);
        return -1;
    }
    state = vfs_table[index].op->create(pathname); //modified by mingxuan 2020-10-18
    if (state == 1) {
        kprintf("          create file success!");
    } else {
		DisErrorInfo(state);
    }
    return state;
}

int do_vdelete(char *path) {

    int pathlen = strlen(path);
    char pathname[MAX_PATH];
    
    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    index = get_index(pathname);
    if(index==-1){
        kprintf("pathname error!\n");
        return -1;
    }
    //return device_table[index].op->delete(pathname);
    return vfs_table[index].op->delete(pathname);   //modified by mingxuan 2020-10-18
}
int do_vopendir(char *path) {
    int state;

    int pathlen = strlen(path);
    char pathname[MAX_PATH];
    
    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    index = (int)(pathname[1]-'0');

    for(int j=0;j<= pathlen-3;j++)
    {
        pathname[j] = pathname[j+3];
    }
    state = f_op_table[index].opendir(pathname);
    if (state == 1) {
        kprintf("          open dir success!");
    } else {
		DisErrorInfo(state);
    }    
    return state;
}

int do_vcreatedir(char *path) {
    int state;

    int pathlen = strlen(path);
    char pathname[MAX_PATH];
    
    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    index = (int)(pathname[1]-'0');

    for(int j=0;j<= pathlen-3;j++)
    {
        pathname[j] = pathname[j+3];
    }
    state = f_op_table[index].createdir(pathname);
    if (state == 1) {
        kprintf("          create dir success!");
    } else {
		DisErrorInfo(state);
    }    
    return state;
}

int do_vdeletedir(char *path) {
    int state;
    int pathlen = strlen(path);
    char pathname[MAX_PATH];
    
    strcpy(pathname,path);
    pathname[pathlen] = 0;

    int index;
    index = (int)(pathname[1]-'0');

    for(int j=0;j<= pathlen-3;j++)
    {
        pathname[j] = pathname[j+3];
    }
    state = f_op_table[index].deletedir(pathname);
    if (state == 1) {
        kprintf("          delete dir success!");
    } else {
		DisErrorInfo(state);
    }   
    return state;
}