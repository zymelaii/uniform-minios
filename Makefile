# path to project mk files
PROJMK_PREFIX := project/

include $(PROJMK_PREFIX)configure.mk
include $(PROJMK_PREFIX)prepare.mk
include $(PROJMK_PREFIX)rules.mk
