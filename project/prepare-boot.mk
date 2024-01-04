# path to project mk files
PROJMK_PREFIX ?=

# collect bootloader objects
SOURCE_DIR := boot/
OUTPUT_DIR := $(OBJDIR)/$(SOURCE_DIR)
include $(PROJMK_PREFIX)collect-objects.mk

# bootloader programs
BOOTLOADER_SOURCES := $(SOURCE_FILES)
BOOTLOADER_FILES   := $(patsubst %,$(OUTPUT_DIR)%.bin,$(basename $(notdir $(BOOTLOADER_SOURCES))))

ASBIN_SOURCE_FILES += $(BOOTLOADER_SOURCES)
ASBIN_FILES        += $(BOOTLOADER_FILES)

# needed by loader.bin, provide kernel info to loader
GENERATED_FILES += $(GENERATED_INCDIR)/kernel_entry.inc
GENERATED_FILES += $(GENERATED_INCDIR)/kernel_file.inc
