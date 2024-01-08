#include <unios/page.h>
#include <unios/elf.h>
#include <unios/proc.h>
#include <unios/syscall.h>
#include <unios/assert.h>
#include <unios/layout.h>
#include <unios/spinlock.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

static u32 exec_elfcpy(u32 fd, Elf32_Phdr elf_progh, u32 pte_attr) {
    u32  laddr   = elf_progh.p_vaddr;
    u32  llimit  = elf_progh.p_vaddr + elf_progh.p_memsz;
    u32  foffset = elf_progh.p_offset;
    u32  flimit  = elf_progh.p_offset + elf_progh.p_filesz;
    bool ok      = pg_map_laddr_range(
        p_proc_current->pcb.cr3, laddr, llimit, PG_P | PG_U | PG_RWX, pte_attr);
    assert(ok);
    pg_refresh();

    u32 size = MIN(llimit - laddr, flimit - foffset);
    do_lseek(fd, foffset, SEEK_SET);
    do_read(fd, (void*)laddr, size);
    laddr += size;
    memset((void*)laddr, 0, llimit - laddr);

    return 0;
}

static u32 exec_load(
    u32 fd, const Elf32_Ehdr* elf_header, const Elf32_Phdr* elf_proghs) {
    assert(elf_header->e_phnum > 0);

    lin_memmap_t* memmap = &p_proc_current->pcb.memmap;
    u32           cr3    = p_proc_current->pcb.cr3;
    //! cleanup old ph info
    ph_info_t* ph_info = memmap->ph_info;
    while (ph_info != NULL) {
        ph_info_t* next  = ph_info->next;
        u32        laddr = ph_info->base;
        u32        limit = ph_info->limit;
        while (laddr < limit) {
            pg_unmap_laddr(cr3, laddr, true);
            laddr = pg_frame_phyaddr(laddr) + NUM_4K;
        }
        do_free((void*)K_LIN2PHY(ph_info));
        ph_info = next;
    }
    memmap->ph_info = NULL;

    for (int ph_num = 0; ph_num < elf_header->e_phnum; ++ph_num) {
        //! accept PT_LOAD section only now
        if (elf_proghs[ph_num].p_type != 0x1) { continue; }
        assert(elf_proghs[ph_num].p_memsz > 0);

        bool flag_exec  = elf_proghs[ph_num].p_flags & 0b001;
        bool flag_read  = elf_proghs[ph_num].p_flags & 0b100;
        bool flag_write = elf_proghs[ph_num].p_flags & 0b010;
        if (!flag_exec && !flag_read && !flag_write) {
            klog(
                "exec: exec_load: unknown elf program header flags=0x%x",
                elf_proghs[ph_num].p_flags);
            return -1;
        }
        //! TODO: more detailed page privilege
        u32 pte_attr = PG_P | PG_U;
        if (flag_write) {
            pte_attr |= PG_RWX;
        } else {
            pte_attr |= PG_RX;
        }

        exec_elfcpy(fd, elf_proghs[ph_num], pte_attr);

        //! maintenance ph info list
        //! TODO: integrate linked-list ops
        ph_info_t* new_ph_info = K_PHY2LIN(do_kmalloc(sizeof(ph_info_t)));
        new_ph_info->base      = elf_proghs[ph_num].p_vaddr;
        new_ph_info->limit =
            elf_proghs[ph_num].p_vaddr + elf_proghs[ph_num].p_memsz;
        if (memmap->ph_info == NULL) {
            new_ph_info->next   = NULL;
            new_ph_info->before = NULL;
            memmap->ph_info     = new_ph_info;
        } else {
            memmap->ph_info->before = new_ph_info;
            new_ph_info->next       = memmap->ph_info;
            new_ph_info->before     = NULL;
            memmap->ph_info         = new_ph_info;
        }
    }

    return 0;
}

