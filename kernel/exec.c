#include <unios/page.h>
#include <unios/elf.h>
#include <unios/proc.h>
#include <unios/syscall.h>
#include <unios/assert.h>
#include <unios/layout.h>
#include <unios/memory.h>
#include <unios/schedule.h>
#include <unios/environ.h>
#include <sys/errno.h>
#include <stdio.h>
#include <atomic.h>
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

    ph_info_t* ph_info = memmap->ph_info;
    while (ph_info != NULL) {
        ph_info_t* next = ph_info->next;
        bool       ok =
            pg_unmap_laddr_range(cr3, ph_info->base, ph_info->limit, true);
        assert(ok);
        kfree(ph_info);
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
        ph_info_t* new_ph_info = kmalloc(sizeof(ph_info_t));
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
    pcb->regs.eflags = EFLAGS_RESERVED | EFLAGS_IF | EFLAGS_IOPL(0);

    u32* frame = (void*)(p_proc_current + 1) - P_STACKTOP;
    memcpy(frame, &pcb->regs, sizeof(pcb->regs));

    lin_memmap_t* memmap     = &p_proc_current->pcb.memmap;
    memmap->vpage_lin_base   = VpageLinBase;
    memmap->vpage_lin_limit  = VpageLinBase;
    memmap->heap_lin_base    = HeapLinBase;
    memmap->stack_lin_base   = StackLinBase;
    memmap->stack_lin_limit  = StackLinBase - 0x4000;
    memmap->arg_lin_base     = ArgLinBase;
    memmap->arg_lin_limit    = ArgLinLimitMAX;
    memmap->kernel_lin_base  = KernelLinBase;
    memmap->kernel_lin_limit = KernelLinBase + KernelSize;

    //! NOTE: keep pages for heap and only reset allocator, so that we can
    //! reduce the overhead of page table updates at the cost of a certain
    //! memory footprint
    mballoc_reset_unsafe(p_proc_current->pcb.allocator);

    p_proc_current->pcb.tree_info.text_hold = true;
    p_proc_current->pcb.tree_info.data_hold = true;

    return 0;
}

static const char* next_env_item(const char* env_list) {
    const char* p = env_list;
    while (*p != '\0' && *p != ';') { ++p; }
    if (*p == ';') { ++p; }
    return p;
}

static int try_open_executable(const char* path) {
    assert(path != NULL);
    if (path[0] == '/') { return do_open(path, O_RDWR); }

    char* const* envp = kgetenvp(p_proc_current);
    if (envp == NULL) { return -1; }

    //! `PWD` current working directory, first search path
    const char* env_pwd = NULL;
    //! `PATH` searching paths
    const char* env_path = NULL;
    //! `PATH_EXT` auto detected extensions for executables
    const char* env_ext = NULL;

    //! NOTE: if multiple env found, simply retrive the first record
    char* const* env_ptr  = envp;
    int          got_envs = 0;
    while (*env_ptr != NULL) {
        if (env_pwd == NULL && strncmp(*env_ptr, "PWD=", 4) == 0) {
            env_pwd = *env_ptr + 4;
            ++got_envs;
        } else if (env_path == NULL && strncmp(*env_ptr, "PATH=", 5) == 0) {
            env_path = *env_ptr + 5;
            ++got_envs;
        } else if (env_ext == NULL && strncmp(*env_ptr, "PATH_EXT=", 9) == 0) {
            env_ext = *env_ptr + 9;
            ++got_envs;
        }
        if (got_envs == 3) { break; }
        ++env_ptr;
    }

    char abspath[PATH_MAX] = {};

    const char* prefix   = env_pwd;
    const char* next_pre = next_env_item(prefix);
    do {
        int         len_pre   = next_pre - prefix - (*next_pre == ';');
        const char* suffix    = env_ext;
        const char* next_suf  = next_env_item(prefix);
        bool        once_more = true;
        while (true) {
            int len_suf = next_suf - suffix - (*next_suf == ';');
            snprintf(
                abspath,
                sizeof(abspath),
                "%*s/%s%*s",
                len_pre,
                len_pre == 0 ? "" : prefix,
                path,
                len_suf,
                len_suf == 0 ? "" : suffix);
            int fd = do_open(abspath, O_RDWR);
            if (fd != -1) { return fd; }
            suffix = next_suf;
            if (*suffix == '\0') {
                if (once_more) {
                    once_more = false;
                } else {
                    break;
                }
            }
        }
        prefix = next_pre;
    } while (*prefix != '\0');

    return -1;
}

static void* exec_read_and_load(int fd) {
    Elf32_Ehdr* elf_header = kmalloc(sizeof(Elf32_Ehdr));
    assert(elf_header != NULL);
    read_Ehdr(fd, elf_header, 0);

    Elf32_Phdr* elf_proghs = kmalloc(sizeof(Elf32_Phdr) * elf_header->e_phnum);
    assert(elf_proghs != NULL);

    for (int i = 0; i < elf_header->e_phnum; i++) {
        u32 offset = elf_header->e_phoff + i * sizeof(Elf32_Phdr);
        read_Phdr(fd, elf_proghs + i, offset);
    }

    void* entry_point = (void*)elf_header->e_entry;
    int   resp        = exec_load(fd, elf_header, elf_proghs);

    kfree(elf_header);
    kfree(elf_proghs);

    return resp == 0 ? entry_point : NULL;
}

