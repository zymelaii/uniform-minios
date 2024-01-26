#include <unios/elf.h>
#include <fs/fat.h>
#include <arch/x86.h>
#include <config.h>
#include <limits.h>
#include <stddef.h>

static void *memset(void *v, int c, size_t n) {
    char *p;
    int   m;

    p = v;
    m = n;
    while (--m >= 0) *p++ = c;

    return v;
}

static void *memcpy(void *dst, const void *src, size_t n) {
    const char *s;
    char       *d;

    s = src;
    d = dst;

    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0) *--d = *--s;
    } else {
        while (n-- > 0) *d++ = *s++;
    }

    return dst;
}

static int strncmp(const char *p, const char *q, size_t n) {
    while (n > 0 && *p && *p == *q) n--, p++, q++;
    if (n == 0)
        return 0;
    else
        return (int)((unsigned char)*p - (unsigned char)*q);
}

static void waitdisk(void) {
    // wait for disk ready
    while ((inb(0x1F7) & 0xC0) != 0x40) /* do nothing */
        ;
}

#define SECTSIZE      512
#define BUF_CLUS_ADDR ((void *)0x20000)
#define BUF_PHDR_ADDR ((void *)0x30000)
#define BUF_DATA_ADDR ((void *)0x40000)
#define ELF_ADDR      ((void *)0x50000)

extern uint32_t PartitionLBA;

/**
 * @brief read a sector from hd0
 *
 * @param dst the destination of data to load
 * @param sector the LBA of sector to read
 */
static void readsect(void *dst, uint32_t sector) {
    // wait for disk to be ready
    waitdisk();

    sector += PartitionLBA;
    outb(0x1F2, 1); // count = 1
    outb(0x1F3, sector);
    outb(0x1F4, sector >> 8);
    outb(0x1F5, sector >> 16);
    outb(0x1F6, (sector >> 24) | 0xE0);
    outb(0x1F7, 0x20); // cmd 0x20 - read sectors

    // wait for disk to be ready
    waitdisk();

    // read a sector
    insl(0x1F0, dst, SECTSIZE / 4);
}

uint32_t    fat_start_sec;
uint32_t    data_start_sec;
uint32_t    elf_clus;
uint32_t    clus_bytes;
fat32_bpb_t bpb;

/**
 * @brief Get the next cluster number
 *
 * @param clus current cluster number
 * @return uint32_t next cluster number
 */
static uint32_t get_next_clus(uint32_t clus) {
    uint32_t        sec         = clus * 4 / SECTSIZE;
    uint32_t        off         = clus * 4 % SECTSIZE;
    static uint32_t fat_now_sec = 0;
    if (fat_now_sec != fat_start_sec + sec) {
        readsect(BUF_CLUS_ADDR, fat_start_sec + sec);
        fat_now_sec = fat_start_sec + sec;
    }
    return *(uint32_t *)(BUF_CLUS_ADDR + off);
}

/**
 * @brief According to current cluster, read corresponding
 * sectors to destination address
 *
 * @param dst the destination of data to load
 * @param clus cluster number
 * @return void* after reading, return next address can read
 */
static void *read_clus(void *dst, uint32_t clus) {
    uint32_t base_sector;

    base_sector = (clus - 2) * bpb.BPB_SecPerClus;
    base_sector += data_start_sec;

    for (int i = 0; i < bpb.BPB_SecPerClus; i++, dst += SECTSIZE)
        readsect(dst, base_sector + i);
    return dst;
}

/**
 * @brief According to offset in elf, get corresponding cluster number
 *
 * @param offset offset in elf
 * @return uint32_t cluster number
 */
