#include <unios/utils/tar.h>
#include <unios/vfs.h>
#include <stdio.h>
#include <assert.h>
#include <type.h>

int untar(const char *tar_path, const char *extract_dir) {
    //! NOTE: untar may only support 1-depth extraction

    char buf[512 * 16]     = {};
    char pathbuf[MAX_PATH] = {};

    int fd = do_vopen(tar_path, O_RDWR);
    assert(fd != -1 && "tar file not exists");

    int  nr_file = 0;
    bool err     = false;
    while (true) {
        do_vread(fd, buf, 512);

        //! FIXME: fs io maybe out of range?
        if (buf[0] == 0) { break; }

        //! compute file size from octal
        tar_header_t *phdr   = (tar_header_t *)buf;
        int           szfile = 0;
        for (char *p = phdr->size; *p; ++p) { szfile = szfile * 8 + *p - '0'; }
        int left = szfile;

        snprintf(pathbuf, sizeof(pathbuf), "%s/%s", extract_dir, phdr->name);
        int fdout = do_vopen(pathbuf, O_CREAT | O_RDWR);
        //! FIXME: handle already existed file
        assert(fdout != -1 && "failed to create target file");

        while (left > 0) {
            int iobytes = min(sizeof(buf), left);
            do_vread(fd, buf, ((iobytes - 1) / 512 + 1) * 512);
            do_vwrite(fdout, buf, iobytes);
            left -= iobytes;
        }

        do_vclose(fdout);
        ++nr_file;
    }

    do_vclose(fd);

    return err ? -1 : nr_file;
}
