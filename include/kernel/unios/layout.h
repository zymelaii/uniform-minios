#pragma once

#include <sys/types.h>
#include <stddef.h> // IWYU pragma: keep

#define NUM_4B 0x00000004
#define NUM_1K 0x00000400
#define NUM_4K 0x00001000
#define NUM_1M 0x00100000
#define NUM_4M 0x00400000
#define NUM_1G 0x40000000

//! where to store page table from loader ~ phy [1, 2) MB
//! NOTE: reusable after `kernel_main` done
#define LoaderPageTableBase  NUM_1M
#define LoaderPageTableLimit (LoaderPageTableBase + NUM_1M)

//! user heap
#define HeapLinBase     ((uintptr_t)(512u * NUM_1M))
#define HeapLinLimitMAX (HeapLinBase + NUM_1G)

//! shared memory space, used by fork
#define SharePageBase  ((uintptr_t)(HeapLinBase - NUM_4K))
#define SharePageLimit HeapLinBase

//! kernel memory space
#define KernelLinBase     ((uintptr_t)(3u * NUM_1G))
#define KernelLinLimitMAX (KernelLinBase + NUM_1G)

//! reserve one page for execve argv & envp
#define ArgLinBase     ((uintptr_t)(KernelLinBase - NUM_4K))
#define ArgLinLimitMAX KernelLinBase

//! user stack
#define StackLinBase     ((uintptr_t)ArgLinBase)
#define StackLinLimitMAX HeapLinLimitMAX

#define K_PHY2LIN(x) ((void *)((phyaddr_t)(x) + (KernelLinBase)))
#define K_LIN2PHY(x) ((phyaddr_t)((void *)(x) - (KernelLinBase)))
