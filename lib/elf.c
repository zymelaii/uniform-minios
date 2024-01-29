#include <sys/elf.h>
#include <stdio.h>

const char *elfhdr_lookup_ident_class(int i) {
    return (const char *[]){
        "INVALID", "32-BIT", "64-BIT"}[i < 0 || i >= 3 ? 0 : i];
}

const char *elfhdr_lookup_ident_encoding(int i) {
    return (const char *[]){"INVALID", "LSB", "MSB"}[i < 0 || i >= 3 ? 0 : i];
}

const char *elfhdr_lookup_type(int i) {
    switch (i) {
        case 0x0000: {
            return "NONE";
        } break;
        case 0x0001: {
            return "REL";
        } break;
        case 0x0002: {
            return "EXEC";
        } break;
        case 0x0003: {
            return "DYN";
        } break;
        case 0x0004: {
            return "CORE";
        } break;
        case 0xfe00: {
            return "LOOS";
        } break;
        case 0xfeff: {
            return "HIOS";
        } break;
        case 0xff00: {
            return "LOPROC";
        } break;
        case 0xffff: {
            return "HIPROC";
        } break;
        default: {
            return "INVALID";
        } break;
    }
}

const char *elfhdr_lookup_machine(int i) {
    switch (i) {
        case 0x00: {
            return "None";
        } break;
        case 0x01: {
            return "AT&T WE 32100";
        } break;
        case 0x02: {
            return "SPARC";
        } break;
        case 0x03: {
            return "x86";
        } break;
        case 0x04: {
            return "Motorola 68000 (M68k)";
        } break;
        case 0x05: {
            return "Motorola 88000 (M88k)";
        } break;
        case 0x06: {
            return "Intel MCU";
        } break;
        case 0x07: {
            return "Intel 80860";
        } break;
        case 0x08: {
            return "MIPS";
        } break;
        case 0x09: {
            return "IBM System/370";
        } break;
        case 0x0a: {
            return "MIPS RS3000 Little-endian";
        } break;
        case 0x0f: {
            return "Hewlett-Packard PA-RISC";
        } break;
        case 0x13: {
            return "Intel 80960";
        } break;
        case 0x14: {
            return "PowerPC";
        } break;
        case 0x15: {
            return "PowerPC (64-bit)";
        } break;
        case 0x16: {
            return "S390, including S390x";
        } break;
        case 0x17: {
            return "IBM SPU/SPC";
        } break;
        case 0x24: {
            return "NEC V800";
        } break;
        case 0x25: {
            return "Fujitsu FR20";
        } break;
        case 0x26: {
            return "TRW RH-32";
        } break;
        case 0x27: {
            return "Motorola RCE";
        } break;
        case 0x28: {
            return "Arm (up to Armv7/AArch32)";
        } break;
        case 0x29: {
            return "Digital Alpha";
        } break;
        case 0x2a: {
            return "SuperH";
        } break;
        case 0x2b: {
            return "SPARC Version 9";
        } break;
        case 0x2c: {
            return "Siemens TriCore embedded processor";
        } break;
        case 0x2d: {
            return "Argonaut RISC Core";
        } break;
        case 0x2e: {
            return "Hitachi H8/300";
        } break;
        case 0x2f: {
            return "Hitachi H8/300H";
        } break;
        case 0x30: {
            return "Hitachi H8S";
        } break;
        case 0x31: {
            return "Hitachi H8/500";
        } break;
        case 0x32: {
            return "IA-64";
        } break;
        case 0x33: {
            return "Stanford MIPS-X";
        } break;
        case 0x34: {
            return "Motorola ColdFire";
        } break;
        case 0x35: {
            return "Motorola M68HC12";
        } break;
        case 0x36: {
            return "Fujitsu MMA Multimedia Accelerator";
        } break;
        case 0x37: {
            return "Siemens PCP";
        } break;
        case 0x38: {
            return "Sony nCPU embedded RISC processor";
        } break;
        case 0x39: {
            return "Denso NDR1 microprocessor";
        } break;
        case 0x3a: {
            return "Motorola Star*Core processor";
        } break;
        case 0x3b: {
            return "Toyota ME16 processor";
        } break;
        case 0x3c: {
            return "STMicroelectronics ST100 processor";
        } break;
        case 0x3d: {
            return "Advanced Logic Corp. TinyJ embedded processor family";
        } break;
        case 0x3e: {
            return "AMD x86-64";
        } break;
        case 0x3f: {
            return "Sony DSP Processor";
        } break;
        case 0x40: {
            return "Digital Equipment Corp. PDP-10";
        } break;
        case 0x41: {
            return "Digital Equipment Corp. PDP-11";
        } break;
        case 0x42: {
            return "Siemens FX66 microcontroller";
        } break;
        case 0x43: {
            return "STMicroelectronics ST9+ 8/16 bit microcontroller";
        } break;
        case 0x44: {
            return "STMicroelectronics ST7 8-bit microcontroller";
        } break;
        case 0x45: {
            return "Motorola MC68HC16 Microcontroller";
        } break;
        case 0x46: {
            return "Motorola MC68HC11 Microcontroller";
        } break;
        case 0x47: {
            return "Motorola MC68HC08 Microcontroller";
        } break;
        case 0x48: {
            return "Motorola MC68HC05 Microcontroller";
        } break;
        case 0x49: {
            return "Silicon Graphics SVx";
        } break;
        case 0x4a: {
            return "STMicroelectronics ST19 8-bit microcontroller";
        } break;
        case 0x4b: {
            return "Digital VAX";
        } break;
        case 0x4c: {
            return "Axis Communications 32-bit embedded processor";
        } break;
        case 0x4d: {
            return "Infineon Technologies 32-bit embedded processor";
        } break;
        case 0x4e: {
            return "Element 14 64-bit DSP Processor";
        } break;
        case 0x4f: {
            return "LSI Logic 16-bit DSP Processor";
        } break;
        case 0x8c: {
            return "TMS320C6000 Family";
        } break;
        case 0xaf: {
            return "MCST Elbrus e2k";
        } break;
        case 0xb7: {
            return "Arm 64-bits (Armv8/AArch64)";
        } break;
        case 0xdc: {
            return "Zilog Z80";
        } break;
        case 0xf3: {
            return "RISC-V";
        } break;
        case 0xf7: {
            return "Berkeley Packet Filter";
        } break;
        default: {
            return "Unknown";
        } break;
    }
}

