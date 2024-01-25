#pragma once

#include <stdint.h>

#define EI_NIDENT 16

/************************************
 *		elf 头
 *****************************************/
typedef struct {
    uint8_t e_ident[EI_NIDENT]; // ELF魔数，ELF字长，字节序，ELF文件版本等
    uint16_t e_type; // ELF文件类型，REL, 可执行文件，共享目标文件等
    uint16_t e_machine; // ELF的CPU平台属性
    uint32_t e_version; // ELF版本号
    uint32_t e_entry;   // ELF程序的入口虚拟地址
    uint32_t e_phoff;   // program header table(program头)在文件中的偏移
    uint32_t e_shoff;   // section header table(section头)在文件中的偏移
    uint32_t e_flags;   // 用于标识ELF文件平台相关的属性
    uint16_t e_ehsize;  // elf header（本文件头）的长度
    uint16_t e_phentsize; // program header table 中每一个条目的长度
    uint16_t e_phnum;     // program header table 中有多少个条目
    uint16_t e_shentsize; // section header table 中每一个条目的长度
    uint16_t e_shnum;     // section header table 中有多少个条目
    uint16_t e_shstrndx;  // section header table 中字符索引
} Elf32_Ehdr;

/*******************************************
 *		program头(程序头)
 **********************************************/
typedef struct {
    uint32_t p_type;   // 该program 	类型
    uint32_t p_offset; // 该program	在文件中的偏移量
    uint32_t p_vaddr;  // 该program	应该放在这个线性地址
    uint32_t p_paddr;  // 该program
                      // 应该放在这个物理地址（对只使用物理地址的系统有效）
    uint32_t p_filesz; // 该program	在文件中的长度
    uint32_t p_memsz; // 该program	在内存中的长度（不一定和filesz相等）
    uint32_t p_flags; // 该program	读写权限
    uint32_t p_align; // 该program	对齐方式
} Elf32_Phdr;

/*********************************************
 *		section头(段头)
 ************************************************/
typedef struct {
    uint32_t s_name; // 该section 段的名字
    uint32_t s_type; // 该section 的类型，代码段，数据段，符号表等
    uint32_t s_flags;     // 该section 在进程虚拟地址空间中的属性
    uint32_t s_addr;      // 该section 的虚拟地址
    uint32_t s_offset;    // 该section 在文件中的偏移
    uint32_t s_size;      // 该section 的长度
    uint32_t s_link;      // 该section	头部表符号链接
    uint32_t s_info;      // 该section	附加信息
    uint32_t s_addralign; // 该section 对齐方式
    uint32_t
        s_entsize; // 该section 若有固定项目，则给出固定项目的大小，如符号表
} Elf32_Shdr;

void read_Ehdr(u32 fd, Elf32_Ehdr *File_Ehdr, u32 offset);
void read_Phdr(u32 fd, Elf32_Phdr *File_Phdr, u32 offset);
void read_Shdr(u32 fd, Elf32_Shdr *File_Shdr, u32 offset);
