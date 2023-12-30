#include <type.h>
#include <proto.h>
#include <global.h>

void do_exit(int exit_code) {
    p_proc_current->task.stat = IDLE;
    sched();
}