const char *elfphdr_lookup_type(int i) {
    switch (i) {
        case 0x00000000: {
            return "PT_NULL";
        } break;
        case 0x00000001: {
            return "PT_LOAD";
        } break;
        case 0x00000002: {
            return "PT_DYNAMIC";
        } break;
        case 0x00000003: {
            return "PT_INTERP";
        } break;
        case 0x00000004: {
            return "PT_NOTE";
        } break;
        case 0x00000005: {
            return "PT_SHLIB";
        } break;
        case 0x00000006: {
            return "PT_PHDR";
        } break;
        case 0x00000007: {
            return "PT_TLS";
        } break;
        case 0x60000000: {
            return "PT_LOOS";
        } break;
        case 0x6fffffff: {
            return "PT_HIO";
        } break;
        case 0x70000000: {
            return "PT_LOPROC";
        } break;
        case 0x7fffffff: {
            return "PT_HIPRO";
        } break;
        default: {
            return "UNKNOWN";
        } break;
    }
}

const char *elfphdr_lookup_flags(int i) {
    switch (i) {
        case 0b001: {
            return "PF_X";
        } break;
        case 0b010: {
            return "PF_W";
        } break;
        case 0b100: {
            return "PF_R";
        } break;
        case 0b011: {
            return "PF_W | PF_X";
        } break;
        case 0b101: {
            return "PF_R | PF_X";
        } break;
        case 0b110: {
            return "PF_R | PF_W";
        } break;
        case 0b111: {
            return "PF_R | PF_W | PF_X";
        } break;
        default: {
            return "INVALID";
        } break;
    }
}

