#include <tar.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <limits.h>

int untar(const char *tar_path, const char *extract_dir) {
    //! NOTE: untar may only support 1-depth extraction

    char buf[512 * 16]     = {};
    char pathbuf[PATH_MAX] = {};

    int fd = open(tar_path, O_RDWR);
    assert(fd != -1 && "tar file not exists");

    int  nr_file = 0;
    bool err     = false;
    while (true) {
        read(fd, buf, 512);

        //! FIXME: fs io maybe out of range?
        if (buf[0] == 0) { break; }

        //! compute file size from octal
        tar_header_t *phdr   = (tar_header_t *)buf;
        int           szfile = 0;
        for (char *p = phdr->size; *p; ++p) { szfile = szfile * 8 + *p - '0'; }
        int left = szfile;

        snprintf(pathbuf, sizeof(pathbuf), "%s/%s", extract_dir, phdr->name);
        int fdout = open(pathbuf, O_CREAT | O_RDWR);
        //! TODO: enable custom handler for existed file
        bool skip = fdout == -1;

        while (left > 0) {
            int iobytes = MIN(sizeof(buf), left);
            read(fd, buf, ((iobytes - 1) / 512 + 1) * 512);
            if (!skip) { write(fdout, buf, iobytes); }
            left -= iobytes;
        }

        if (!skip) { close(fdout); }
        ++nr_file;
    }

    close(fd);

    return err ? -1 : nr_file;
}
