# path to project mk files
PROJMK_PREFIX ?=

# collect lib objects
SOURCE_DIR := lib/
OUTPUT_DIR := $(OBJDIR)$(SOURCE_DIR)
include $(PROJMK_PREFIX)collect-objects.mk
