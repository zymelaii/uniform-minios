#include <unios/vfs.h>
#include <unios/protect.h>
#include <unios/proc.h>
#include <unios/proto.h>
#include <unios/fs_const.h>
#include <unios/hd.h>
#include <stdint.h>

int kernel_initial;
int ticks;
u32 k_reenter;
int u_proc_sum;

u32 cr3_ready;
