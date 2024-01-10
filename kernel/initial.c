#include <unios/config.h>
#include <unios/proc.h>
#include <assert.h>
#include <tar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <stddef.h>

static const char *initial_envs = "PWD=/orange\n"
                                  "PATH=/orange\n"
                                  "PATH_EXT=.bin\n";

void initial() {
    int fd = -1;
    fd     = open("/dev_tty0", O_RDWR);
    assert(fd == stdin);
    fd = open("/dev_tty0", O_RDWR);
    assert(fd == stdout);
    fd = open("/dev_tty0", O_RDWR);
    assert(fd == stderr);

    char path[PATH_MAX] = {};
    snprintf(path, sizeof(path), "/orange/%s", INSTALL_FILENAME);
    int resp = untar(path, "/orange");
    assert(resp != -1);

    const char *path_to_env0 = "/orange/env";

    fd = open(path_to_env0, O_RDWR);
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
        klog("load initial env from `%s`\n%s\n", path_to_env0, buf);
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

        klog("expected envp [pid=%d] {\n", get_pid());
        for (int i = 0; i <= total_envs; ++i) {
            klog("  [%d] %s\n", i, envp[i]);
        }
        klog("}\n");

        bool ok = putenv(envp);
        assert(ok);

        char *const *new_envp = getenv();
        assert(new_envp != NULL);
        free((void *)new_envp);
        klog("current envp [pid=%d] {\n", get_pid());
        for (int i = 0; i <= total_envs; ++i) {
            klog("  [%d] %s\n", i, envp[i]);
        }
        klog("}\n");

        kfree(envp);
        kfree(buf);
    }
    close(fd);

    close(stdin);
    close(stdout);
    close(stderr);

    int err = exec("shell_0");
    unreachable();
}
