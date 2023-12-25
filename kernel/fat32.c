/**********************************************************
*	fat32.c       //added by mingxuan 2019-5-17
***********************************************************/

#include "fat32.h"
#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "fs_misc.h"
#include "string.h"
#include "stdio.h"

extern DWORD FAT_END;
extern DWORD TotalSectors;
extern WORD Bytes_Per_Sector;
extern BYTE Sectors_Per_Cluster;
extern WORD Reserved_Sector;
extern DWORD Sectors_Per_FAT;
extern UINT Position_Of_RootDir;
extern UINT Position_Of_FAT1;
extern UINT Position_Of_FAT2;
extern struct file_desc f_desc_table[NR_FILE_DESC];
CHAR VDiskPath[256]={0};
CHAR cur_path[256]={0};
u8* buf;
STATE state;
File f_desc_table_fat[NR_FILE_DESC];

static void load_disk();
static void mkfs_fat();

STATE DeleteDir(const char *dirname)
{
	CHAR fullpath[256]={0};
	CHAR parent[256]={0};
	CHAR name[256]={0};
	DWORD parentCluster=0,startCluster=0;
	UINT tag=0;
	STATE state;
	
	ToFullPath((PCHAR)dirname,fullpath);
	GetParentFromPath(fullpath,parent);
	GetNameFromPath(fullpath,name);
	
	state=IsFile(fullpath,&tag);
	if(state!=OK)
	{
		return state;
	}
	if(tag==F)
	{
		return WRONGPATH;
	}
	state=PathToCluster(parent,&parentCluster);
	if(state!=OK)
	{
		return state;
	}
	state=ClearRecord(parentCluster,name,&startCluster);
	if(state!=OK)
	{
		return state;
	}
	DeleteAllRecord(startCluster);
	return OK;
}

STATE CreateDir(const char *dirname)
{
	Record record;
	CHAR fullname[256]={0};
	CHAR name[256]={0};
	CHAR parent[256]={0};
	DWORD parentCluster=0,startCluster=0,sectorIndex=0,off_in_sector=0;
	STATE state;
	
	ToFullPath((PCHAR)dirname,fullname);
	GetNameFromPath(fullname,name);
	GetParentFromPath(fullname,parent);
	state=PathToCluster(parent,&parentCluster);
	if(state!=OK)
	{
		return state;//找不到路径
	}
	state=FindSpaceInDir(parentCluster,name,&sectorIndex,&off_in_sector);
	if(state!=OK)
	{
		return state;//虚拟磁盘空间不足
	}
	state=FindClusterForDir(&startCluster);
	if(state!=OK)
	{
		return state;//虚拟磁盘空间不足
	}
	CreateRecord(name,0x10,startCluster,0,&record);
	WriteRecord(record,sectorIndex,off_in_sector);
	WriteFAT(1,&startCluster);//写FAT
	CreateRecord(".",0x10,startCluster,0,&record);//准备目录项.的数据
	sectorIndex=Reserved_Sector+2*Sectors_Per_FAT+(startCluster-2)*Sectors_Per_Cluster;
	WriteRecord(record,sectorIndex,0);//写.目录项
	CreateRecord("..",0x10,parentCluster,0,&record);//准备目录项..的数据
	WriteRecord(record,sectorIndex,sizeof(Record));//写..目录项
	//fflush(fp);
	return OK;
}

STATE OpenDir(const char* dirname)
{
	DWORD parentCluster=0;
	CHAR fullpath[256]={0},parent[256]={0},name[256]={0};
	Record record;
	STATE state;
	
	if(strcmp(dirname,".")==0)
	{
		return OK;
	}else if(strcmp(dirname,"..")==0||strcmp(dirname,"\\")==0){
		ChangeCurrentPath((PCHAR)dirname);
		return OK;
	}else{	
		if(IsFullPath((PCHAR)dirname))
		{
			strcpy(fullpath,(PCHAR)dirname);
			GetParentFromPath(fullpath,parent);
			if(strlen(parent)==0)//说明dirname是根目录
			{
				memset(cur_path,0,sizeof(cur_path));
				strcpy(cur_path,fullpath);
				cur_path[strlen(cur_path)]='\\';
				return OK;
			}
			GetNameFromPath(fullpath,name);
		}else{
			MakeFullPath(cur_path,(PCHAR)dirname,fullpath);
			strcpy(parent,cur_path);
			strcpy(name,(PCHAR)dirname);
		}
		state=PathToCluster(parent,&parentCluster);
		if(state!=OK)
		{
			return state;
		}
		state=ReadRecord(parentCluster,name,&record,NULL,NULL);
		if(state!=OK)
		{
			return state;
		}
		if(record.proByte==(BYTE)0x10)
		{
			strcpy(cur_path,fullpath);
			return OK;
		}else{
			return WRONGPATH;
		}
	}
	return OK;
}


