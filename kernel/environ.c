#include <unios/environ.h>
#include <unios/proc.h>
#include <unios/page.h>
#include <unios/assert.h>
#include <unios/syscall.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

char **kgetenvp(process_t *proc) {
    if (proc == NULL) { return NULL; }

    lin_memmap_t *memmap = &proc->pcb.memmap;
    if (!pg_addr_pte_exist(proc->pcb.cr3, memmap->arg_lin_base)) {
        return NULL;
    }

    char **arg = (char **)memmap->arg_lin_base;
    while (*arg != NULL) {
        if (arg[1] == NULL) { break; }
        ++arg;
    }

    char **envp = NULL;
    if (*arg == NULL) {
        envp = &arg[1];
    } else {
        uintptr_t addr = (u32)*arg + strlen(*arg) + 1;
        //! align to 4 bytes ~ sizeof pointer
        envp = (void *)((addr + 3) & ~0b11);
    }
    return envp;
}

static void *do_getenv() {
    char **envp    = kgetenvp(p_proc_current);
    int    nr_envs = 0;
    char **env     = envp;
    while (*env != NULL) {
        ++env;
        ++nr_envs;
    }

    char  *env_base  = (char *)&env[1];
    size_t size_envs = 0;
    if (nr_envs > 0) { size_envs = env[-1] - env_base + strlen(env[-1]) + 1; }
    size_t size_envp = (nr_envs + 1) * sizeof(void *) + size_envs;

    char **envp_dup = do_malloc(size_envp);
    if (envp_dup == NULL) { return NULL; }

    envp_dup[nr_envs] = NULL;
    memcpy(&envp_dup[nr_envs + 1], env_base, size_envs);
    for (int i = 0; i < nr_envs; ++i) {
        ptrdiff_t offset = (void *)envp[i] - (void *)envp;
        envp_dup[i]      = (void *)envp_dup + offset;
    }
    return envp_dup;
}

static bool do_putenv(char **envp) {
    pcb_t        *pcb    = &p_proc_current->pcb;
    phyaddr_t     cr3    = pcb->cr3;
    lin_memmap_t *memmap = &pcb->memmap;

    char **dst_envp = kgetenvp(p_proc_current);
    int    nr_envs  = 0;
    char **env      = envp;
    while (*env != NULL) {
        ++env;
        ++nr_envs;
    }

    char  *limit    = (void *)memmap->arg_lin_limit;
    char  *env_base = (char *)(dst_envp + nr_envs + 1);
    size_t len      = 0;
    for (int i = 0; i < nr_envs; ++i) { len += strlen(envp[i]) + 1; }
    if (env_base + len >= limit) { return false; }

    bool ok = pg_map_laddr_range(
        cr3,
        (u32)dst_envp,
        (u32)(env_base + len),
        PG_P | PG_U | PG_RWX,
        PG_P | PG_U | PG_RWX);
    assert(ok);
    pg_refresh();

    char *p = env_base;
    for (int i = 0; i < nr_envs; ++i) {
        dst_envp[i] = p;
        char *q     = envp[i];
        while (*q != '\0') { *p++ = *q++; }
        *p++ = '\0';
    }
    dst_envp[nr_envs] = NULL;

    return true;
}

bool do_environ(int op, char *const **p_envp) {
    if (p_envp == NULL) { return false; }
    if (op != ENVIRON_GET && op != ENVIRON_PUT) { return false; }

    pcb_t        *pcb    = &p_proc_current->pcb;
    phyaddr_t     cr3    = pcb->cr3;
    lin_memmap_t *memmap = &pcb->memmap;

    char **argv        = (char **)memmap->arg_lin_base;
    bool   should_init = false;

    //! NOTE: memory for args not allocated, neither do argv, that is, argv
    //! is empty
    if (!pg_addr_pte_exist(cr3, (u32)argv)) {
        //! FIXME: a empty argv usually indicates that the current proc is
        //! manually inited during kernel initialization, and we decide to make
        //! argc=0 in this case, maybe there is a better solution
        bool ok = pg_map_laddr(
            cr3,
            (u32)argv,
            PG_INVALID,
            PG_P | PG_U | PG_RWX,
            PG_P | PG_U | PG_RWX);
        assert(ok);
        pg_refresh();
        argv[0]     = NULL;
        should_init = true;
    }

    char **envp = kgetenvp(p_proc_current);
    if (should_init) {
        assert(pg_addr_pte_exist(cr3, (u32)envp));
        envp[0] = NULL;
    }

    bool ok = false;

    if (op == ENVIRON_GET) {
        void *resp = do_getenv();
        if (resp != NULL) { *p_envp = resp; }
        return resp != NULL;
    }

    if (op == ENVIRON_PUT) { return do_putenv((char **)*p_envp); }

    //! TODO: switch PG_RWX to PG_RX after ops done

    panic("unreachable");
}
