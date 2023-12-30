#include <unios/config.h>
#include <unios/utils/tar.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

void initial() {
    int stdin  = open("/dev_tty0", O_RDWR);
    int stdout = open("/dev_tty0", O_RDWR);
    int stderr = open("/dev_tty0", O_RDWR);

    char path[MAX_PATH] = {};
    snprintf(path, sizeof(path), "/orange/%s", INSTALL_FILENAME);
    int resp = untar(path, "/orange");
    assert(resp != -1);

    close(stdin);
    close(stdout);
    close(stderr);

    int err = exec("/orange/shell_0.bin");
    panic("unreachable");
}