const char *elfshdr_lookup_type(int i) {
    switch (i) {
        case 0x00000000: {
            return "SHT_NULL";
        } break;
        case 0x00000001: {
            return "SHT_PROGBITS";
        } break;
        case 0x00000002: {
            return "SHT_SYMTAB";
        } break;
        case 0x00000003: {
            return "SHT_STRTAB";
        } break;
        case 0x00000004: {
            return "SHT_RELA";
        } break;
        case 0x00000005: {
            return "SHT_HASH";
        } break;
        case 0x00000006: {
            return "SHT_DYNAMIC";
        } break;
        case 0x00000007: {
            return "SHT_NOTE";
        } break;
        case 0x00000008: {
            return "SHT_NOBITS";
        } break;
        case 0x00000009: {
            return "SHT_REL";
        } break;
        case 0x0000000a: {
            return "SHT_SHLIB";
        } break;
        case 0x0000000b: {
            return "SHT_DYNSYM";
        } break;
        case 0x0000000e: {
            return "SHT_INIT_ARRAY";
        } break;
        case 0x0000000f: {
            return "SHT_FINI_ARRAY";
        } break;
        case 0x00000010: {
            return "SHT_PREINIT_ARRAY";
        } break;
        case 0x00000011: {
            return "SHT_GROUP";
        } break;
        case 0x00000012: {
            return "SHT_SYMTAB_SHNDX";
        } break;
        case 0x00000013: {
            return "SHT_NUM";
        } break;
        case 0x60000000: {
            return "SHT_LOOS";
        } break;
        default: {
            return "UNKNOWN";
        } break;
    }
}

int elf_dump_hdr(char *buf, int len, elf_header_t *hdr) {
    return snprintf(
        buf,
        len,
        "elf header {\n"
        "  ident {\n"
        "    magic: 0x%8x\n"
        "    class: %d \"%s\"\n"
        "    encoding: %d \"%s\"\n"
        "    version: %d\n"
        "  }\n"
        "  type: 0x%x \"%s\"\n"
        "  machine: 0x%x \"%s\"\n"
        "  version: %d\n"
        "  entry: %#p\n"
        "  program header offset: %d\n"
        "  section header offset: %d\n"
        "  flags: %d\n"
        "  elf header size: %d\n"
        "  program header size: %d\n"
        "  program header number: %d\n"
        "  section header size: %d\n"
        "  section header number: %d\n"
        "  section header index to names: %d\n"
        "}",
        hdr->magic,
        hdr->elf[0],
        elfhdr_lookup_ident_class(hdr->elf[0]),
        hdr->elf[1],
        elfhdr_lookup_ident_encoding(hdr->elf[1]),
        hdr->elf[2],
        hdr->type,
        elfhdr_lookup_type(hdr->type),
        hdr->machine,
        elfhdr_lookup_machine(hdr->machine),
        hdr->version,
        hdr->entry,
        hdr->phoff,
        hdr->shoff,
        hdr->flags,
        hdr->ehsize,
        hdr->phentsize,
        hdr->phnum,
        hdr->shentsize,
        hdr->shnum,
        hdr->shstrndx);
}

int elf_dump_proghdr(char *buf, int len, elf_proghdr_t *hdr) {
    return snprintf(
        buf,
        len,
        "elf program header {\n"
        "  type: 0x%x \"%s\"\n"
        "  offset: %#p\n"
        "  virtual address: %#p\n"
        "  physical address: %#p\n"
        "  file size: %d\n"
        "  memory size: %d\n"
        "  flags: 0x%x \"%s\"\n"
        "  align: %d\n"
        "}",
        hdr->type,
        elfphdr_lookup_type(hdr->type),
        hdr->offset,
        hdr->va,
        hdr->pa,
        hdr->filesz,
        hdr->memsz,
        hdr->flags,
        elfphdr_lookup_flags(hdr->flags),
        hdr->align);
}

int elf_dump_secthdr(char *buf, int len, elf_secthdr_t *hdr) {
    return snprintf(
        buf,
        len,
        "elf section header {\n"
        "  name offset in .shstrtab: %d\n"
        "  type: 0x%x \"%s\"\n"
        "  flags: 0x%08x\n"
        "  virtual address: %#p\n"
        "  offset: %d\n"
        "  file size: %d\n"
        "  associated section index: %d\n"
        "  extra info: 0x%04x\n"
        "  align: %d\n"
        "  entry size: %d\n"
        "}",
        hdr->name,
        hdr->type,
        elfshdr_lookup_type(hdr->type),
        hdr->flags,
        hdr->addr,
        hdr->offset,
        hdr->size,
        hdr->link,
        hdr->info,
        hdr->addralign,
        hdr->entsize);
}