static int count_flatten_ptr_array(void** array) {
    if (array == NULL) { return -1; }
    void** p = array;
    int    n = 0;
    while (*p++ != NULL) { ++n; }
    return n;
}

static int sizeof_flatten_str_array(int total, char* const* array) {
    assert(total == 0 || array != NULL);
    int size = (total + 1) * sizeof(void*);
    for (int i = 0; i < total; ++i) { size += strlen(array[i]) + 1; }
    return size;
}

static void fix_broken_flatten_str_array(int total, char** array) {
    assert(array != NULL);
    char* str = (char*)(array + total + 1);
    for (int i = 0; i < total; ++i) {
        array[i] = str;
        str      += strlen(str) + 1;
    }
    array[total] = NULL;
}

static int take_consecutive_laddr_range_ptes(
    u32 cr3, u32 base, u32 limit, u32** p_ptes) {
    assert(p_ptes != NULL);
    *p_ptes = NULL;

    int total = 0;
    base      = pg_frame_phyaddr(base);
    limit     = pg_frame_phyaddr(limit + NUM_4K - 1);
    for (int addr = base; addr < limit; addr += NUM_4K) {
        if (!pg_addr_pte_exist(cr3, addr)) { break; }
        ++total;
    }

    if (total == 0) {
        *p_ptes = NULL;
        return 0;
    }

    u32* ptes = kmalloc(total * sizeof(u32));
    if (ptes == NULL) { return -ENOMEM; }

    for (int addr = base, i = 0; i < total; addr += NUM_4K, ++i) {
        u32 pde = pg_pde(cr3, addr);
        assert((pde & PG_MASK_P) == PG_P);
        u32* p_pte = pg_pte_ptr(pde, addr);
        assert(p_pte != NULL);
        assert((*p_pte & PG_MASK_P) == PG_P);
        ptes[i] = *p_pte;
        *p_pte  = 0;
    }

    *p_ptes = ptes;
    return total;
}

static int exec_replace_argv_and_envp(
    const char* path,
    int*        p_argc,
    char***     p_argv,
    char***     p_envp,
    u32**       p_old_ptes,
    int*        p_nr_old_ptes) {
    assert(path != NULL);
    assert(p_argc != NULL);
    assert(p_argv != NULL);
    assert(p_envp != NULL);
    assert(p_old_ptes != NULL);
    assert(p_nr_old_ptes != NULL);
    *p_old_ptes    = NULL;
    *p_nr_old_ptes = -1;

    char* const* argv = *p_argv;
    char* const* envp = *p_envp;

    int  nr_arg      = count_flatten_ptr_array((void**)argv);
    int  nr_env      = count_flatten_ptr_array((void**)envp);
    bool derive_envp = false;

    int argc = (nr_arg == -1 ? 0 : nr_arg) + 1;
    if (nr_env == -1) {
        bool ok = do_environ(ENVIRON_GET, &envp);
        assert(ok);
        nr_env      = count_flatten_ptr_array((void**)envp);
        derive_envp = true;
    }
    assert(nr_env != -1);

    int size_argv = sizeof_flatten_str_array(argc - 1, argv);
    size_argv     += sizeof(void*) + strlen(path) + 1;
    //! align to 4 bytes ~ pointer size
    size_argv = (size_argv + 3) & ~0b11;

    int size_envp = sizeof_flatten_str_array(nr_env, envp);
    assert(size_envp >= sizeof(void*));

    int total_size = size_argv + size_envp;

    pcb_t*        pcb    = &p_proc_current->pcb;
    lin_memmap_t* memmap = &pcb->memmap;
    if (total_size > memmap->arg_lin_limit - memmap->arg_lin_base) {
        return -E2BIG;
    }

    //! WARNING: if envp derived from the current, content of envp must be
    //! cloned before unmapping the old pages
    char** src_envp = (char**)envp;
    if (derive_envp) {
        src_envp = kmalloc(size_envp);
        if (src_envp == NULL) { return -ENOMEM; }
        memcpy(src_envp, envp, size_envp);
        fix_broken_flatten_str_array(nr_env, src_envp);
    }

    int resp = take_consecutive_laddr_range_ptes(
        pcb->cr3, memmap->arg_lin_base, memmap->arg_lin_limit, p_old_ptes);
    if (resp < 0) {
        if (derive_envp) { kfree(src_envp); }
        return resp;
    }
    *p_nr_old_ptes = resp;
    assert(*p_nr_old_ptes == 0 || *p_old_ptes != NULL);

    do {
        bool ok = pg_map_laddr_range(
            pcb->cr3,
            memmap->arg_lin_base,
            memmap->arg_lin_base + total_size,
            PG_P | PG_U | PG_RWX,
            PG_P | PG_U | PG_RWX);
        if (ok) { break; }
        ok = pg_unmap_laddr_range(
            pcb->cr3,
            memmap->arg_lin_base,
            memmap->arg_lin_base + total_size,
            true);
        assert(ok);
        if (derive_envp) { kfree(src_envp); }
        return -ENOMEM;
    } while (0);
    pg_refresh();

    char** dst_argv  = (char**)memmap->arg_lin_base;
    dst_argv[argc]   = NULL;
    char*  arg       = (char*)&dst_argv[argc + 1];
    char** dst_envp  = (char**)(memmap->arg_lin_base + size_argv);
    dst_envp[nr_env] = NULL;
    char* env        = (char*)&dst_envp[nr_env + 1];

    dst_argv[0] = arg;
    strcpy(arg, path);
    arg += strlen(arg) + 1;
    for (int i = 1; i < argc; ++i) {
        dst_argv[i] = arg;
        strcpy(arg, argv[i - 1]);
        arg += strlen(arg) + 1;
    }

    for (int i = 0; i < nr_env; ++i) {
        dst_envp[i] = env;
        strcpy(env, src_envp[i]);
        env += strlen(env) + 1;
    }

    if (derive_envp) { kfree(src_envp); }

    *p_argc = argc;
    *p_argv = dst_argv;
    *p_envp = dst_envp;

    return 0;
}

