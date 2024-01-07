#pragma once

#include <sys/types.h>

void restart_initial();
void restart_restore();
void schedule();

extern phyaddr_t cr3_ready;
