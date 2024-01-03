#pragma once

#include <stdint.h>
#include <stdbool.h>

#define NULL ((void *)0)

#define TRUE  true
#define FALSE false

typedef uint32_t size_t;
typedef int32_t  ssize_t;

typedef int32_t  intptr_t;
typedef uint32_t uintptr_t;

typedef int32_t ptrdiff_t;

//! WARNING: make sure that your x, y expr no side-effect!
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