STATE ReadFile(int fd,void *buf, int length)
{
	int size = 0;
	PBYTE sector=NULL;
	DWORD curSectorIndex=0,nextSectorIndex=0,off_in_sector=0,free_in_sector=0,readsize=0;
	UINT isLastSector=0,tag=0;
	PFile pfile = p_proc_current->task.filp[fd] ->fd_node.fd_file;

	if(pfile->flag!=R && pfile->flag!=RW && pfile->flag!=(RW|C)) //modified by mingxuan 2019-5-18
	{
		return ACCESSDENIED;
	}
	
	kprintf("read:");
	if(pfile->off>=pfile->size)
	{
		return 0;
	}
	sector = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));
	if(sector==NULL)
	{
		return SYSERROR;
	}
	GetFileOffset(pfile,&curSectorIndex,&off_in_sector,&isLastSector);
	do 
	{	
		if(isLastSector)//当前的扇区是该文件的最后一个扇区
		{
			if(pfile->size%Bytes_Per_Sector==0)
			{
				free_in_sector=Bytes_Per_Sector-off_in_sector;
			}else{
				free_in_sector=pfile->size%Bytes_Per_Sector-off_in_sector;//最后一个扇区的剩余量
			}
			tag=1;//置跳出标志
		}else{
			free_in_sector=Bytes_Per_Sector-off_in_sector;//本扇区的剩余量
		}
		if(free_in_sector<length-(size))//缓冲区装不满
		{
			readsize=free_in_sector;
		}else{//缓冲区能装满
			readsize=length-(size);
			tag=1;//置跳出标志
		}
		ReadSector(sector,curSectorIndex);
		memcpy(buf+(size),sector+off_in_sector,readsize);
		(size)+=readsize;
		pfile->off+=readsize;
		if(tag==1)//最后一个扇区或缓冲区装满了
		{
			break;
		}else{//缓冲区还没装满并且还没到最后一个扇区
			GetNextSector(pfile,curSectorIndex,&nextSectorIndex,&isLastSector);
			curSectorIndex=nextSectorIndex;
			off_in_sector=0;
		}
	}while(1);
	sys_free(sector);
	//pfile->off = 0;
	return size;
}

STATE WriteFile(int fd, const void *buf, int length)
{
	
	PBYTE sector=NULL;
	DWORD clusterNum=0;
	DWORD curSectorIndex=0,nextSectorIndex=0,off_in_sector=0,free_in_sector=0,off_in_buf=0;
	UINT isLastSector=0;
	STATE state;
	PFile pfile = p_proc_current->task.filp[fd] ->fd_node.fd_file;
	//PFile pfile = &f_desc_table_fat[0];

	//if(pfile->flag!=W) //deleted by mingxuan 2019-5-18
	if(pfile->flag!=W && pfile->flag!=RW && pfile->flag!=(RW|C) ) //modified by mingxuan 2019-5-18
	{
		return ACCESSDENIED;
	}

	sector = (PBYTE)K_PHY2LIN(sys_kmalloc(Bytes_Per_Sector*sizeof(BYTE)));
	if(sector==NULL)
	{
		return SYSERROR;
	}
	if(pfile->start==0)//此文件是个空文件原来没有分配簇
	{
		state=AllotClustersForEmptyFile(pfile,length);//空间不足无法分配
		if(state!=OK)
		{
			sys_free(sector);
			return state;//虚拟磁盘空间不足
		}
	}else{
		if(NeedMoreCluster(pfile,length,&clusterNum))
		{
			state=AddCluster(pfile->start,clusterNum);//空间不足
			if(state!=OK)
			{
				sys_free(sector);
				return state;//虚拟磁盘空间不足
			}
		}
	}
	GetFileOffset(pfile,&curSectorIndex,&off_in_sector,&isLastSector);
	free_in_sector=Bytes_Per_Sector-off_in_sector;
	while(free_in_sector<length-off_in_buf)//当前扇区的空闲空间放不下本次要写入的内容
	{
		ReadSector(sector,curSectorIndex);
		memcpy(sector+off_in_sector,(void *)buf+off_in_buf,free_in_sector);
		WriteSector(sector,curSectorIndex);
		off_in_buf+=free_in_sector;
		pfile->off+=free_in_sector;
		GetNextSector(pfile,curSectorIndex,&nextSectorIndex,&isLastSector);
		curSectorIndex=nextSectorIndex;
		free_in_sector=Bytes_Per_Sector;
		off_in_sector=0;
	}
	ReadSector(sector,curSectorIndex);
	memcpy(sector+off_in_sector,(void *)buf+off_in_buf,length-off_in_buf);
	WriteSector(sector,curSectorIndex);
	pfile->off+=length-off_in_buf;
	sys_free(sector);
	//fflush(fp);
	return OK;
}

