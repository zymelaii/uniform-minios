#pragma once

#include <unios/proc.h>

enum environ_op {
    ENVIRON_PUT,
    ENVIRON_GET,
};

char **kgetenvp(process_t *proc);
