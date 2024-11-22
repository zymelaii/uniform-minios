#pragma once

#include <macro_helper.h>

//! NOTE: see https://en.wikipedia.org/wiki/FLAGS_register
#define EFLAGS_RESERVED 0x00000002            //<! always 1 in eflags
#define EFLAGS_CF       0x00000001            //<! carry flag
#define EFLAGS_IF       0x00000200            //<! interrupt enable flag
#define EFLAGS_IOPL(pl) (((pl) & 0b11) << 12) //<! I/O privilege level
#define EFLAGS_PF       0x00000004            //<! parity flag
#define EFLAGS_AF       0x00000010            //<! auxiliary carry flag
#define EFLAGS_ZF       0x00000040            //<! zero flag
#define EFLAGS_SF       0x00000080            //<! sign flag
#define EFLAGS_TF       0x00000100            //<! trap flag
#define EFLAGS_IF       0x00000200            //<! interrupt flag
#define EFLAGS_DF       0x00000400            //<! direction flag
#define EFLAGS_OF       0x00000800            //<! overflow flag
#define EFLAGS_NT       0x00004000            //<! nested task
#define EFLAGS_RF       0x00010000            //<! resume flag
#define EFLAGS_VM       0x00020000            //<! virtual 8086 mode
#define EFLAGS_AC       0x00040000            //<! alignment check
#define EFLAGS_VIF      0x00080000            //<! virtual interrupt flag
#define EFLAGS_VIP      0x00100000            //<! virtual interrupt pending
#define EFLAGS_ID       0x00200000            //<! id flag

//! cr0 register flags
#define CR0_PE 0x00000001 //<! protection enable
#define CR0_MP 0x00000002 //<! monitor coprocessor
#define CR0_EM 0x00000004 //<! emulation
#define CR0_TS 0x00000008 //<! task switched
#define CR0_ET 0x00000010 //<! extension type
#define CR0_NE 0x00000020 //<! numeric errror
#define CR0_WP 0x00010000 //<! write protect
#define CR0_AM 0x00040000 //<! alignment mask
#define CR0_NW 0x20000000 //<! not writethrough
#define CR0_CD 0x40000000 //<! cache disable
#define CR0_PG 0x80000000 //<! paging

//! cr4 register flags
#define CR4_PCE 0x00000100 //<! performance counter enable
#define CR4_MCE 0x00000040 //<! machine check enable
#define CR4_PSE 0x00000010 //<! page size extensions
#define CR4_DE  0x00000008 //<! debugging extensions
#define CR4_TSD 0x00000004 //<! time stamp disable
#define CR4_PVI 0x00000002 //<! protect mode virtual interrupts
#define CR4_VME 0x00000001 //<! v86 mode extensions

//! helpful macros to declare eflags, e.g. EFLAGS(IF, IOPL(1))
#define EFLAGS_IMPL1(x, ...) MH_EXPAND(MH_CONCAT(EFLAGS_, x))
#define EFLAGS_IMPL2(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL1(_, __VA_ARGS__))
#define EFLAGS_IMPL3(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL2(_, __VA_ARGS__))
#define EFLAGS_IMPL4(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL3(_, __VA_ARGS__))
#define EFLAGS_IMPL5(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL4(_, __VA_ARGS__))
#define EFLAGS_IMPL6(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL5(_, __VA_ARGS__))
#define EFLAGS_IMPL7(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL6(_, __VA_ARGS__))
#define EFLAGS_IMPL8(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL7(_, __VA_ARGS__))
#define EFLAGS_IMPL9(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL8(_, __VA_ARGS__))
#define EFLAGS_IMPL10(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL9(_, __VA_ARGS__))
#define EFLAGS_IMPL11(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL10(_, __VA_ARGS__))
#define EFLAGS_IMPL12(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL11(_, __VA_ARGS__))
#define EFLAGS_IMPL13(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL12(_, __VA_ARGS__))
#define EFLAGS_IMPL14(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL13(_, __VA_ARGS__))
#define EFLAGS_IMPL15(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL14(_, __VA_ARGS__))
#define EFLAGS_IMPL16(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL15(_, __VA_ARGS__))
#define EFLAGS_IMPL17(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL16(_, __VA_ARGS__))
#define EFLAGS_IMPL18(x, _, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_, x)) | MH_EXPAND(EFLAGS_IMPL17(_, __VA_ARGS__))
#define EFLAGS_IMPL(N, x, ...) \
    MH_EXPAND(MH_CONCAT(EFLAGS_IMPL, N)(x, __VA_ARGS__))
#define EFLAGS_WRAPPER(...) \
    MH_EXPAND(EFLAGS_IMPL(MH_EXPAND(MH_NARG(__VA_ARGS__)), __VA_ARGS__))
#define EFLAGS(...) (EFLAGS_WRAPPER(RESERVED, __VA_ARGS__))
