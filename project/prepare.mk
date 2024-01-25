# path to project mk files
PROJMK_PREFIX ?=

# elf type .obj files
OBJECT_FILES :=

# generated files
GENERATED_FILES :=

# cached files
CACHED_FILES :=

include $(PROJMK_PREFIX)prepare-boot.mk
include $(PROJMK_PREFIX)prepare-fs_flags.mk
include $(PROJMK_PREFIX)prepare-lib.mk
include $(PROJMK_PREFIX)prepare-user.mk
include $(PROJMK_PREFIX)prepare-kernel.mk
