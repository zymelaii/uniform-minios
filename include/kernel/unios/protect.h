#pragma once

#include <stdint.h>

#define GDT_SIZE 128
#define IDT_SIZE 256

//! selector
//! ┏━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┳━━┓
//! ┃15┃14┃13┃12┃11┃10┃09┃08┃07┃06┃05┃04┃03┃02┃01┃00┃
//! ┣━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━┻━━╋━━╋━━┻━━┫
//! ┃ descriptor index                     ┃TI┃ RPL ┃
//! ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┻━━┻━━━━━┛
typedef struct selector_s {
    uint16_t rpl   : 2;
    uint16_t ti    : 1;
    uint16_t index : 13;
} __attribute__((packed)) selector_t;

//! descriptor of GDT & SDT
typedef struct descriptor_s {
    uint16_t limit0;
    uint16_t base0;
    uint8_t  base1;
    uint8_t  attr0;
    uint8_t  attr1_limit1;
    uint8_t  base2;
} __attribute__((packed)) descriptor_t;

//! gate descriptor
typedef struct gate_descriptor_s {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  reserved;

    union {
        uint8_t attr;

        struct {
            uint8_t attr_type    : 4;
            uint8_t attr_s       : 1;
            uint8_t attr_dpl     : 2;
            uint8_t attr_present : 1;
        };
    };

    uint16_t offset_high;
} __attribute__((packed)) gate_descriptor_t;

//! pointer to descriptor table
typedef struct descptr_s {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) descptr_t;

extern descptr_t         gdt_ptr;
extern descptr_t         idt_ptr;
extern descriptor_t      gdt[GDT_SIZE];
extern gate_descriptor_t idt[IDT_SIZE];

typedef struct tss_s {
    uint32_t backlink;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t flags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iobase;
} tss_t;

//! indicies to GDT descriptor, defined in loader
#define INDEX_DUMMY     0 // ┓
#define INDEX_FLAT_C    1 // ┣ LOADER 里面已经确定了的.
#define INDEX_FLAT_RW   2 // ┃
#define INDEX_VIDEO     3 // ┛
#define INDEX_TSS       4
#define INDEX_LDT_FIRST 5

//! selector, defined in loader
#define SELECTOR_DUMMY     0          // ┓
#define SELECTOR_FLAT_C    0x08       // ┣ LOADER 里面已经确定了的.
#define SELECTOR_FLAT_RW   0x10       // ┃
#define SELECTOR_VIDEO     (0x18 + 3) // ┛<-- RPL=3
#define SELECTOR_TSS       0x20 // TSS. 从外层跳到内存时 SS 和 ESP 的值从里面获得.
#define SELECTOR_LDT_FIRST 0x28

#define SELECTOR_KERNEL_CS SELECTOR_FLAT_C
#define SELECTOR_KERNEL_DS SELECTOR_FLAT_RW
#define SELECTOR_KERNEL_GS SELECTOR_VIDEO

//! total descriptors in LDT
//! NOTE: each task owns 1 LDT
#define LDT_SIZE 2

//! descriptor indices in LDT
#define INDEX_LDT_C  0
#define INDEX_LDT_RW 1

//! 描述符类型值说明
#define DA_32       0x4000 //<! 32 位段
#define DA_LIMIT_4K 0x8000 //<! 段界限粒度为 4K 字节
#define DA_DPL0     0x00   //<! DPL = 0
#define DA_DPL1     0x20   //<! DPL = 1
#define DA_DPL2     0x40   //<! DPL = 2
#define DA_DPL3     0x60   //<! DPL = 3

//! 存储段描述符类型值说明
#define DA_DR   0x90 //<! 存在的只读数据段类型值
#define DA_DRW  0x92 //<! 存在的可读写数据段属性值
#define DA_DRWA 0x93 //<! 存在的已访问可读写数据段类型值
#define DA_C    0x98 //<! 存在的只执行代码段属性值
#define DA_CR   0x9A //<! 存在的可执行可读代码段属性值
#define DA_CCO  0x9C //<! 存在的只执行一致代码段属性值
#define DA_CCOR 0x9E //<! 存在的可执行可读一致代码段属性值

//! 系统段描述符类型值说明
#define DA_LDT      0x82 //<! 局部描述符表段类型值
#define DA_TaskGate 0x85 //<! 任务门类型值
#define DA_386TSS   0x89 //<! 可用 386 任务状态段类型值
#define DA_386CGate 0x8C //<! 386 调用门类型值
#define DA_386IGate 0x8E //<! 386 中断门类型值
#define DA_386TGate 0x8F //<! 386 陷阱门类型值

//! selector attribute
#define SA_MASK_RPL 0xFFFC
#define SA_MASK_TI  0xFFFB
#define SA_RPL0     0
#define SA_RPL1     1
#define SA_RPL2     2
#define SA_RPL3     3
#define SA_TIG      0
#define SA_TIL      4

#define RPL_KERNEL SA_RPL0
#define RPL_TASK   SA_RPL1
#define RPL_USER   SA_RPL3

#define vir2phys(seg_base, vir) \
    (uint32_t)(((uint32_t)seg_base) + (uint32_t)(vir))

void init_protect_mode();
void init_descriptor(
    descriptor_t* desc, uint32_t base, uint32_t limit, uint16_t attr);
uint32_t seg2phys(uint16_t seg);
