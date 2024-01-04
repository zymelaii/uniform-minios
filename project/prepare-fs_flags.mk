# path to project mk files
PROJMK_PREFIX ?=

# collect fs_flags objects
SOURCE_DIR := fs_flags/
OUTPUT_DIR := $(OBJDIR)/$(SOURCE_DIR)
include $(PROJMK_PREFIX)collect-objects.mk

# fs flag files
FS_FLAG_SOURCES := $(SOURCE_FILES)
FS_FLAG_FILES   := $(patsubst %,$(OUTPUT_DIR)%.bin,$(basename $(notdir $(FS_FLAG_SOURCES))))

ASBIN_SOURCE_FILES += $(FS_FLAG_SOURCES)
ASBIN_FILES        += $(FS_FLAG_FILES)
