#include <unios/page.h>
#include <unios/syscall.h>
#include <unios/assert.h>
#include <unios/proc.h>
#include <unios/kstate.h>
#include <unios/memory.h>
#include <unios/tracing.h>
#include <arch/x86.h>
#include <string.h>

bool pg_free_page_table(u32 cr3) {
    assert(cr3 != 0);
    assert(cr3 == pg_frame_phyaddr(cr3));
    assert(cr3 != p_proc_current->pcb.cr3);
    free_phypage(pg_frame_phyaddr(cr3));
    return true;
}

bool pg_clear_page_table(u32 cr3, bool free) {
    assert(cr3 != 0);
    u32 laddr = 0;
    for (u32 i = 0; i < 1024; ++i, laddr += 0x400000) {
        u32 *pde_ptr = pg_pde_ptr(cr3, laddr);
        u32  pde     = *pde_ptr;
        //! case 0: pde not exist is also a good unmap
        if ((pde & PG_MASK_P) != PG_P) { continue; }
        //! case 1: unmap the pde
        u32 pde_phyaddr = pg_frame_phyaddr(pde);
        if (free) { free_phypage(pde_phyaddr); }
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
    if (free) { free_phypage(phyaddr); }
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
        u32 pde_phyaddr = (u32)kmalloc_phypage();
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
            phyaddr = (u32)(in_kernel ? kmalloc_phypage() : malloc_phypage());
            if (phyaddr == 0) {
                kinfo("warn: pg_map_laddr: run out of phy page");
                return false;
            }
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

bool pg_unmap_laddr_range(u32 cr3, u32 laddr_base, u32 laddr_limit, bool free) {
    laddr_base  = pg_frame_phyaddr(laddr_base);
    laddr_limit = pg_frame_phyaddr(laddr_limit + 0xfff);
    for (u32 laddr = laddr_base; laddr < laddr_limit; laddr += NUM_4K) {
        bool ok = pg_unmap_laddr(cr3, laddr, free);
        if (!ok) { return false; }
    }
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
    static uint32_t pfh_cntr = 0;
    uint32_t        id       = pfh_cntr++;

    u32 cr2 = rcr2();

    kinfo(
        "[#PF.%d] trigger page fault %s",
        ++id,
        kstate_on_init ? "during initializing kernel" : "");

    kinfo(
        "eip=%#08x cr2=%#08x code=%x cs=%#08x eflags=%#04x from pid=%d",
        eip,
        cr2,
        err_code,
        cs,
        eflags,
        p_proc_current->pcb.pid);

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
        kinfo(
            "cr3=%#08x pde=%#08x { %s | %s | %s } pte=%#08x { %s | %s | %s }",
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

    kinfo("[#PF.%d] page fault handler done", id);

    if (!kstate_on_init) { disable_int(); }
    halt();

    kinfo("[#PF.%d] leave page fault handler", id);
}

bool pg_create_and_init(u32 *p_cr3) {
    assert(p_cr3 != NULL);
    u32 cr3 = kmalloc_phypage();
    //! NOTE: cr3 should only comes from pg_create_and_init and is always
    //! non-zero
    if (cr3 == 0) { return false; }
    assert(cr3 == pg_frame_phyaddr(cr3));
    memset(K_PHY2LIN(cr3), 0, NUM_4K);
    bool should_rollback = false;
    //! init kernel memory space
    phyaddr_t phy_base  = 0;
    phyaddr_t phy_limit = 0;
    bool      ok        = get_phymem_bound(KernelSpace, &phy_base, &phy_limit);
    assert(ok);
    u32 base    = (u32)K_PHY2LIN(phy_base);
    u32 limit   = (u32)K_PHY2LIN(phy_limit);
    u32 laddr   = base;
    u32 phyaddr = 0;
    while (laddr < limit) {
        bool ok = pg_map_laddr(
            cr3, laddr, phyaddr, PG_P | PG_U | PG_RWX, PG_P | PG_S | PG_RWX);
        if (!ok) {
            should_rollback = true;
            break;
        }
        laddr   += NUM_4K;
        phyaddr += NUM_4K;
    }
    if (should_rollback) {
        pg_unmap_laddr_range(cr3, base, limit, false);
        return false;
    } else {
        pg_refresh();
    }
    *p_cr3 = cr3;
    return true;
}

#pragma GCC push_options
#pragma GCC optimize("O3")

u32 pg_laddr_phyaddr(u32 cr3, u32 laddr) {
    u32 pde = pg_pde(cr3, laddr);
    u32 pte = pg_pte(pde, laddr);
    return pg_frame_phyaddr(pte) | pg_offset(laddr);
}

bool pg_addr_pte_exist(u32 cr3, u32 laddr) {
    u32 pde = pg_pde(cr3, laddr);
    if ((pde & PG_MASK_P) != PG_P) { return false; }
    return pg_pte_exist(pde, laddr);
}

#pragma GCC pop_options
