#include <unios/layout.h>
#include <unios/host_device.h>
#include <sys/types.h>
#include <config.h>
#include <stddef.h>
#include <math.h>

#define INVALID_PHYADDR 0xffffffff

static size_t get_total_memory() {
    size_t              total_memory = 0;
    host_device_info_t *device_info  = (void *)DEVICE_INFO_ADDR;

    for (int i = 0; i < device_info->ards_count; i++) {
        ards_t ards = device_info->ards_buffer[i];
        if (ards.type == 1) {
            size_t segment_limit = ards.base_addr_low + ards.length_low;
            total_memory         = max(total_memory, segment_limit);
        }
    }

    return total_memory;
}

static inline void *pg_frame_phyaddr(uint32_t entry) {
    return (void *)(entry & 0xfffff000);
}

static phyaddr_t alloc_free_page() {
    static phyaddr_t allocator = LoaderPageTableBase;

    phyaddr_t phyaddr  = allocator;
    allocator         += NUM_4K;

    if (phyaddr >= LoaderPageTableLimit) { phyaddr = INVALID_PHYADDR; }

    return phyaddr;
}

uint32_t *setup_paging() {
    size_t total_memory = get_total_memory();

    //! TODO: unios requires at least 32 MB, otherwise abort here
    if (total_memory < 32 * NUM_1M) {
        while (true) {}
    }

    const int attr_rw_p  = 0x3;
    uint32_t *loader_cr3 = (void *)alloc_free_page();
    size_t    total_pdes = idiv_ceil(total_memory, NUM_4M);

    int       index   = 0;
    int       koffset = idiv_floor(KernelLinBase, NUM_4M);
    phyaddr_t maddr   = 0;
    phyaddr_t laddr   = 0;

    if (total_pdes >= koffset) {
        //! TODO: phy mapping overlaps, that means the given KernelLinBase may
        //! be an unreasonable design
        while (true) {}
    }

    // identical & kernel mapping
    while (maddr < total_memory) {
        uint32_t pde0 = alloc_free_page();
        uint32_t pde1 = alloc_free_page();

        //! NOTE: 2 MB page table is big enough to map 128 MB physical memory,
        //! so if run out of memory occurs during the mapping, we could simple
        //! break it and continue our boot work since 128 MB already meet our
        //! needs currently
        if (pde0 == INVALID_PHYADDR || pde1 == INVALID_PHYADDR) { break; }
        pde0 |= attr_rw_p;
        pde1 |= attr_rw_p;

        //! NOTE: ensure identical and kernel maps the same range
        //! FIXME: unreasonable kernel base assignment may lead the mapping work
        //! to a very embarrassing situation in this proc method , e.g. large
        //! physical memory with a very small range memory mapping
        if (index + koffset >= 1024) { break; }
        loader_cr3[index]           = pde0;
        loader_cr3[index + koffset] = pde1;

        int       total_ptes = min(1024, (total_memory - maddr) / NUM_4K);
        uint32_t *pd0        = pg_frame_phyaddr(pde0);
        uint32_t *pd1        = pg_frame_phyaddr(pde1);

        for (int j = 0; j < total_ptes; ++j, laddr += NUM_4K) {
            pd0[j] = laddr | attr_rw_p;
            pd1[j] = laddr | attr_rw_p;
        }

        maddr += NUM_4M;
        ++index;
    }

    return loader_cr3;
}