STATE CloseFile(int fd)
{
	PFile pfile;
	pfile = p_proc_current->task.filp[fd] ->fd_node.fd_file;
	DWORD curSectorIndex=0,curClusterIndex=0,nextClusterIndex=0,parentCluster=0;
	UINT isLastSector=0;
	Record record;
	DWORD sectorIndex=0,off_in_sector=0;

	//p_proc_current->task.filp_fat[fd] = 0;
	f_desc_table_fat[fd].flag = 0;
	p_proc_current->task.filp[fd]->flag = 0;
	p_proc_current->task.filp[fd] = 0;
	if(pfile->flag==R)
	{
		return OK;
	}else{
		if(pfile->off<pfile->size)
		{
			GetFileOffset(pfile,&curSectorIndex,NULL,&isLastSector);
			if(isLastSector==0)
			{
				curSectorIndex=(curClusterIndex-Reserved_Sector-2*Sectors_Per_FAT)/Sectors_Per_Cluster+2;
				GetNextCluster(curClusterIndex,&nextClusterIndex);
				if(nextClusterIndex!=FAT_END)//说明当前簇不是此文件的最后一簇
				{
					WriteFAT(1,&curClusterIndex);//把当前簇设置为此文件的最后一簇
					ClearFATs(nextClusterIndex);//清除此文件的多余簇
				}
			}
		}
		PathToCluster(pfile->parent,&parentCluster);
		ReadRecord(parentCluster,pfile->name,&record,&sectorIndex,&off_in_sector);
		record.highClusterNum=(WORD)(pfile->start>>16);
		record.lowClusterNum=(WORD)(pfile->start&0x0000FFFF);
		record.filelength=pfile->off;
		WriteRecord(record,sectorIndex,off_in_sector);
	}
	return OK;
}

