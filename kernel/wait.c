#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/proto.h>
#include <unios/global.h>
#include <unios/spinlock.h>
#include <arch/x86.h>
#include <stdlib.h>
#include <stdio.h>

static pcb_t* get_exit_child_proc() {
    pcb_t* pcb = &p_proc_current->pcb;
    for (int i = 0; i < pcb->info.child_p_num; ++i) {
        pcb_t* exit_child = (pcb_t*)pid2proc(pcb->info.child_process[i]);
        if (exit_child->stat == ZOMBIE) { return exit_child; }
    }
    return NULL;
}

int do_wait(int* wstatus) {
    assert(false && "not implemented");
}
