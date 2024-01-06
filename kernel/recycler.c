#include <unios/proc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

void sweeper() {
    while (true) {
        if (wait(NULL) == -1) {
            p_proc_current->pcb.stat = SLEEPING;
            yield();
        } else {
            trace_logging("---recycled orphan! pid[/]---\n");
        }
    }
}
