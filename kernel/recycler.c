#include <unios/syscall.h>
#include <unios/global.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

void sweeper() {
    while (1) {
        if (wait(NULL) == -1) {
            p_proc_current->pcb.stat = SLEEPING;
            yield();
        } else {
            trace_logging("---recycled orphan! pid[/]---\n");
        }
    }
}
