# shell used by unios mkfiles
SHELL := $(shell echo `which bash 2> /dev/null`)
ifeq ($(SHELL),)
    $(error unios requires `bash` to complete the build)
endif

# avoid using builtin `echo` command
ECHO := $(shell which echo)

# pretty print method for jobs
define begin-job
	printf "\e[?25l"; \
	printf "%s.%03d [PROC] $(1) $(2)" "$$(date '+%Y-%m-%d %H:%M:%S')" "$$(echo "$$(date +%N) / 1000000" | bc)"
endef

define end-job-as-done
	$(ECHO) -e "\e[32m[DONE]\e[0m $(1) $(2)"
endef

define end-job-as-skip
	$(ECHO) -e "\e[33m[SKIP]\e[0m $(1) $(2)"
endef

define end-job-as-fail
	$(ECHO) -e "\e[31m[FAIL]\e[0m $(1) $(2)"
endef

define end-job
	printf "\r%s.%03d " "$$(date '+%Y-%m-%d %H:%M:%S')" "$$(echo "$$(date +%N) / 1000000" | bc)"; \
	$(call end-job-as-$(1),$(2),$(3)); \
	printf "\e[?25h"
endef

# path to project mk files
PROJMK_PREFIX ?=

# path to generated headers
GENERATED_INCDIR := $(OBJDIR)include

# uniform-os image
IMAGE_FILE := $(OBJDIR)$(IMAGE_NAME).img

# uniform-os kernel
KERNEL_FILE       := $(OBJDIR)kernel/$(KERNEL_NAME)
KERNEL_DEBUG_FILE := $(KERNEL_FILE)d

# start address of kernel .text
KERNEL_START_ADDR := 0xc0200000

# standard library for uniform-os
LIBRT_FILE := $(OBJDIR)lib/lib$(LIBRT).a

# file name of the user prog archive
INSTALL_FILENAME := "$(USER_PROG_ARCHIVE)"

# hardcoded image assignment arguments
OSBOOT_START_OFFSET    := $(shell echo $$[ 0x0010005a ]) #<! [FIXED] fat32 boot sector boot code offset
ORANGE_FS_PART_START   := 6144                           #<! [FIXED] orangefs start sector
ORANGE_FS_SB_OFFSET    := $(shell echo $$[ 6145 * 512 ]) #<! [FIXED] orangefs superblock offset
ORANGE_FS_PART_SECTORS := 43857                          #<! [FIXED] orangefs total sectors
ORANGE_FS_IMAP_SECTORS := 1                              #<! [FIXED] orangefs inode bitmap sectors
INSTALL_PHY_SECTOR     := 7095                           #<! where to start installing user app tar
INSTALL_NR_SECTORS     := 4000                           #<! total sectors reserved for user app tar

# start sector of user programs relative to orangefs partition
INSTALL_START_SECTOR := $(shell echo $$[$(INSTALL_PHY_SECTOR) - $(ORANGE_FS_PART_START)])

DEVICE_INFO_ADDR := 0x90000

KERNEL_NAME_IN_FAT := $(shell \
    basename=$(shell echo -n $(basename $(notdir $(KERNEL_NAME))) | cut -c -8 | tr 'a-z' 'A-Z');    \
	suffix=$(shell echo -n $(patsubst .%,%,$(suffix $(KERNEL_NAME))) | cut -c -3 | tr 'a-z' 'A-Z'); \
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
LINKDIRS += $(OBJDIR)lib
include $(PROJMK_PREFIX)conf-toolchain.mk

# configure qemu
QEMU_ARCH   := i386
QEMU_MEMORY := 256
include $(PROJMK_PREFIX)conf-qemu.mk

# configure gdb
GDB_SCRIPTS_HOME ?= share/gdb/
include $(PROJMK_PREFIX)conf-gdb.mk

# libgcc, introduced mainly for some useful built-in functions
LIBGCC_FILE := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
ifeq ($(LIBGCC_FILE),)
    $(error unios requires `gcc-multilib` to complete the build)
endif
ifeq ($(shell nm $(LIBGCC_FILE) 2> /dev/null | grep 'T __divdi3'),)
    LIBGCC_FILE := libgcc.x86_64-32-13.2.1.a
endif
