#pragma once

#include <stdint.h>
#include "hd.h"

/* equal to 1 if kernel is initializing, equal to 0 if done.
 * added by xw, 18/5/31
 */
extern int kernel_initial;

extern int ticks;

extern u32 k_reenter;
extern int u_proc_sum; // 内核中用户进程/线程数量

extern char task_stack[];
extern u32  cr3_ready;

struct memfree {
    u32 addr;
    u32 size;
};

extern struct hd_info hd_info[1];
