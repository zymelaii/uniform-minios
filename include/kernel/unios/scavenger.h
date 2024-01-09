#pragma once

#include <unios/proc.h>
#include <sys/types.h>

void recycle_memory_part(phyaddr_t cr3, void* base, void* limit);
void recycle_proc_memory(process_t* proc);

void scavenger();
