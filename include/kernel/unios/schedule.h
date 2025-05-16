#pragma once

#include <sys/types.h>

void restart_initial();
void restart_restore();
void sched();

extern phyaddr_t cr3_ready;
