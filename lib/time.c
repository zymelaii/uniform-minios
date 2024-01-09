#include <sys/defs.h>
#include <stdlib.h>
#include <time.h>

clock_t clock_from_sysclk(int ticks) {
    return ticks * 1000 / SYSCLK_FREQ_HZ;
}

clock_t clock() {
    return clock_from_sysclk(get_ticks());
}
