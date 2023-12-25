/**********************************************************
*	vfs.h       //added by mingxuan 2019-5-17
***********************************************************/

//#define NR_DEV 10
#define NR_FS 10		//modified by mingxuan 2020-10-18
#define DEV_NAME_LEN 15
//#define NR_fs 3
#define NR_FS_OP 3		//modified by mingxuan 2020-10-18
#define NR_SB_OP 2		//added by mingxuan 2020-10-30

//#define FILE_MAX_LEN 512*4	//最大长度为4个扇区
#define FILE_MAX_LEN 512*16		//最大长度为16个扇区(8KB)

/* //deleted by mingxuan 2020-10-18
//设备表	
struct device{
    char * dev_name; 			//设备名
    struct file_op * op;          //指向操作表的一项
    int  dev_num;                //设备号
};
*/
// Replace struct device, added by mingxuan 2020-10-18
struct vfs{
    char * fs_name; 			//设备名
    struct file_op * op;        //指向操作表的一项
    //int  dev_num;             //设备号	//deleted by mingxuan 2020-10-29

	struct super_block *sb;		//added by mingxuan 2020-10-29
	struct sb_op *s_op;			//added by mingxuan 2020-10-29
};

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

int do_vopen(const char *path, int flags);
int do_vclose(int fd);
int do_vread(int fd, char *buf, int count);
int do_vwrite(int fd, const char *buf, int count);
int do_vunlink(const char *path);
int do_vlseek(int fd, int offset, int whence);
int do_vcreate(char *pathname);
int do_vdelete(char *path);
int do_vopendir(char *dirname);
int do_vcreatedir(char *dirname);
int do_vdeletedir(char *dirname);

void init_vfs();
void init_file_desc_table();
void init_fileop_table();

int sys_CreateFile(void *uesp);
int sys_DeleteFile(void *uesp);
int sys_OpenFile(void *uesp);
int sys_CloseFile(void *uesp);
int sys_WriteFile(void *uesp);
int sys_ReadFile(void *uesp);
int sys_OpenDir(void *uesp);
int sys_CreateDir(void *uesp);
int sys_DeleteDir(void *uesp);
int sys_ListDir(void *uesp);

//文件系统的操作函数
struct file_op{
    int (*create)   (const char*);
	int (*open)    (const char* ,int);
	int (*close)   (int);
	int (*read)    (int,void * ,int);
	int (*write)   (int ,const void* ,int);
	int (*lseek)   (int ,int ,int);
	int (*unlink)  (const char*);
    int (*delete) (const char*);
	int (*opendir) (const char *);
	int (*createdir) (const char *);
	int (*deletedir) (const char *);
};

//added by mingxuan 2020-10-29
struct sb_op{
	void (*read_super_block) (int);
	struct super_block* (*get_super_block) (int);
};