STATE OpenFile(const char *filename,int mode)
{

	CHAR fullpath[256]={0};
	CHAR parent[256]={0};
	CHAR name[256]={0};
	Record record;
	DWORD parentCluster;
	STATE state;

	DWORD sectorIndex=0,off_in_sector=0; //added by mingxuan 2019-5-19

	ToFullPath((PCHAR)filename,fullpath);
	GetParentFromPath(fullpath,parent);
	GetNameFromPath(fullpath,name);

	state=PathToCluster(parent,&parentCluster);
	kprintf("\nstate=");
	kprintf("%d", state);
	if(state!=OK)
	{
		return -1;
	}
	
	//added by mingxuan 2019-5-19
	state=FindSpaceInDir(parentCluster,name,&sectorIndex,&off_in_sector); //检测文件名是否存在
	if(mode & O_CREAT) //如果用户使用了O_CREAT
	{
		if(state == NAMEEXIST) //文件存在，使用O_CREAT是多余的，继续执行OpenFile即可
		{
			kprintf("file exists, O_CREAT is no use!");
		}
		else //文件不存在，需要使用O_CREAT，先创建文件，再执行OpenFile
		{
			CreateRecord(name,0x20,0,0,&record);
			WriteRecord(record,sectorIndex,off_in_sector);//写目录项
		}
	}
	else //用户没有使用O_CREAT
	{
		if(state != NAMEEXIST) //文件不存在，需要使用O_CREAT，用户没有使用，则报错并返回-1，表示路径有误
		{
			kprintf("no file, use O_CREAT!");
			return -1;
		}
		else{} //文件存在，使用O_CREAT是多余的，继续执行OpenFile即可
	}
	//~mingxuan 2019-5-19

	state=ReadRecord(parentCluster,name,&record,NULL,NULL);
	kprintf("state=");
	kprintf("%d", state);
	if(state!=OK)
	{
		kprintf("ReadRecord Fail!");
		return -1;
	}
	
	int i;
	int fd = -1;
	for (i = 3; i < NR_FILES; i++) {
		if (p_proc_current->task.filp[i] == 0) {
			fd = i;
			break;
		}
	}

    if ((fd < 0) || (fd >= NR_FILES)) {
		// panic("filp[] is full (PID:%d)", proc2pid(p_proc_current));
		kprintf("filp[] is full (PID:");
		kprintf("%d", proc2pid(p_proc_current));
		kprintf(")\n");
		return -1;
    }

	//找一个未用的文件描述符
	for (i = 0; i < NR_FILE_DESC; i++)
		if ((f_desc_table[i].flag == 0))
			break;
	if (i >= NR_FILE_DESC) {
		kprintf("f_desc_table[] is full (PID:");
		kprintf("%d", proc2pid(p_proc_current));
		kprintf(")\n");
		return -1;
	}
	
	p_proc_current->task.filp[fd] = &f_desc_table[i];
	f_desc_table[i].flag = 1;
	
	//找一个未用的FILE
	for (i = 0; i < NR_FILE_DESC; i++)
		if (f_desc_table_fat[i].flag == 0)
			break;
	if (i >= NR_FILE_DESC) {
		kprintf("f_desc_table[] is full (PID:");
		kprintf("%d", proc2pid(p_proc_current));
		kprintf(")\n");
	}

	//以下是给File结构体赋值
	memset(f_desc_table_fat[i].parent,0,sizeof(f_desc_table_fat[i].parent));//初始化parent字段
	memset(f_desc_table_fat[i].name,0,sizeof(f_desc_table_fat[i].name));//初始化name字段
	strcpy(f_desc_table_fat[i].parent,parent);
	strcpy(f_desc_table_fat[i].name,name);
	f_desc_table_fat[i].start=(record.highClusterNum<<16)+record.lowClusterNum;
	f_desc_table_fat[i].off=0;
	f_desc_table_fat[i].size=record.filelength;
	f_desc_table_fat[i].flag=mode;
	// kprintf("flag:");
	// deint(f_desc_table_fat[i].flag);
	// kprintf("index:");
	// deint(i);
	p_proc_current->task.filp[fd] ->fd_node.fd_file = &f_desc_table_fat[i];
	
	return fd;
}

STATE CreateFile(const char *filename)
{
	Record record;
	CHAR fullpath[256]={0};
	CHAR parent[256]={0};
	CHAR name[256]={0};
	DWORD parentCluster=0,sectorIndex=0,off_in_sector=0;
	STATE state;
	
	ToFullPath((PCHAR)filename,fullpath);
	GetParentFromPath(fullpath,parent);
	GetNameFromPath((PCHAR)filename,name);
	state=PathToCluster(parent,&parentCluster);
	if(state!=OK)
	{
		return state;//找不到路径
	}

	state=FindSpaceInDir(parentCluster,name,&sectorIndex,&off_in_sector);
	if(state != OK) {
		return state;
	}

	CreateRecord(name,0x20,0,0,&record);
	WriteRecord(record,sectorIndex,off_in_sector);//写目录项
	return OK;
}

