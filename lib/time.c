#include <sys/defs.h>
#include <stdlib.h>
#include <time.h>

clock_t clock() {
    u64 sys_tick = get_ticks();
    return sys_tick * 1000 / SYSCLK_FREQ_HZ;
}
