# path to project mk files
PROJMK_PREFIX ?=

# collect user objects
SOURCE_DIR := user/
OUTPUT_DIR := $(OBJDIR)/$(SOURCE_DIR)
include $(PROJMK_PREFIX)collect-objects.mk

#! NOTE: non user prog sources, but is dependent on by the
#> user prog, only assembly sources currently
USER_DEPS_SOURCES := $(filter-out %.c,$(SOURCE_FILES))
USER_DEPS_OBJECTS := $(patsubst %,$(OBJDIR)/%.obj,$(USER_DEPS_SOURCES))

#! NOTE: user prog sources, each source corresponds to a user
#> program with the same name
USER_PROG_SOURCES := $(filter %.c,$(SOURCE_FILES))
USER_PROG_FILES   := $(patsubst %.c,$(OBJDIR)/%,$(USER_PROG_SOURCES))

# archive file of the user progs
USER_TAR_FILE := $(OUTPUT_DIR)$(USER_PROG_ARCHIVE)