STATE DeleteFile(const char *filename)
{
	CHAR fullpath[256]={0};
	CHAR parent[256]={0};
	CHAR name[256]={0};
	DWORD parentCluster=0;
	DWORD startCluster=0;
	UINT tag=0;
	STATE state;
	
	ToFullPath((PCHAR)filename,fullpath);
	GetParentFromPath(fullpath,parent);
	GetNameFromPath(fullpath,name);
	
	state=IsFile(fullpath,&tag);
	if(state!=OK)
	{
		return state;
	}
	if(tag==D)
	{
		return WRONGPATH;
	}
	state=PathToCluster(parent,&parentCluster);
	if(state!=OK)
	{
		return state;
	}
	state=ClearRecord(parentCluster,name,&startCluster);
	if(state!=OK)
	{
		return state;
	}
	if(startCluster!=0)
	{
		ClearFATs(startCluster);
	}
	return OK;
}

STATE IsFile(PCHAR path,PUINT tag)
{
	CHAR fullpath[256]={0};
	CHAR parent[256]={0};
	CHAR name[256]={0};
	DWORD parentCluster=0;
	Record record;
	STATE state;
	
	ToFullPath(path,fullpath);
	GetParentFromPath(fullpath,parent);
	GetNameFromPath(fullpath,name);	
	state=PathToCluster(parent,&parentCluster);
	if(state!=OK)
	{
		return state;//找不到路径
	}
	state=ReadRecord(parentCluster,name,&record,NULL,NULL);
	if(state!=OK)
	{
		return state;//找不到路径
	}
	if(record.proByte==0x10)
	{
		*tag=D;
	}else{
		*tag=F;
	}
	return OK;
}

void init_fs_fat() 
{
    kprintf("Initializing fat32 file system...  \n");
	
	buf = (u8*)K_PHY2LIN(sys_kmalloc(FSBUF_SIZE));

    int fat32_dev = get_fs_dev(PRIMARY_MASTER, FAT32_TYPE);	//added by mingxuan 2020-10-27

	//load_disk(FAT_DEV);	// deleted by mingxuan 2020-10-27
	load_disk(fat32_dev);	// modified by mingxuan 2020-10-27
    if (TotalSectors == 0) {
        mkfs_fat();
    	//load_disk(FAT_DEV);	//deleted by mingxuan 2020-10-27
		load_disk(fat32_dev);	//modified by mingxuan 2020-10-27
    }
	int i;
	for (i = 0; i < NR_FILE_DESC; ++i) {
		f_desc_table_fat[i].flag = 0;
	}
}

static void load_disk(int dev) {
	MESSAGE driver_msg;
	PCHAR cur="V:\\";

	driver_msg.type		= DEV_READ;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= SECTOR_SIZE * 1;
	driver_msg.BUF		= buf;
	driver_msg.CNT		= SECTOR_SIZE;
	driver_msg.PROC_NR	= proc2pid(p_proc_current);///TASK_A

	hd_rdwt(&driver_msg);

    memcpy(&Bytes_Per_Sector,buf+11,2);
	memcpy(&Sectors_Per_Cluster,buf+13,1);
	memcpy(&Reserved_Sector,buf+14,2);
	memcpy(&TotalSectors,buf+32,4);
	memcpy(&Sectors_Per_FAT,buf+36,4);
	Position_Of_RootDir=(Reserved_Sector+Sectors_Per_FAT*2)*Bytes_Per_Sector;
	Position_Of_FAT1=Reserved_Sector*Bytes_Per_Sector;
	Position_Of_FAT2=(Reserved_Sector+Sectors_Per_FAT)*Bytes_Per_Sector;
	strcpy(cur_path,cur);
}