static uint32_t get_clus(uint32_t offset) {
    uint32_t elf_offset = ROUNDDOWN(offset, clus_bytes);

#define CLUS_CACHE_SIZE 4
    // (elf_offset, cluster)
    static uint32_t elf_offset_cache[CLUS_CACHE_SIZE][2] = {
        {U32_MAX, U32_MAX},
        {U32_MAX, U32_MAX},
        {U32_MAX, U32_MAX},
        {U32_MAX, U32_MAX},
    };
    int      hit  = -1;
    uint32_t clus = 0;
    for (int i = 0; i < CLUS_CACHE_SIZE; i++) {
        if (elf_offset_cache[i][0] == elf_offset) {
            clus = elf_offset_cache[i][1];
            hit  = i;
            break;
        }
    }
    if (hit != -1) {
        //! LRU
        while (hit) {
            elf_offset_cache[hit][0] = elf_offset_cache[hit - 1][0];
            elf_offset_cache[hit][1] = elf_offset_cache[hit - 1][1];
            hit--;
        }
        elf_offset_cache[0][0] = elf_offset;
        elf_offset_cache[0][1] = clus;
    } else {
        uint32_t new_elf_offset = 0;
        uint32_t new_clus       = elf_clus;
        for (int i = 0; i < CLUS_CACHE_SIZE; i++) {
            if (elf_offset_cache[i][0] < elf_offset
                && elf_offset_cache[i][0] >= new_elf_offset) {
                new_elf_offset = elf_offset_cache[i][0];
                new_clus       = elf_offset_cache[i][1];
            }
        }

        for (int i = CLUS_CACHE_SIZE - 1; i >= 1; i--) {
            elf_offset_cache[i][0] = elf_offset_cache[i - 1][0];
            elf_offset_cache[i][1] = elf_offset_cache[i - 1][1];
        }

        while (new_elf_offset < elf_offset) {
            new_elf_offset += clus_bytes;
            new_clus       = get_next_clus(new_clus);
        }
        elf_offset_cache[0][0] = new_elf_offset;
        elf_offset_cache[0][1] = new_clus;
        clus                   = new_clus;
    }
    return clus;
}

/**
 * @brief read a segment from hd0, load data [offset, offset + count) (hd0)
 * to [va, va + offset) (os)
 *
 * @param va address to read
 * @param count bytes to read
 * @param offset offset in hd0
 */
void readseg(void *va, uint32_t count, uint32_t offset) {
    uint32_t end_offset = offset + count;
    if (offset % clus_bytes != 0) {
        uint32_t clus = get_clus(offset);
        read_clus(BUF_DATA_ADDR, clus);
        uint32_t n = MIN(end_offset, ROUNDUP(offset, clus_bytes)) - offset;
        va         = memcpy(va, BUF_DATA_ADDR + offset % clus_bytes, n);
        offset     += n;
    }
    while (end_offset - offset >= clus_bytes) {
        uint32_t clus = get_clus(offset);
        va            = read_clus(va, clus);
        offset        += clus_bytes;
    }
    if (offset < end_offset) {
        uint32_t clus = get_clus(offset);
        read_clus(BUF_DATA_ADDR, clus);
        uint32_t n = end_offset - offset;
        va         = memcpy(va, BUF_DATA_ADDR, n);
        offset     += n;
    }
}

/**
 * @brief Get the phdr of elf
 *
 * @param eh pointer to ehdr
 * @param i index of phdr
 * @return Elf32_Phdr* pointer to phdr
 */
static Elf32_Phdr *get_phdr(Elf32_Ehdr *eh, int i) {
    static uint32_t clus   = 0;
    uint32_t        offset = eh->e_phoff + i * eh->e_phentsize;
    if (clus != get_clus(offset)) {
        clus = get_clus(offset);
        read_clus(BUF_PHDR_ADDR, clus);
        read_clus(BUF_PHDR_ADDR + clus_bytes, get_next_clus(clus));
    }
    return BUF_PHDR_ADDR + offset % clus_bytes;
}

//! TODO: change it to real macro
#define PT_LOAD 1

void load_and_enter_kernel(void) {
    readsect((void *)&bpb, 0);

    fat_start_sec  = bpb.BPB_RsvdSecCnt;
    data_start_sec = fat_start_sec + bpb.BPB_FATSz32 * bpb.BPB_NumFATs;
    clus_bytes     = (uint32_t)bpb.BPB_SecPerClus * bpb.BPB_BytsPerSec;

    uint32_t root_clus = bpb.BPB_RootClus;

    while (root_clus < 0x0FFFFFF8) {
        fat32_entry_t *buf_end = read_clus(BUF_DATA_ADDR, root_clus);
        for (fat32_entry_t *p = BUF_DATA_ADDR; p < buf_end; p++) {
            if (strncmp((void *)p->name, KERNEL_NAME_IN_FAT, 11) == 0) {
                elf_clus = (u32)p->start_clus_hi << 16 | p->start_clus_lo;
                break;
            }
        }
        if (elf_clus != 0) break;
        root_clus = get_next_clus(root_clus);
    }

    read_clus(ELF_ADDR, elf_clus);

    Elf32_Ehdr *eh = ELF_ADDR;
    for (int i = 0; i < eh->e_phnum; i++) {
        Elf32_Phdr *ph = get_phdr(eh, i);
        if (ph->p_type != PT_LOAD) continue;
        readseg((void *)ph->p_vaddr, ph->p_filesz, ph->p_offset);
        memset(
            (void *)ph->p_vaddr + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
    }

    if (eh->e_entry == 0) {
        //! TODO: something error with kernel, abort
        while (true) {}
    }

    ((void (*)())eh->e_entry)();
    __builtin_unreachable();
}
