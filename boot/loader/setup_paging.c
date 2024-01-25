#include <unios/layout.h>
#include <arch/device.h>
#include <config.h>
#include <stddef.h>

static size_t get_total_memory() {
    size_t         total_memory = 0;
    device_info_t *device_info  = (void *)DEVICE_INFO_ADDR;

    for (int i = 0; i < device_info->ards_count; i++) {
        ards_t ards = device_info->ards_buffer[i];
        if (ards.type == 1) {
            size_t segment_limit = ards.base_addr_low + ards.length_low;
            total_memory         = MAX(total_memory, segment_limit);
        }
    }

    return total_memory;
}

static inline uint32_t *pg_frame_phyaddr(u32 entry) {
    return (void *)(entry & 0xfffff000);
}

static uint32_t *alloc_free_page(void) {
    static size_t allocator = NUM_1M;
    uint32_t     *res;

    res       = (void *)allocator;
    allocator += NUM_4K;

    //! TODO: if allocator can't alloc more free pages, print some error message
    //! and then be in a dead loop
    while ((void *)res >= (void *)(2 * NUM_1M)) {}

    return res;
}

uint32_t *setup_paging(void) {
    size_t total_memory = get_total_memory();
    total_memory        = MIN(total_memory, 128 * NUM_1M);
    // align to size of pde (4MB)
    total_memory = ROUNDUP(total_memory, NUM_4M);

    uint32_t *loader_cr3 = alloc_free_page();
    uint32_t  pde_num    = total_memory / NUM_4M;
    // identical mapping
    for (uint32_t i = 0, mapping_addr = 0; i < pde_num; i++) {
        uint32_t pde  = (uint32_t)alloc_free_page() | 0x3;
        loader_cr3[i] = pde;
        uint32_t *pd  = pg_frame_phyaddr(pde);
        for (uint32_t j = 0; j < 1024; j++) {
            pd[j]        = mapping_addr | 0x3;
            mapping_addr += NUM_4K;
        }
    }
    // kernel mapping
    uint32_t kernel_pde_offset = KernelLinBase / NUM_4M;
    for (uint32_t i = 0, mapping_addr = 0; i < pde_num; i++) {
        uint32_t  pde = (uint32_t)alloc_free_page() | 0x3;
        uint32_t *pd  = pg_frame_phyaddr(pde);

        loader_cr3[i + kernel_pde_offset] = pde;
        for (uint32_t j = 0; j < 1024; j++) {
            pd[j]        = mapping_addr | 0x3;
            mapping_addr += NUM_4K;
        }
    }
    return loader_cr3;
}