static void mkfs_fat() {
    MESSAGE driver_msg;

	int fat32_dev = get_fs_dev(PRIMARY_MASTER, FAT32_TYPE);	//added by mingxuan 2020-10-27

	/* get the geometry of ROOTDEV */
	struct part_info geo;
	driver_msg.type		= DEV_IOCTL;
	//driver_msg.DEVICE	= MINOR(FAT_DEV);	//deleted by mingxuan 2020-10-27
	driver_msg.DEVICE	= MINOR(fat32_dev);	//modified by mingxuan 2020-10-27

	driver_msg.REQUEST	= DIOCTL_GET_GEO;
	driver_msg.BUF		= &geo;
	driver_msg.PROC_NR	= proc2pid(p_proc_current);
	hd_ioctl(&driver_msg);

	kprintf("dev size: ");
	kprintf("%d", geo.size);
	kprintf(" sectors\n");

    TotalSectors = geo.size;

	DWORD jump=0x009058eb;//跳转指令：占3个字节
	DWORD oem[2]={0x4f44534d,0x302e3553};//厂商标志，OS版本号:占8个字节
	//以下是BPB的内容
	WORD bytes_per_sector=512;//每扇区字节数：占2个字节
	WORD sectors_per_cluster=8;//每簇扇区数：占1个字节
	WORD reserved_sector=32;//保留扇区数：占2个字节
	WORD number_of_FAT=2;//FAT数：占1个字节
	BYTE mediaDescriptor=0xF8;
	DWORD sectors_per_FAT=(TotalSectors*512-8192)/525312+1;//每FAT所占扇区数，用此公式可以算出来：占4个字节
	DWORD root_cluster_number=2;//根目录簇号：占4个字节
	//以下是扩展BPB内容
	CHAR volumeLabel[11]={'N','O',' ','N','A','M','E',' ',' ',' ',' '};//卷标：占11个字节
	CHAR systemID[8]={'F','A','T','3','2',' ',' ',' '};//系统ID，FAT32系统中一般取为“FAT32”：占8个字节
	//以下是有效结束标志
	DWORD end=0xaa55;
	
	DWORD media_descriptor[2]={0x0ffffff8,0xffffffff};//FAT介质描述符
	DWORD cluster_tag=0x0fffffff;//文件簇的结束单元标记
	
	Record vLabel;//卷标的记录项。
	// DWORD clearSize=sectors_per_cluster*bytes_per_sector-sizeof(Record);
	char volumelabel[3] = "MZY";

	memcpy(buf,&jump,3);//写入跳转指令:占3个字节(其实没有用)
	memcpy(buf+3,oem,8);//厂商标志，OS版本号:占8个字节
	//以下是写 BPB
	memcpy(buf+11,&bytes_per_sector,2);//每扇区字节数：占2个字节
	memcpy(buf+13,&sectors_per_cluster,1);//写入每簇扇区数：占1个字节
	memcpy(buf+14,&reserved_sector,2);//写入保留扇区数：占2个字节
	memcpy(buf+16,&number_of_FAT,1);//写入FAT数：占1个字节
	memcpy(buf+21,&mediaDescriptor,1);//写入媒体描述符
	memcpy(buf+32,&TotalSectors,4);//写入总扇区数
	memcpy(buf+36,&sectors_per_FAT,4);//写入每FAT所占扇区数：占4个字节
	memcpy(buf+44,&root_cluster_number,4);//写入根目录簇号：占4个字节
	//以下是写 扩展BPB
	memcpy(buf+71,volumeLabel,11);//写卷标：占11个字节
	memcpy(buf+82,systemID,8);//系统ID，FAT32系统中一般取为“FAT32”：占8个字节
	//由于引导代码对于本虚拟系统没有用，故省略
	memcpy(buf+510,&end,2);
    //WR_SECT_FAT(buf, 1);			//deleted by mingxuan 2020-10-27
	WR_SECT_FAT(fat32_dev, buf, 1);	//modified by mingxuan 2020-10-27

	//初始化FAT
	memset(buf,0,SECTOR_SIZE);//写介质描述单元
	memcpy(buf,media_descriptor,8);
	memcpy(buf+8,&cluster_tag,4);//写根目录的簇号
    //WR_SECT_FAT(buf, reserved_sector);	// deleted by mingxuan 2020-10-27
	WR_SECT_FAT(fat32_dev, buf, reserved_sector);	// modified by mingxuan 2020-10-27

	//初始化根目录
	CreateRecord(volumelabel,0x08,0,0,&vLabel);//准备卷标的目录项的数据	
	memset(buf,0,SECTOR_SIZE);//将准备好的记录项数据写入虚拟硬盘
	memcpy(buf,&vLabel,sizeof(Record));
    //WR_SECT_FAT(buf, reserved_sector+2*sectors_per_FAT);	// deleted by mingxuan 2020-10-27
	WR_SECT_FAT(fat32_dev, buf, reserved_sector+2*sectors_per_FAT);	// modified by mingxuan 2020-10-27
}

