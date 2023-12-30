#include <unios/syscall.h>
#include <type.h>
#include <elf.h>
#include <fs.h>

void read_Ehdr(u32 fd, Elf32_Ehdr *File_Ehdr, u32 offset) {
    do_lseek(fd, offset, SEEK_SET);
    do_read(fd, (void *)File_Ehdr, sizeof(Elf32_Ehdr));
}

void read_Phdr(u32 fd, Elf32_Phdr *File_Phdr, u32 offset) {
    do_lseek(fd, offset, SEEK_SET);
    do_read(fd, (void *)File_Phdr, sizeof(Elf32_Phdr));
}

void read_Shdr(u32 fd, Elf32_Shdr *File_Shdr, u32 offset) {
    do_lseek(fd, offset, SEEK_SET);
    do_read(fd, (void *)File_Shdr, sizeof(Elf32_Shdr));
}
