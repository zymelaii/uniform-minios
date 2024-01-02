#include "assert.h"
#include "proc.h"
#include "stdlib.h"
#include "x86.h"
#include <type.h>
#include <proto.h>
#include <global.h>
#include <spinlock.h>
#include <stdio.h>

static PCB_t* get_exit_child_proc() {
    PCB_t* pcb = &p_proc_current->pcb;
    for (int i = 0; i < pcb->info.child_p_num; ++i) {
        PCB_t* exit_child = (PCB_t*)pid2proc(pcb->info.child_process[i]);
        if (exit_child->stat == ZOMBIE) { return exit_child; }
    }
    return NULL;
}

int do_wait(int* wstatus) {
    assert(false && "not implemented");
}
