#include <unios/proc.h>
#include <unios/tracing.h>
#include <unios/hd.h>
#include <unios/fs.h>
#include <unios/interrupt.h>
#include <unios/kstate.h>
#include <unios/schedule.h>
#include <arch/x86.h>
#include <sys/defs.h>
#include <config.h>
#include <assert.h>
#include <tar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <stddef.h>

static void initial_setup_fs() {
    hd_open(PRIMARY_MASTER);
    kdebug("init hd done");

    init_fs();
    kdebug("init fs done");
}

static void initial_setup_envs() {
    const char *initial_envs = "PWD=/orange\n"
                               "PATH=/orange\n"
                               "PATH_EXT=.bin\n";

    char path[PATH_MAX] = {};
    snprintf(path, sizeof(path), "/orange/%s", INSTALL_FILENAME);
    int resp = untar(path, "/orange");
    assert(resp != -1);

    const char *path_to_env0 = "/orange/env";

    int fd = open(path_to_env0, O_RDWR);
    if (fd == -1) {
        fd = open(path_to_env0, O_CREAT | O_RDWR);
        assert(fd != -1);
        int len      = strlen(initial_envs);
        int total_wr = write(fd, initial_envs, len);
        assert(total_wr == len);
        close(fd);
        fd = open(path_to_env0, O_RDWR);
    }
    assert(fd != -1);

    //! FIXME: bad impl, expect better fs & vfs support
    int len = p_proc_current->pcb.filp[fd]->fd_node.fd_inode->i_size;
    if (len == 0) {
        char *null_envp[1] = {};
        bool  ok           = putenv((void *)&null_envp);
        assert(ok);
    } else {
        char *buf = kmalloc(len);
        assert(buf != NULL);
        int total_rd = read(fd, buf, len);
        assert(total_rd == len);
        kinfo("load initial env from `%s`\n%s", path_to_env0, buf);
        int total_envs = 0;
        for (int i = 0; i < len; ++i) {
            if (buf[i] == '=') {
                ++total_envs;
            } else if (buf[i] == '\n') {
                buf[i] = '\0';
            }
        }
        char **envp = kmalloc(sizeof(void *) * (total_envs + 1));
        assert(envp != NULL);
        char *p      = buf;
        int   nr_env = 0;
        while (true) {
            envp[nr_env++] = p;
            if (nr_env >= total_envs) { break; }
            while (*++p != '\0') {}
            while (*p++ == '\0') {}
            --p;
        }
        envp[total_envs] = NULL;

        kdebug("expected envp [pid=%d] {", get_pid());
        for (int i = 0; i <= total_envs; ++i) {
            kdebug("  [%d] %s", i, envp[i]);
        }
        kdebug("}");

        bool ok = putenv(envp);
        assert(ok);

        char *const *new_envp = getenv();
        assert(new_envp != NULL);
        free((void *)new_envp);
        kdebug("current envp [pid=%d] {", get_pid());
        for (int i = 0; i <= total_envs; ++i) {
            kdebug("  [%d] %s", i, envp[i]);
        }
        kdebug("}");

        kfree(envp);
        kfree(buf);
    }
    close(fd);
}

static void initial_enable_preinited_procs() {
    for (int i = 0; i < NR_PCBS; ++i) {
        process_t *proc = proc_table[i];
        if (proc == NULL) { continue; }
        if (proc->pcb.stat != PREINITED) { continue; }
        proc->pcb.stat = READY;
    }
}

void initial() {
    initial_setup_fs();

    int fd = -1;
    fd     = open("/dev_tty0", O_RDWR);
    assert(fd == stdin);
    fd = open("/dev_tty0", O_RDWR);
    assert(fd == stdout);
    fd = open("/dev_tty0", O_RDWR);
    assert(fd == stderr);

    initial_setup_envs();

    close(stdin);
    close(stdout);
    close(stderr);

    initial_enable_preinited_procs();

    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        int err = exec("shell_0");
        unreachable();
    }

    while (true) { yield(); }
    unreachable();
}
