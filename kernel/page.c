#include <unios/page.h>
#include <unios/syscall.h>
#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/kstate.h>
#include <arch/x86.h>
#include <string.h>
#include <stdio.h>

bool pg_free_pde(u32 cr3) {
    assert(cr3 != 0);
    assert(cr3 == pg_frame_phyaddr(cr3));
    assert(cr3 != p_proc_current->pcb.cr3);
    do_free((void *)pg_frame_phyaddr(cr3));
    return true;
}

bool pg_unmap_pte(u32 cr3, bool free) {
    assert(cr3 != 0);
    u32 laddr = 0;
    for (u32 i = 0; i < 1024; ++i, laddr += 0x400000) {
        u32 *pde_ptr = pg_pde_ptr(cr3, laddr);
        u32  pde     = *pde_ptr;
        //! case 0: pde not exist is also a good unmap
        if ((pde & PG_MASK_P) != PG_P) { continue; }
        //! case 1: unmap the pde
        u32 pde_phyaddr = pg_frame_phyaddr(pde);
        if (free) { do_free((void *)pde_phyaddr); }
        *pde_ptr = 0;
    }
    pg_refresh();
    return true;
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
    assert(phyaddr == PG_INVALID || phyaddr == pg_frame_phyaddr(phyaddr));
    assert((pde_attr & PG_MASK_P) == PG_P);
    assert((pte_attr & PG_MASK_P) == PG_P);
    u32 *pde_ptr = pg_pde_ptr(cr3, laddr);
    if ((*pde_ptr & PG_MASK_P) != PG_P) {
        u32 pde_phyaddr = (u32)do_kmalloc_4k();
        assert(pde_phyaddr == pg_frame_phyaddr(pde_phyaddr));
        memset(K_PHY2LIN(pde_phyaddr), 0, NUM_4K);
        *pde_ptr = pde_phyaddr | pde_attr;
    }
    u32 pde         = *pde_ptr;
    u32 old_pte     = pg_pte(pde, laddr);
    u32 old_phyaddr = pg_frame_phyaddr(old_pte);
    if (phyaddr == PG_INVALID) {
        if ((old_pte & PG_MASK_P) != PG_P) {
            bool in_kernel = laddr >= KernelLinBase;
            phyaddr = (u32)(in_kernel ? do_kmalloc_4k() : do_malloc_4k());
        } else {
            phyaddr = old_phyaddr;
        }
    }
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
    for (u32 laddr = laddr_base; laddr < laddr_limit; laddr += NUM_4K) {
        bool ok = pg_map_laddr(cr3, laddr, PG_INVALID, pde_attr, pte_attr);
        if (!ok) { return false; }
    }
    return true;
}

void pg_refresh() {
    tlbflush();
}

void page_fault_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags) {
    u32 cr2 = rcr2();

    klog("BEGIN --> trigger page fault\n");
    if (kstate_on_init) { klog("during initializing kernel\n"); }

    klog(
        "pid[%d]: eip=0x%08x cr2=0x%08x code=%x cs=0x%08x eflags=0x%04x\n",
        p_proc_current->pcb.pid,
        eip,
        cr2,
        err_code,
        cs,
        eflags);

    if (!kstate_on_init) {
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
        klog(
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
    }

    klog("<--- END\n");

    if (!kstate_on_init) { disable_int(); }
    halt();
}

u32 pg_create_and_init() {
    u32 cr3 = (u32)do_kmalloc_4k();
    //! NOTE: cr3 should only comes from pg_create_and_init and is always
    //! non-zero
    assert(cr3 != 0);
    assert(cr3 == pg_frame_phyaddr(cr3));
    memset(K_PHY2LIN(cr3), 0, NUM_4K);

    //! init kernel memory space
    u32 laddr   = KernelLinBase;
    u32 phyaddr = 0;
    while (laddr < KernelLinBase + KernelSize) {
        bool ok = pg_map_laddr(
            cr3, laddr, phyaddr, PG_P | PG_U | PG_RWX, PG_P | PG_S | PG_RWX);
        assert(ok);
        laddr   += NUM_4K;
        phyaddr += NUM_4K;
    }
    pg_refresh();
    return cr3;
}

void clear_kernel_pagepte_low() {
    u32 page_num = *(u32 *)PageTblNumAddr;
    u32 phyaddr  = KernelPageTblAddr;
    memset(K_PHY2LIN(phyaddr), 0, sizeof(u32) * page_num);
    memset(K_PHY2LIN(phyaddr + NUM_4K), 0, NUM_4K * page_num);
    pg_refresh();
}
