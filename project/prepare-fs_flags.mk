# path to project mk files
PROJMK_PREFIX ?=

# collect fs_flags objects
SOURCE_DIR := fs_flags/
OUTPUT_DIR := $(OBJDIR)$(SOURCE_DIR)
include $(PROJMK_PREFIX)collect-objects.mk

ORANGE_FS_FLAG_FILE := $(OBJDIR)fs_flags/orange_flag.bin