int do_execve(const char* path, char* const* argv, char* const* envp) {
    int  errno        = 0;
    u32* old_ptes     = NULL;
    int  nr_old_ptes  = 0;
    int  fd           = -1;
    bool unrestorable = false;

    pcb_t*        pcb    = &p_proc_current->pcb;
    lin_memmap_t* memmap = &pcb->memmap;
    lock_or(&pcb->lock, schedule);

    do {
        if (path == NULL) {
            errno = EFAULT;
            break;
        }

        u32 fd = try_open_executable(path);
        if (fd == -1) {
            errno = ENOENT;
            break;
        }

        int    new_argc = 0;
        char** new_argv = (void*)argv;
        char** new_envp = (void*)envp;
        int    resp     = exec_replace_argv_and_envp(
            path, &new_argc, &new_argv, &new_envp, &old_ptes, &nr_old_ptes);
        if (resp != 0) {
            errno = -resp;
            break;
        }

        //! ATTENTION: from here, subsequent operations will massively
        //! modify the pcb's data and page tables, and the recovery of
        //! errors will also be extremely costly. therefore, we directly
        //! mark the errors that occur in the future as unrecoverable
        //! errors. when these errors occur, execve directly panic and does
        //! not return
        //! TODO: introduec a more advanced architecture to solve this kind
        //! of problem
        unrestorable = true;

        void* entry_point = exec_read_and_load(fd);
        do_close(fd);
        fd = -1;
        if (entry_point == NULL) {
            errno = ENOEXEC;
            break;
        }

        exec_pcb_init(path);

        //! FIXME: may be there's no need to remap stack?
        //! FIXME: potential phy page leak
        bool ok = pg_map_laddr_range(
            pcb->cr3,
            memmap->stack_lin_limit,
            memmap->stack_lin_base,
            PG_P | PG_U | PG_RWX,
            PG_P | PG_U | PG_RWX);
        if (!ok) {
            errno = ENOMEM;
            break;
        }
        pg_refresh();

        u32* frame        = (void*)(p_proc_current + 1) - P_STACKTOP;
        u32* callee_stack = (u32*)memmap->stack_lin_base;
        callee_stack[-1]  = (u32)new_envp;
        callee_stack[-2]  = (u32)new_argv;
        callee_stack[-3]  = (u32)new_argc;
        pcb->regs.esp     = (u32)&callee_stack[-3];
        pcb->regs.eip     = (u32)entry_point;
        frame[NR_EIPREG]  = pcb->regs.eip;
        frame[NR_ESPREG]  = pcb->regs.esp;
    } while (0);

    if (fd != -1) { do_close(fd); }

    if (errno != 0 && unrestorable) { panic("execve: unrestorable error"); }

    bool have_old_ptes = nr_old_ptes > 0 && old_ptes != NULL;
    if (errno != 0 && have_old_ptes) {
        bool ok = pg_unmap_laddr_range(
            pcb->cr3, memmap->arg_lin_base, memmap->arg_lin_limit, true);
        assert(ok);
        u32 addr = memmap->arg_lin_base;
        for (u32 i = 0; i < nr_old_ptes; ++i, addr += NUM_4K) {
            u32 pde = pg_pde(pcb->cr3, addr);
            assert((pde & PG_MASK_P) == PG_P);
            u32* pte_ptr = pg_pte_ptr(pde, addr);
            *pte_ptr     = old_ptes[i];
        }
        pg_refresh();
    }
    if (have_old_ptes && errno == 0) {
        for (int i = 0; i < nr_old_ptes; ++i) {
            free_phypage(pg_frame_phyaddr(old_ptes[i]));
        }
    }
    if (have_old_ptes) { kfree(old_ptes); }

    p_proc_current->pcb.stat = READY;
    release(&p_proc_current->pcb.lock);
    if (errno != 0) { klog("exec: caught %s", strerrno(errno)); }
    return -errno;
}
