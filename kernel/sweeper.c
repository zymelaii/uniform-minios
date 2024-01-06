#include <unios/proc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

void sweeper() {
    while (true) {
        int pid = wait(NULL);
        if (pid == -1) {
            p_proc_current->pcb.stat = SLEEPING;
            yield();
        } else {
            trace_logging("---recycled orphan! pid[%d]---\n", pid);
        }
    }
}