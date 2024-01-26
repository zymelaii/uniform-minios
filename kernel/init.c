#include <unios/tracing.h>
#include <unios/hd.h>
#include <unios/fs.h>
#include <unios/tty.h>
#include <config.h>
#include <assert.h>
#include <tar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

static void init_setup_fs() {
    hd_open(PRIMARY_MASTER);
    kinfo("init hd done");

    init_fs();
    kinfo("init fs done");
}

static void init_setup_envs() {
    const char *initial_envs = "PWD=/orange\n"
                               "PATH=/orange\n"
                               "PATH_EXT=.bin\n";

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

    kinfo("setup envs done");
}

static void init_enable_preinited_procs() {
    for (int i = 0; i < NR_PCBS; ++i) {
        process_t *proc = proc_table[i];
        if (proc == NULL) { continue; }
        if (proc->pcb.stat != PREINITED) { continue; }
        proc->pcb.stat = READY;
    }
}

static void init_untar_user_progs() {
    char path[PATH_MAX] = {};
    snprintf(path, sizeof(path), "/orange/%s", INSTALL_FILENAME);
    int resp = untar(path, "/orange");
    assert(resp != -1);
    kinfo("untar builtin user programs done");
}

void init_handle_new_tty() {
    //! NOTE: block init proc reenter the shell init routine
    static int lock = 0;
    if (!try_lock(&lock)) { return; }

    int nr_tty = tty_wait_for();
    if (nr_tty == -1) {
        release(&lock);
        return;
    }

    pid_t pid = fork();
    assert(pid >= 0);
    if (pid > 0) { return; }
    int       rfd[3]        = {-1, -1, -1};
    const int tfd[3]        = {stdin, stdout, stderr};
    char      buf[PATH_MAX] = {};
    snprintf(buf, sizeof(buf), "/dev_tty%d", nr_tty);
    for (int i = 0; i < 3; ++i) {
        rfd[i] = open(buf, O_RDWR);
        assert(rfd[i] == tfd[i]);
    }

    tty_notify_shell();
    release(&lock);

    printf("[TTY #%d]\n", nr_tty);
    exec("shell_0");
    unreachable();
}

void init() {
    init_setup_fs();
    init_setup_envs();
    init_untar_user_progs();
    init_enable_preinited_procs();
    while (true) {
        init_handle_new_tty();
        yield();
    }
    unreachable();
}
