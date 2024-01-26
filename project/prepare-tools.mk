# path to project mk files
PROJMK_PREFIX ?=

# collect tools objects
SOURCE_DIR := tools/
OUTPUT_DIR := $(OBJDIR)/$(SOURCE_DIR)
include $(PROJMK_PREFIX)collect-objects.mk

TOOLS_SOURCE_FILES := $(filter %.c,$(SOURCE_FILES))
TOOLS_EXECUTABLE   := $(patsubst $(SOURCE_DIR)%.c,$(OUTPUT_DIR)%,$(TOOLS_SOURCE_FILES))
