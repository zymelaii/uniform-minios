#include <type.h>
#include <elf.h>
#include <unios/syscall.h>
#include <assert.h>
#include <stdio.h>
#include <proc.h>
#include <global.h>
#include <const.h>
#include <proto.h>
#include <string.h>

static u32 exec_elfcpy(u32 fd, Elf32_Phdr Echo_Phdr, u32 attribute) {
    u32 laddr   = Echo_Phdr.p_vaddr;
    u32 llimit  = Echo_Phdr.p_vaddr + Echo_Phdr.p_memsz;
    u32 foffset = Echo_Phdr.p_offset;
    u32 flimit  = Echo_Phdr.p_offset + Echo_Phdr.p_filesz;

    u32 pg_addr  = laddr & 0xfffff000;
    u32 pg_bound = (llimit + 0xfff) & 0xfffff000;
    while (pg_addr < pg_bound) {
        lin_mapping_phy(
            pg_addr,
            MAX_UNSIGNED_INT,
            p_proc_current->task.pid,
            PG_P | PG_USU | PG_RWW,
            attribute);
        pg_addr += 0x1000;
    }

    u32 size = min(llimit - laddr, flimit - foffset);
    do_lseek(fd, foffset, SEEK_SET);
    do_read(fd, (void*)laddr, size);
    laddr += size;
    memset((void*)laddr, 0, llimit - laddr);

    return 0;
}

static u32 exec_load(
    u32 fd, const Elf32_Ehdr* elf_header, const Elf32_Phdr* elf_proghs) {
    assert(elf_header->e_phnum > 0);

    LIN_MEMMAP* memmap = &p_proc_current->task.memmap;

    //! cleanup old ph info
    PH_INFO* ph_info = memmap->ph_info;
    while (ph_info != NULL) {
        PH_INFO* next = ph_info->next;
        do_free(ph_info);
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
            trace_logging(
                "exec: exec_load: unknown elf program header flags=0x%x",
                elf_proghs[ph_num].p_flags);
            return -1;
        }
        //! TODO: more detailed page privilege
        u32 pte_attr = PG_P | PG_USU;
        if (flag_write) {
            pte_attr |= PG_RWW;
        } else {
            pte_attr |= PG_RWR;
        }
        //! FIXME: well, here if i do not set PG_RWW i can't pass something with
        //! bss segments
        pte_attr |= PG_RWW;

        exec_elfcpy(fd, elf_proghs[ph_num], pte_attr);

        //! maintenance ph info list
        //! TODO: integrate linked-list ops
        PH_INFO* new_ph_info       = do_kmalloc(sizeof(PH_INFO));
        new_ph_info->lin_addr_base = elf_proghs[ph_num].p_vaddr;
        new_ph_info->lin_addr_limit =
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

static int exec_pcb_init(char* path) {
    char* p_regs;
    strncpy(
        p_proc_current->task.p_name,
        path,
        sizeof(p_proc_current->task.p_name) - 1);

    PROCESS_0* task     = &p_proc_current->task;
    task->stat          = READY;
    task->ldts[0].attr1 = DA_C | PRIVILEGE_USER << 5;
    task->ldts[1].attr1 = DA_DRW | PRIVILEGE_USER << 5;
    task->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
    task->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
    task->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
    task->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
    task->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_USER;
    task->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_USER;
    task->regs.eflags = 0x202; //<! IF=1 bit2 is always 1

    u32* frame = (void*)(p_proc_current + 1) - P_STACKTOP;
    memcpy(frame, &task->regs, sizeof(task->regs));

    LIN_MEMMAP* memmap        = &p_proc_current->task.memmap;
    memmap->vpage_lin_base    = VpageLinBase;
    memmap->vpage_lin_limit   = VpageLinBase;
    memmap->heap_lin_base     = HeapLinBase;
    memmap->heap_lin_limit    = HeapLinBase;
    memmap->stack_child_limit = StackLinLimitMAX;
    memmap->stack_lin_base    = StackLinBase;
    memmap->stack_lin_limit   = StackLinBase - 0x4000;
    memmap->arg_lin_base      = ArgLinBase;
    memmap->arg_lin_limit     = ArgLinBase;
    memmap->kernel_lin_base   = KernelLinBase;
    memmap->kernel_lin_limit  = KernelLinBase + KernelSize;

    p_proc_current->task.info.text_hold = 1;
    p_proc_current->task.info.data_hold = 1;

    return 0;
}

int do_exec(char* path) {
    assert(path != NULL);

    u32 fd = do_open(path, O_RDWR);
    if (fd == -1) {
        trace_logging("exec: executable not found");
        return -1;
    }

    Elf32_Ehdr* elf_header = NULL;
    Elf32_Phdr* elf_proghs = NULL;
    elf_header             = do_kmalloc(sizeof(Elf32_Ehdr));
    assert(elf_header != NULL);
    read_Ehdr(fd, elf_header, 0);
    elf_proghs = do_kmalloc(sizeof(Elf32_Phdr) * elf_header->e_phnum);
    assert(elf_proghs != NULL);
    for (int i = 0; i < elf_header->e_phnum; i++) {
        u32 offset = elf_header->e_phoff + i * sizeof(Elf32_Phdr);
        read_Phdr(fd, elf_proghs + i, offset);
    }

    //! FIXME: elf_header->e_phnum becomes 0 after some more exec calls

    if (exec_load(fd, elf_header, elf_proghs) == -1) {
        do_close(fd);
        do_free(elf_header);
        do_free(elf_proghs);
        return -1;
    }

    exec_pcb_init(path);

    LIN_MEMMAP* memmap            = &p_proc_current->task.memmap;
    u32*        frame             = (void*)(p_proc_current + 1) - P_STACKTOP;
    p_proc_current->task.regs.eip = elf_header->e_entry;
    p_proc_current->task.regs.esp = memmap->stack_lin_base;
    frame[NR_EIPREG]              = p_proc_current->task.regs.eip;
    frame[NR_ESPREG]              = p_proc_current->task.regs.esp;

    //! FIXME: exceeded stack mapping, see kernel main
    for (u32 laddr = memmap->stack_lin_base; laddr > memmap->stack_lin_limit;
         laddr     -= num_4K) {
        int ok = lin_mapping_phy(
            laddr,
            MAX_UNSIGNED_INT,
            p_proc_current->task.pid,
            PG_P | PG_USU | PG_RWW,
            PG_P | PG_USU | PG_RWW);
        if (ok != 0) {
            trace_logging("exec: pagetbl mapping failed");
            return -1;
        }
    }

    //! FIXME: head not allocated yet

    do_close(fd);
    do_free(elf_header);
    do_free(elf_proghs);

    return 0;
}