int rw_sector_fat(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
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

int rw_sector_sched_fat(int io_type, int dev, int pos, int bytes, int proc_nr, void* buf)
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

int sys_CreateFile(void *uesp)
{
	state=CreateFile((PCHAR)(void *)get_arg(uesp, 1)); 
	if(state==OK)
	{
		kprintf("           create file success");
	}
	else {
		DisErrorInfo(state);
	}

	return state;
}

int sys_DeleteFile(void *uesp)
{
	state=DeleteFile((PCHAR)(void *)get_arg(uesp, 1));
	if(state==OK)
	{
		kprintf("           delete file success");
	}
	else {
		DisErrorInfo(state);
	}
	return state;
}

int sys_OpenFile(void *uesp)
{
/*	// state=OpenFile(get_arg(uesp, 1),
	// 				get_arg(uesp, 2));
	// if(state==OK)
	// {
	// 	kprintf("open file success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
	state=OpenFile(get_arg(uesp, 1),
					get_arg(uesp, 2));
	kprintf("           open file success");
	return state;	*/
	return 0;
}

int sys_CloseFile(void *uesp)
{
	state=CloseFile(get_arg(uesp, 1));
	if(state==OK)
	{
		kprintf("           close file success");
	}
	else {
		DisErrorInfo(state);
	}
	return state;
}

int sys_WriteFile(void *uesp)
{
	state=WriteFile(get_arg(uesp, 1),
			(BYTE *)(void *)get_arg(uesp, 2),
			get_arg(uesp, 3));
	if(state==OK)
	{
		kprintf("           write file success");
	}
	else {
		DisErrorInfo(state);
	}
	return state;
}

int sys_ReadFile(void *uesp)
{
	// state=ReadFile(get_arg(uesp, 1),
	// 				get_arg(uesp, 2),
	// 				get_arg(uesp, 3),
	// 				get_arg(uesp, 4));
	// if(state==OK)
	// {
	// 	//debug("read file success");
	// 	debug("           read file success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
	return 0;
}

int sys_OpenDir(void *uesp)
{
	// state=OpenDir(get_arg(uesp, 1));
	// if(state==OK)
	// {
	// 	//debug("open dir success");
	// 	debug("           open dir success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
	return 0;
}

int sys_CreateDir(void *uesp)
{
	// state=CreateDir(get_arg(uesp, 1));
	// if(state==OK)
	// {
	// 	//debug("create dir success");
	// 	debug("           create dir success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
	return 0;
}

int sys_DeleteDir(void *uesp)
{
	// state=DeleteDir(get_arg(uesp, 1));
	// if(state==OK)
	// {
	// 	debug("delete dir success");
	// 	debug("           delete dir success");
	// }
	// else {
	// 	DisErrorInfo(state);
	// }
	// return state;
	return 0;
}

int sys_ListDir(void *uesp) {
		
	// DArray *array=NULL;
	// char *s = get_arg(uesp, 1);
	// CHAR temp[256]={0};
	// UINT tag=0;

	// array = InitDArray(10, 10);
	// memset(temp, 0, sizeof(temp));
	// if (strlen(s) != 0)
	// {
	// 	strcpy(temp,s);
	// 	if(IsFile(temp,&tag))
	// 	{
	// 		if(tag==1)
	// 		{
	// 			printf("不是目录的路径\n\n");
	// 		}
	// 	}
	// }
	// else {
	// 	GetCurrentPath(temp);
	// }
	// state=ListAll(temp, array);
	// if(state==OK)
	// {
	// 	DirCheckup(array);
	// }else {
	// 	DisErrorInfo(state);
	// 	kprintf("\n");
	// }
	// DestroyDArray(array);
	return 0;
}

void DisErrorInfo(STATE state)
{
	if(state==SYSERROR)
	{
		kprintf("          system error\n");
	}
	else if(state==VDISKERROR)
	{
		kprintf("          disk error\n");
	}
	else if(state==INSUFFICIENTSPACE)
	{
		kprintf("          no much space\n");
	}
	else if(state==WRONGPATH)
	{
		kprintf("          path error\n");
	}
	else if(state==NAMEEXIST)
	{
		kprintf("          name exists\n");
	}
	else if(state==ACCESSDENIED)
	{
		kprintf("          deny access\n");
	}
	else
	{
		kprintf("          unknown error\n");
	}
}

