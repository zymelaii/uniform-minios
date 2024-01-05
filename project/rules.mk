# path to project mk files
PROJMK_PREFIX ?=

# general rules
all: $(IMAGE_FILE) $(KERNEL_DEBUG_FILE)
.PHONY: all

clean:
	@rm -rf $(OBJDIR)
.PHONY: clean

# run & debug rules
run: $(IMAGE_FILE)
	@$(QEMU) $(QEMU_FLAGS) -drive file=$<,format=raw
.PHONY: run

debug: $(IMAGE_FILE)
	@$(QEMU) $(QEMU_FLAGS) -drive file=$<,format=raw -s -S
.PHONY: debug

gdb: $(KERNEL_DEBUG_FILE)
	@$(GDB) $(GDB_FLAGS) -ex 'file $<'
.PHONY: gdb

# unios rules
include $(PROJMK_PREFIX)rules-gen.mk
include $(PROJMK_PREFIX)rules-obj.mk
include $(PROJMK_PREFIX)rules-asbin.mk
include $(PROJMK_PREFIX)rules-lib.mk
include $(PROJMK_PREFIX)rules-user.mk
include $(PROJMK_PREFIX)rules-kernel.mk
include $(PROJMK_PREFIX)rules-image.mk

# install rules
install:
	@\
	files=\
	"		$(USER_PROG_FILES) 		\
			$(KERNEL_FILE) 			\
			$(KERNEL_DEBUG_FILE) 	\
			$(LIBRT_FILE)			\
	";								\
	for file in $${files}; do 		\
		target=`basename $${file}`;									\
		echo -ne "[PROC] install $${target}\r";						\
		if [ -e "$${file}" ]; then									\
			cp -t $(OBJDIR) $${file}; 								\
			echo -e "\e[1K\r\e[32m[DONE]\e[0m install $${target}";	\
		else														\
			echo -e "\e[1K\r[SKIP] install $${target}";				\
		fi;															\
	done
.PHONY: install

# extra configures
.DELETE_ON_ERROR:
.PRECIOUS: $(OBJECT_FILES) $(CACHED_FLAG_FILES)
