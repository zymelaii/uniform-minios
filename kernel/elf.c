#include <unios/syscall.h>
#include <unios/elf.h>
#include <unios/fs.h>
#include <unios/assert.h>
#include <stdint.h>

void read_Ehdr(u32 fd, Elf32_Ehdr *File_Ehdr, u32 offset) {
    do_lseek(fd, offset, SEEK_SET);
    int resp = do_read(fd, (void *)File_Ehdr, sizeof(Elf32_Ehdr));
    assert(resp == sizeof(Elf32_Ehdr));
}

void read_Phdr(u32 fd, Elf32_Phdr *File_Phdr, u32 offset) {
    do_lseek(fd, offset, SEEK_SET);
    int resp = do_read(fd, (void *)File_Phdr, sizeof(Elf32_Phdr));
    assert(resp == sizeof(Elf32_Phdr));
}

void read_Shdr(u32 fd, Elf32_Shdr *File_Shdr, u32 offset) {
    do_lseek(fd, offset, SEEK_SET);
    int resp = do_read(fd, (void *)File_Shdr, sizeof(Elf32_Shdr));
    assert(resp == sizeof(Elf32_Shdr));
}
