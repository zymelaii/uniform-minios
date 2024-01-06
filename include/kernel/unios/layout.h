#pragma once

#include <stddef.h>
#include <sys/types.h>

#define NUM_4B 0x00000004
#define NUM_1K 0x00000400
#define NUM_4K 0x00001000
#define NUM_1M 0x00100000
#define NUM_4M 0x00400000
#define NUM_1G 0x40000000

//! WARNING: real size of kernel may excceed this
#define KernelSize 0x800000

//! where to store total page table, see load.inc
#define PageTblNumAddr 0x500

//! where is kernel page table, see load.inc
#define KernelPageTblAddr 0x200000

//! read only sections in elf
//! NOTE: only for reference, refer to target elf please
#define RO_SECTION_SIZE (512 * NUM_1M)
#define TextLinBase     ((uintptr_t)0x0)
#define TextLinLimitMAX (TextLinBase + RO_SECTION_SIZE)
#define DataLinLimitMAX TextLinLimitMAX

//! reserved memory space
#define VpageLinBase     ((uintptr_t)DataLinLimitMAX)
#define VpageLinLimitMAX (VpageLinBase + 128 * NUM_1M - NUM_4K)

//! shared memory space, reserved for fork, etc.
#define SharePageBase  ((uintptr_t)VpageLinLimitMAX)
#define SharePageLimit (SharePageBase + NUM_4K)

//! user heap
#define HeapLinBase     ((uintptr_t)SharePageLimit)
#define HeapLinLimitMAX (HeapLinBase + NUM_1G)

//! kernel memory space
#define KernelLinBase     ((uintptr_t)(3u * NUM_1G))
#define KernelLinLimitMAX (KernelLinBase + NUM_1G)

//! reserve one page for execve argv & envp
#define ArgLinBase     ((uintptr_t)(KernelLinBase - NUM_4K))
#define ArgLinLimitMAX KernelLinBase

//! user stack
#define StackLinBase     ((uintptr_t)ArgLinBase)
#define StackLinLimitMAX HeapLinLimitMAX

#define K_PHY2LIN(x) ((void*)((phyaddr_t)(x) + (KernelLinBase)))
#define K_LIN2PHY(x) ((phyaddr_t)((void*)(x) - (KernelLinBase)))
