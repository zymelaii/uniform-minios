#include <stdlib.h>
#include <stddef.h>

int exec(const char *path) {
    return execve(path, NULL, NULL);
}
