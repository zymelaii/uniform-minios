# path to project mk files
PROJMK_PREFIX ?=

# collect tools objects
SOURCE_DIR   := tools/
OUTPUT_DIR   := $(OBJDIR)$(SOURCE_DIR)
SOURCE_FILES := $(shell find $(SOURCE_DIR) -type f)
OBJECT_FILES += $(patsubst $(SOURCE_DIR)%,$(OUTPUT_DIR)%.obj,$(SOURCE_FILES))

TOOL_PROGRAMS      := $(notdir $(wildcard $(SOURCE_DIR)*))
TOOL_PROGRAM_FILES := $(patsubst %,$(OUTPUT_DIR)dist/%,$(TOOL_PROGRAMS))

# collect objects for each tool into variable TOOL_$(program)_OBJECTS
$(foreach program,$(TOOL_PROGRAMS),$(eval TOOL_$(program)_OBJECTS := $(filter $(OUTPUT_DIR)$(program)/%.obj,$(OBJECT_FILES))))
