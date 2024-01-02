#include <unios/config.h>
#include <unios/utils/tar.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initial() {
    int stdin  = open("/dev_tty0", O_RDWR);
    int stdout = open("/dev_tty0", O_RDWR);
    int stderr = open("/dev_tty0", O_RDWR);

    char path[MAX_PATH] = {};
    snprintf(path, sizeof(path), "/orange/%s", INSTALL_FILENAME);
    int resp = untar(path, "/orange");
    assert(resp != -1);
    int fd = open("/orange/env", O_CREAT | O_RDWR);
    assert(fd != -1);
    const char *ENV0 = "PWD=/orange\n"
                       "PATH=/orange\n"
                       "PATH_EXT=.bin\n";
    int         n    = write(fd, ENV0, strlen(ENV0));
    assert(n == strlen(ENV0));
    close(stdin);
    close(stdout);
    close(stderr);

    int err = exec("/orange/shell_0.bin");
    panic("unreachable");
}
