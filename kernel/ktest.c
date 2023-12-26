#include <unios/vfs.h>
#include <unios/config.h>
#include <unios/utils/tar.h>
#include <assert.h>
#include <stdio.h>
#include <proto.h>

void initial() {
    int stdin  = do_vopen("/dev/tty0", O_RDWR);
    int stdout = do_vopen("/dev/tty0", O_RDWR);
    int stderr = do_vopen("/dev/tty0", O_RDWR);

    char path[MAX_PATH] = {};
    snprintf(path, sizeof(path), "/orange/%s", INSTALL_FILENAME);
    int resp = untar(path, "/orange");
    assert(resp != -1);

    do_vclose(stdin);
    do_vclose(stdout);
    do_vclose(stderr);

    int err = exec("/orange/shell_0.bin");
    panic("unreachable");
}
