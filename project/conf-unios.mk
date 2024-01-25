# shell used by unios mkfiles
SHELL := $(shell echo `which bash 2> /dev/null`)
ifeq ($(SHELL),)
    $(error unios requires `bash` to complete the build)
endif

# path to project mk files
PROJMK_PREFIX ?=

# path to generated headers
GENERATED_INCDIR := $(OBJDIR)/include

# uniform-os image
IMAGE_FILE := $(OBJDIR)/$(IMAGE_NAME).img

# uniform-os kernel
KERNEL_FILE       := $(OBJDIR)/kernel/$(KERNEL_NAME)
KERNEL_DEBUG_FILE := $(KERNEL_FILE)d

# start address of kernel .text
KERNEL_START_ADDR := 0xc0200000

# standard library for uniform-os
LIBRT_FILE := $(OBJDIR)/lib/lib$(LIBRT).a

# file name of the user prog archive
INSTALL_FILENAME := "$(USER_PROG_ARCHIVE)"

# hardcoded image assignment arguments
OSBOOT_START_OFFSET    := $(shell echo $$[0x0010005a])
ORANGE_FS_START_OFFSET := $(shell echo $$[0x00300200])
PART_START_SECTOR      := 6144
INSTALL_PHY_SECTOR     := 7095
INSTALL_NR_SECTORS     := 1000

INSTALL_START_SECTOR := $(shell echo $$[$(INSTALL_PHY_SECTOR) - $(PART_START_SECTOR)])

DEVICE_INFO_ADDR := 0x90000

KERNEL_NAME_IN_FAT := $(shell \
    basename=$(shell echo -n $(basename $(notdir $(KERNEL_NAME))) | cut -c -8 | tr 'a-z' 'A-Z');	\
	suffix=$(shell echo -n $(patsubst .%,%,$(suffix $(KERNEL_NAME))) | cut -c -3 | tr 'a-z' 'A-Z');	\
	printf "\"%-*s%-*s\"" "8" "$${basename}" "3" "$${suffix}"                                       \
)

# configure toolchain
DEFINES  ?=
INCDIRS  ?=
INCDIRS  += include
INCDIRS  += include/kernel
INCDIRS  += include/lib
INCDIRS  += include/deps
LINKDIRS ?=
LINKDIRS += $(OBJDIR)/lib
include $(PROJMK_PREFIX)conf-toolchain.mk

# configure qemu
include $(PROJMK_PREFIX)conf-qemu.mk

# configure gdb
GDB_SCRIPTS_HOME ?= misc/gdb/
include $(PROJMK_PREFIX)conf-gdb.mk

# libgcc, introduced mainly for some useful built-in functions
LIBGCC_FILE := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
ifeq ($(LIBGCC_FILE),)
    $(error unios requires `gcc-multilib` to complete the build)
endif
