#include <unios/page.h>
#include <unios/syscall.h>
#include <assert.h>
#include <type.h>
#include <const.h>
#include <global.h>
#include <string.h>
#include <stdio.h>
#include <x86.h>

// to determine if a page fault is reparable
u32 cr2_save;
u32 cr2_count = 0;

void switch_pde() {
    //! switch the page directory table after schedule() is called
    cr3_ready = p_proc_current->pcb.cr3;
}

bool pg_unmap_laddr(u32 cr3, u32 laddr, bool free) {
    assert(cr3 != 0);
    u32 pde = pg_pde(cr3, laddr);
    //! case 0: pde not present is also a good unmap
    if ((pde & PG_MASK_P) != PG_P) { return true; }
    u32 *pte_ptr = pg_pte_ptr(pde, laddr);
    u32  pte     = *pte_ptr;
    //! case 1: pte already not present
    if ((pte & PG_MASK_P) != PG_P) { return true; }
    u32 phyaddr = pg_frame_phyaddr(pte);
    assert(phyaddr != 0);
    assert(phyaddr == pg_frame_phyaddr(phyaddr));
    //! case 2: pte present, reset then
    if (free) { do_free((void *)phyaddr); }
    *pte_ptr = 0;
    pg_refresh();
    return true;
}

bool pg_map_laddr(u32 cr3, u32 laddr, u32 phyaddr, u32 pde_attr, u32 pte_attr) {
    assert(cr3 != 0);
    assert(pg_frame_phyaddr(pde_attr) == 0);
    assert(pg_frame_phyaddr(pte_attr) == 0);
    assert(phyaddr == 0 || phyaddr == pg_frame_phyaddr(phyaddr));
    assert((pde_attr & PG_MASK_P) == PG_P);
    assert((pte_attr & PG_MASK_P) == PG_P);

    u32 *pde_ptr = pg_pde_ptr(cr3, laddr);
    if ((*pde_ptr & PG_MASK_P) != PG_P) {
        u32 pde_phyaddr = (u32)do_kmalloc_4k();
        assert(pde_phyaddr != 0);
        assert(pde_phyaddr == pg_frame_phyaddr(pde_phyaddr));
        memset((void *)K_PHY2LIN(pde_phyaddr), 0, num_4K);
        *pde_ptr = pde_phyaddr | pde_attr;
    }
    u32 pde = *pde_ptr;

    u32 old_pte     = pg_pte(pde, laddr);
    u32 old_phyaddr = pg_frame_phyaddr(old_pte);
    if (phyaddr == 0) {
        if ((old_pte & PG_MASK_P) != PG_P) {
            bool in_kernel = laddr >= KernelLinBase;
            phyaddr = (u32)(in_kernel ? do_kmalloc_4k() : do_malloc_4k());
        } else {
            phyaddr = old_phyaddr;
        }
    }
    assert(phyaddr != 0);
    assert(phyaddr == pg_frame_phyaddr(phyaddr));

    u32 pte = phyaddr | pte_attr;

    //! case 0: compeletly the same, no need to write
    if (old_pte == pte) { return true; }

    if (old_phyaddr == phyaddr) {
        //! case 1: old present, with the same page, but different from attr
        //! TODO: attr overwrite policy
    } else if (old_phyaddr != 0) {
        //! case 2: old present, but with different phy page
        //! TODO: release old phy page
    } else {
        //! case 3: old not present, just write
    }

    *pg_pte_ptr(pde, laddr) = pte;
    return true;
}

bool pg_map_laddr_range(
    u32 cr3, u32 laddr_base, u32 laddr_limit, u32 pde_attr, u32 pte_attr) {
    laddr_base  = pg_frame_phyaddr(laddr_base);
    laddr_limit = pg_frame_phyaddr(laddr_limit + 0xfff);
    for (u32 laddr = laddr_base; laddr < laddr_limit; laddr += num_4K) {
        bool ok = pg_map_laddr(cr3, laddr, 0, pde_attr, pte_attr);
        if (!ok) { return false; }
    }
    return true;
}

void pg_refresh() {
    tlbflush();
}

void page_fault_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags) {
    u32 cr2 = rcr2();

    trace_logging("BEGIN --> trigger page fault\n");
    if (kernel_initial) { trace_logging("during initializing kernel\n"); }

    trace_logging(
        "eip=0x%08x cr2=0x%08x code=%x cs=0x%08x eflags=0x%04x\n",
        eip,
        cr2,
        err_code,
        cs,
        eflags);

    bool recovery_failed = false;
    do {
        if (kernel_initial) {
            recovery_failed = true;
            break;
        }

        u32  cr3     = p_proc_current->pcb.cr3;
        u32 *pde_ptr = pg_pde_ptr(cr3, cr2);
        u32  pde     = *pde_ptr;
        u32 *pte_ptr = pg_pte_ptr(pde, cr2);
        u32  pte     = *pte_ptr;

        const char *flag[3][2] = {
            {"NP", "P"  },
            {"RX", "RWX"},
            {"S",  "U"  },
        };
        trace_logging(
            "cr3=0x%08x pde=0x%08x { %s | %s | %s } pte=0x%08x { %s | %s | %s "
            "}\n",
            p_proc_current->pcb.cr3,
            pde,
            flag[0][(pde & PG_MASK_US) == PG_U],
            flag[1][(pde & PG_MASK_RW) == PG_RWX],
            flag[2][(pde & PG_MASK_P) == PG_P],
            pte,
            flag[0][(pte & PG_MASK_US) == PG_U],
            flag[1][(pte & PG_MASK_RW) == PG_RWX],
            flag[2][(pte & PG_MASK_P) == PG_P]);

        const int MAX_RETRIES = 1;
        if (cr2 != cr2_save) {
            cr2_save  = cr2;
            cr2_count = 0;
        } else if (++cr2_count >= MAX_RETRIES) {
            recovery_failed = true;
            break;
        } else {
            trace_logging(
                "%d times retry available\n", MAX_RETRIES - cr2_count);
        }

        *pde_ptr |= PG_P;
        *pte_ptr |= PG_P;
        pg_refresh();
    } while (0);

    if (recovery_failed) { trace_logging("page fault recovery failed\n"); }

    trace_logging("<--- END\n");

    if (recovery_failed) {
        if (!kernel_initial) { disable_int(); }
        halt();
    }
}

u32 pg_create_and_init() {
    u32 cr3 = (u32)do_kmalloc_4k();
    assert(cr3 != 0);
    assert(pg_frame_phyaddr(cr3) == cr3);
    memset((void *)K_PHY2LIN(cr3), 0, num_4K);

    //! init kernel memory space
    u32 laddr   = KernelLinBase;
    u32 phyaddr = 0;
    while (laddr < KernelLinBase + KernelSize) {
        bool ok = pg_map_laddr(
            cr3, laddr, phyaddr, PG_P | PG_U | PG_RWX, PG_P | PG_S | PG_RWX);
        assert(ok);
        laddr   += num_4K;
        phyaddr += num_4K;
    }
    pg_refresh();
    return cr3;
}

void clear_kernel_pagepte_low() {
    u32 page_num = *(u32 *)PageTblNumAddr;
    u32 phyaddr  = KernelPageTblAddr;
    memset((void *)K_PHY2LIN(phyaddr), 0, sizeof(u32) * page_num);
    memset((void *)K_PHY2LIN(phyaddr + num_4K), 0, num_4K * page_num);
    pg_refresh();
}
