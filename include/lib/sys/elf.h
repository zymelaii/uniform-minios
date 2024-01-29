#pragma once

#include <stdint.h>

//! NOTE: references as below:
//! https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
//! http://www.skyfree.org/linux/references/ELF_Format.pdf

#define ELF_MAGIC 0x464C457F //<! { 0x7e, 'E', 'L', 'F' }

#define ELF_PT_NULL    0x00000000
#define ELF_PT_LOAD    0x00000001
#define ELF_PT_DYNAMIC 0x00000002
#define ELF_PT_INTERP  0x00000003
#define ELF_PT_NOTE    0x00000004
#define ELF_PT_SHLIB   0x00000005
#define ELF_PT_PHDR    0x00000006
#define ELF_PT_TLS     0x00000007
#define ELF_PT_LOOS    0x60000000
#define ELF_PT_HIO     0x6fffffff
#define ELF_PT_LOPROC  0x70000000
#define ELF_PT_HIPRO   0x7fffffff

#define ELF_PF_X 0x1
#define ELF_PF_W 0x2
#define ELF_PF_R 0x4

#define ELF_SHT_NULL          0x00000000
#define ELF_SHT_PROGBITS      0x00000001
#define ELF_SHT_SYMTAB        0x00000002
#define ELF_SHT_STRTAB        0x00000003
#define ELF_SHT_RELA          0x00000004
#define ELF_SHT_HASH          0x00000005
#define ELF_SHT_DYNAMIC       0x00000006
#define ELF_SHT_NOTE          0x00000007
#define ELF_SHT_NOBITS        0x00000008
#define ELF_SHT_REL           0x00000009
#define ELF_SHT_SHLIB         0x0000000a
#define ELF_SHT_DYNSYM        0x0000000b
#define ELF_SHT_INIT_ARRAY    0x0000000e
#define ELF_SHT_FINI_ARRAY    0x0000000f
#define ELF_SHT_PREINIT_ARRAY 0x00000010
#define ELF_SHT_GROUP         0x00000011
#define ELF_SHT_SYMTAB_SHNDX  0x00000012
#define ELF_SHT_NUM           0x00000013
#define ELF_SHT_LOOS          0x60000000

#define ELF_SHF_WRITE            0x00000001
#define ELF_SHF_ALLOC            0x00000002
#define ELF_SHF_EXECINSTR        0x00000004
#define ELF_SHF_MERGE            0x00000010
#define ELF_SHF_STRINGS          0x00000020
#define ELF_SHF_INFO_LINK        0x00000040
#define ELF_SHF_LINK_ORDER       0x00000080
#define ELF_SHF_OS_NONCONFORMING 0x00000100
#define ELF_SHF_GROUP            0x00000200
#define ELF_SHF_TLS              0x00000400
#define ELF_SHF_MASKOS           0x0ff00000
#define ELF_SHF_MASKPROC         0xf0000000
#define ELF_SHF_ORDERED          0x04000000
#define ELF_SHF_EXCLUDE          0x08000000

typedef struct elf_header_s {
    uint32_t magic;
    uint8_t  elf[12];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} elf_header_t;

typedef struct elf_proghdr_s {
    uint32_t type;
    uint32_t offset;
    uint32_t va;
    uint32_t pa;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
} elf_proghdr_t;

typedef struct elf_secthdr_s {
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
} elf_secthdr_t;

const char *elfhdr_lookup_ident_class(int i);
const char *elfhdr_lookup_ident_encoding(int i);
const char *elfhdr_lookup_type(int i);
const char *elfhdr_lookup_machine(int i);
const char *elfphdr_lookup_flags(int i);
const char *elfphdr_lookup_type(int i);
const char *elfshdr_lookup_type(int i);

int elf_dump_hdr(char *buf, int len, elf_header_t *hdr);
int elf_dump_proghdr(char *buf, int len, elf_proghdr_t *hdr);
int elf_dump_secthdr(char *buf, int len, elf_secthdr_t *hdr);
