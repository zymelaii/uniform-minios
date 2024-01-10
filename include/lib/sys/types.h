#pragma once

#include <stdint.h>

typedef int32_t  pid_t;
typedef uint32_t clock_t;
typedef void*    handle_t;

typedef int32_t  offset_t;
typedef uint32_t phyaddr_t;

typedef void (*int_handler_t)();
typedef void (*irq_handler_t)();

typedef void (*task_handler_t)();

typedef uint32_t (*syscall_t)();