static int exec_pcb_init(const char* path) {
    char* p_regs;
    strncpy(
        p_proc_current->pcb.name, path, sizeof(p_proc_current->pcb.name) - 1);

    pcb_t* pcb = &p_proc_current->pcb;

    pcb->ldts[0].attr0 = DA_C | RPL_USER << 5;
    pcb->ldts[1].attr0 = DA_DRW | RPL_USER << 5;
    pcb->regs.cs     = ((8 * 0) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
    pcb->regs.ds     = ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
    pcb->regs.es     = ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
    pcb->regs.fs     = ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
    pcb->regs.ss     = ((8 * 1) & SA_MASK_RPL & SA_MASK_TI) | SA_TIL | RPL_USER;
    pcb->regs.gs     = (SELECTOR_KERNEL_GS & SA_MASK_RPL) | RPL_USER;
    pcb->regs.eflags = 0x202; //<! IF=1 bit2 is always 1

    u32* frame = (void*)(p_proc_current + 1) - P_STACKTOP;
    memcpy(frame, &pcb->regs, sizeof(pcb->regs));

    lin_memmap_t* memmap     = &p_proc_current->pcb.memmap;
    memmap->vpage_lin_base   = VpageLinBase;
    memmap->vpage_lin_limit  = VpageLinBase;
    memmap->heap_lin_base    = HeapLinBase;
    memmap->heap_lin_limit   = HeapLinBase;
    memmap->stack_lin_base   = StackLinBase;
    memmap->stack_lin_limit  = StackLinBase - 0x4000;
    memmap->arg_lin_base     = ArgLinBase;
    memmap->arg_lin_limit    = ArgLinBase;
    memmap->kernel_lin_base  = KernelLinBase;
    memmap->kernel_lin_limit = KernelLinBase + KernelSize;

    p_proc_current->pcb.tree_info.text_hold = true;
    p_proc_current->pcb.tree_info.data_hold = true;

    return 0;
}

static int open_first_executable(const char* path) {
    assert(path != NULL);
    if (path[0] == '/') { return do_open(path, O_RDWR); }

    char abspath[PATH_MAX] = {};

    const char* env_pwd    = "/orange";
    const char* env_path[] = {
        "/orange",
        NULL,
    };
    const char* env_ext[] = {".bin", NULL};

    int i = -1; //<! search path index
    int j = -1; //<! extension index

    const char* prefix = env_pwd;
    do {
        const char* ext = "";
        do {
            snprintf(abspath, sizeof(abspath), "%s/%s%s", prefix, path, ext);
            int fd = do_open(abspath, O_RDWR);
            if (fd != -1) { return fd; }
            ext = env_ext[++j];
        } while (env_ext[j] != NULL);
        prefix = env_path[++i];
    } while (env_path[i] != NULL);

    return -1;
}

int do_execve(const char* path, char* const* argv, char* const* envp) {
    assert(path != NULL);

    //! FIXME: concurrent issue about io, only lock the whole io ops with one
    //! lock can the execve run safely with multiple instances
    static int io_lock = 0;
    lock_or_schedule(&io_lock);
    u32 fd = open_first_executable(path);
    if (fd == -1) {
        klog("exec: executable not found\n");
        release(&io_lock);
        return -1;
    }

    lock_or_schedule(&p_proc_current->pcb.lock);
    Elf32_Ehdr* elf_header = NULL;
    Elf32_Phdr* elf_proghs = NULL;
    elf_header             = K_PHY2LIN(do_kmalloc(sizeof(Elf32_Ehdr)));
    assert(elf_header != NULL);

    read_Ehdr(fd, elf_header, 0);
    elf_proghs =
        K_PHY2LIN(do_kmalloc(sizeof(Elf32_Phdr) * elf_header->e_phnum));
    assert(elf_proghs != NULL);
    for (int i = 0; i < elf_header->e_phnum; i++) {
        u32 offset = elf_header->e_phoff + i * sizeof(Elf32_Phdr);
        read_Phdr(fd, elf_proghs + i, offset);
    }

    if (exec_load(fd, elf_header, elf_proghs) == -1) {
        do_close(fd);
        do_free((void*)K_LIN2PHY(elf_header));
        do_free((void*)K_LIN2PHY(elf_proghs));
        release(&io_lock);
        return -1;
    }
    release(&io_lock);

    exec_pcb_init(path);

    lin_memmap_t* memmap = &p_proc_current->pcb.memmap;
    bool          ok     = pg_map_laddr_range(
        p_proc_current->pcb.cr3,
        memmap->stack_lin_limit,
        memmap->stack_lin_base,
        PG_P | PG_U | PG_RWX,
        PG_P | PG_U | PG_RWX);
    assert(ok);
    pg_refresh();

    //! FIXME: head not allocated yet

    u32* frame                   = (void*)(p_proc_current + 1) - P_STACKTOP;
    u32* callee_stack            = (u32*)memmap->stack_lin_base;
    callee_stack[-1]             = (u32)0xcafebabe;
    callee_stack[-2]             = (u32)0xbaadf00d;
    callee_stack[-3]             = (u32)1;
    p_proc_current->pcb.regs.esp = (u32)&callee_stack[-3];
    p_proc_current->pcb.regs.eip = elf_header->e_entry;
    frame[NR_EIPREG]             = p_proc_current->pcb.regs.eip;
    frame[NR_ESPREG]             = p_proc_current->pcb.regs.esp;
    p_proc_current->pcb.stat     = READY;
    do_close(fd);
    do_free((void*)K_LIN2PHY(elf_header));
    do_free((void*)K_LIN2PHY(elf_proghs));
    release(&p_proc_current->pcb.lock);
    return 0;
}
