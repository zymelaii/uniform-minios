#ifndef	_ORANGES_MEMMAN_H_
#define	_ORANGES_MEMMAN_H_

#define MEMMAN_FREES	4090		//32KB
#define MEMMAN_ADDR	0x01ff0000	//存memman，31M960K
#define FMIBuff		0x007ff000	//loader中getFreeMemInfo返回值存放起始地址(7M1020K)
#define KWALL		0x00600000
#define WALL		0x00800000
#define UWALL		0x01000000
#define MEMSTART	0x00400000
#define MEMEND		0x02000000
#define TEST		0x11223344
struct FREEINFO{
	u32 addr,size;
};					

struct MEMMAN{
	u32 frees,maxfrees,lostsize,losts;	//frees为当前空闲内存块数
	struct FREEINFO free[MEMMAN_FREES];	//空闲内存
};

void init();
u32 do_malloc(u32 size);
u32 do_kmalloc(u32 size);
u32 do_malloc_4k();
u32 do_kmalloc_4k();
u32 do_free(u32 addr,u32 size);
u32 do_free_4k(u32 addr);


#endif 