
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            type.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef	_ORANGES_TYPE_H_
#define	_ORANGES_TYPE_H_

#ifndef NULL
#define NULL ((void*) 0)
#endif

typedef _Bool bool;
enum { false, true };

typedef long long		i64;
typedef unsigned long long	u64;
typedef int			i32;
typedef unsigned int		u32;
typedef short			i16;
typedef unsigned short		u16;
typedef char			i8;
typedef unsigned char		u8;

typedef i32			intptr_t;
typedef u32 			uintptr_t;

// 通常描述一个对象的大小，会根据机器的型号变化类型
typedef u32			size_t;
// signed size_t 通常描述系统调用返回值，会根据机器的型号变化类型
typedef i32			ssize_t;
// 通常描述偏移量，会根据机器的型号变化类型
typedef i32			off_t;
// 通常描述物理地址
typedef u32			phyaddr_t;

typedef	void	(*int_handler)	();
typedef	void	(*task_f)	();
typedef	void	(*irq_handler)	(int irq);

typedef	char *	va_list;	//added by mingxuan 2019-5-19

typedef void*	system_call;

//mainly used in filesystem. added by xw, 18/8/27
/**
 * MESSAGE mechanism is borrowed from MINIX
 */
struct mess1 {
	int m1i1;
	int m1i2;
	int m1i3;
	int m1i4;
};

struct mess2 {
	void* m2p1;
	void* m2p2;
	void* m2p3;
	void* m2p4;
};

struct mess3 {
	int	m3i1;
	int	m3i2;
	int	m3i3;
	int	m3i4;
	u64	m3l1;
	u64	m3l2;
	void*	m3p1;
	void*	m3p2;
};

typedef struct {
	int source;
	int type;
	union {
		struct mess1 m1;
		struct mess2 m2;
		struct mess3 m3;
	} u;
} MESSAGE;

/**
 * @enum msgtype
 * @brief MESSAGE types
 */
enum msgtype {
	/* 
	 * when hard interrupt occurs, a msg (with type==HARD_INT) will
	 * be sent to some tasks
	 */
	HARD_INT = 1,

	/* SYS task */
	GET_TICKS,

	/// zcr added from ch9/e/include/const.h
	/* FS */
	OPEN, CLOSE, READ, WRITE, LSEEK, STAT, UNLINK,
	
	/* message type for drivers */
	DEV_OPEN = 1001,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL
};

#endif /* _ORANGES_TYPE_H_ */
