OBJDIR := obj
INCDIR := include

PREFIX ?=

CC	    := $(PREFIX)gcc
AS	    := nasm
AR	    := $(PREFIX)ar
LD	    := $(PREFIX)ld
NM	    := $(PREFIX)nm
OBJCOPY	:= $(PREFIX)objcopy
OBJDUMP	:= $(PREFIX)objdump

DEFS ?=

CFLAGS ?=
CFLAGS += $(DEFS)
CFLAGS += -I$(INCDIR) -MD
CFLAGS += -fno-stack-protector
CFLAGS += -O0
CFLAGS += -std=gnu99
CFLAGS += -fno-builtin -nostdinc -nostdlib
CFLAGS += -m32
CFLAGS += -static -fno-pie
CFLAGS += -g
CFLAGS += -Werror -Wno-unused-variable
# default qemu cpu core may not support sse
CFLAGS += -mno-sse -mno-sse2

ASFLAGS ?=

LDFLAGS	?=
LDFLAGS += -m elf_i386
LDFLAGS += -nostdlib

GCC_LIB	:= $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

OBJDIRS	:=

QEMU ?= qemu-system-i386

QEMU_FLAGS := -boot order=a
QEMU_FLAGS += -serial file:stdout.log
QEMU_FLAGS += -m 128m
QEMU_FLAGS += -display curses

GDB ?= gdb

GDB_FLAGS := -q -nx
GDB_FLAGS += -x  'misc/gdb/connect-qemu.gdb'
GDB_FLAGS += -x  'misc/gdb/instr-level.gdb'
GDB_FLAGS += -ex 'set pagination off'
GDB_FLAGS += -ex 'set confirm off'
GDB_FLAGS += -ex 'set disassembly-flavor att'
GDB_FLAGS += -ex 'layout asm'
GDB_FLAGS += -ex 'tui reg general'
GDB_FLAGS += -ex 'connect-qemu'

include boot/Makefrag
include lib/Makefrag
include kernel/Makefrag
include user/Makefrag
include fs_flags/Makefrag

IMAGE = $(OBJDIR)/a.img
BUILD_IMAGE_SCRIPT = build_img.sh

# added by mingxuan 2020-9-12
# Offset of os_boot in hd
# 活动分区所在的扇区号
# OSBOOT_SEC = 4096
# 活动分区所在的扇区号对应的字节数
# OSBOOT_OFFSET = $(OSBOOT_SEC) * 512
OSBOOT_OFFSET = 1048576
# FAT32 规范规定 os_boot 的前 89 个字节是 FAT32 的配置信息
# OSBOOT_START_OFFSET = OSBOOT_OFFSET + 90
OSBOOT_START_OFFSET = 1048666 # for test12.img

# added by mingxuan 2020-10-29
# Offset of fs in hd
# 文件系统标志所在扇区号 = 文件系统所在分区的第 1 个扇区 + 1
# ORANGE_FS_SEC = 6144 + 1 = 6145
# 文件系统标志所在扇区 = $(ORANGE_FS_SEC) * 512
ORANGE_FS_START_OFFSET = 3146240
# FAT32_FS_SEC = 53248 + 1 = 53249
# 文件系统标志所在扇区 = $(ORANGE_FS_SEC) * 512
FAT32_FS_START_OFFSET = 27263488

# oranges 文件系统在硬盘上的起始扇区
# PART_START_SECTOR = 92049
PART_START_SECTOR = 6144	# modified by mingxuan 2020-10-12

# 写入硬盘的起始位置
# INSTALL_PHY_SECTOR = PART_START_SECTOR + 951 # Why is 951 ?
INSTALL_PHY_SECTOR = 7095	# modified by mingxuan 2020-10-12

# 写入硬盘的文件大小
INSTALL_NR_SECTORS = 1000

INSTALL_START_SECTOR = $(shell echo $$(($(INSTALL_PHY_SECTOR)-$(PART_START_SECTOR))))
SUPER_BLOCK_ADDR = $(shell echo $$((($(PART_START_SECTOR)+1)*512)))

INSTALL_TYPE     = INSTALL_TAR
INSTALL_FILENAME = app.tar

all: $(IMAGE)
.PHONY: all

clean:
	@rm -rf $(OBJDIR)
.PHONY: clean

$(IMAGE): \
		$(OBJDIR)/boot/mbr.bin		\
		$(OBJDIR)/boot/boot.bin		\
		$(OBJDIR)/boot/loader.bin	\
		$(OBJDIR)/kernel/kernel.bin	\
		$(BUILD_IMAGE_SCRIPT)		\
		$(OBJDIR)/user/$(USER_TAR)	\
		$(FS_FLAG_OBJFILES)
	@./build_img.sh $@ $(OBJDIR) $(OSBOOT_START_OFFSET)
	@dd if=$(OBJDIR)/user/$(USER_TAR) of=$@ bs=512 count=$(INSTALL_NR_SECTORS) seek=$(INSTALL_PHY_SECTOR) conv=notrunc
	@dd if=$(OBJDIR)/fs_flags/orange_flag.bin of=$@ bs=1 count=1 seek=$(ORANGE_FS_START_OFFSET) conv=notrunc
	@dd if=$(OBJDIR)/fs_flags/fat32_flag.bin of=$@ bs=1 count=11 seek=$(FAT32_FS_START_OFFSET) conv=notrunc

run: $(IMAGE)
	@$(QEMU) $(QEMU_FLAGS) -drive file=$(IMAGE),format=raw
.PHONY: run

run-debug: $(IMAGE)
	@$(QEMU) $(QEMU_FLAGS) -drive file=$(IMAGE),format=raw -s -S
.PHONY: run-debug

KERN_DBG := $(OBJDIR)/kernel/kernel.dbg

gdb: $(KERN_DBG)
	@$(GDB) $(GDB_FLAGS) -ex 'file $<'
.PHONY: gdb

disassemble: $(KERN_DBG)
	@$(OBJDUMP) -S $< | less -S
.PHONY: disassemble

.PRECIOUS: 	$(OBJDIR)/.vars.%						  \
		$(OBJDIR)/kernel/%.o $(OBJDIR)/kernel/%.d	  \
		$(OBJDIR)/user/%.o $(OBJDIR)/user/%.d	  	  \
		$(OBJDIR)/lib/%.o $(OBJDIR)/lib/%.d

$(OBJDIR)/.vars.%: FORCE
	@echo "$($*)" | cmp -s $@ || echo "$($*)" > $@
.PHONY: FORCE

.DELETE_ON_ERROR:

$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@perl mergedep.pl $@ $^

-include $(OBJDIR)/.deps
