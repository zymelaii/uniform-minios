#include <unios/config.h>
#include <unios/assert.h>
#include <tar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>

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

    fd = open("/orange/env", O_CREAT | O_RDWR);
    assert(fd != -1);
    const char *ENV0     = "PWD=/orange\n"
                           "PATH=/orange\n"
                           "PATH_EXT=.bin\n";
    size_t      len      = strlen(ENV0);
    int         total_wr = write(fd, ENV0, len);
    assert(total_wr == len);
    close(fd);

    close(stdin);
    close(stdout);
    close(stderr);

    int err = exec("shell_0");
    panic("unreachable");
}
