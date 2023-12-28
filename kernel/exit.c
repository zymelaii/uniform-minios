#include <type.h>
#include <proto.h>
#include <global.h>

void sys_exit(int exitcode) {
    p_proc_current->task.stat = IDLE;
    sched();
}
