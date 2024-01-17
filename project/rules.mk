# path to project mk files
PROJMK_PREFIX ?=

# general rules
all: $(IMAGE_FILE) $(KERNEL_DEBUG_FILE)
.PHONY: all

clean:
	@\
	if [ ! -d $(OBJDIR) ]; then exit; fi;		\
	echo -ne "[PROC] clean-up all stuffs\r";	\
	if rm -rf $(OBJDIR) 2> /dev/null; then						\
		echo -e "\e[1K\r\e[32m[DONE]\e[0m clean-up all stuffs";	\
	else														\
		echo -e "\e[1K\r\e[31m[FAIL]\e[0m clean-up all stuffs";	\
	fi
.PHONY: clean

# run & debug rules
run: $(IMAGE_FILE)
	@$(QEMU) $(QEMU_FLAGS) -drive file=$<,format=raw
.PHONY: run

debug: $(IMAGE_FILE)
	@$(QEMU) $(QEMU_FLAGS) -drive file=$<,format=raw -s -S
.PHONY: debug

monitor: $(KERNEL_DEBUG_FILE)
	@$(GDB) $(GDB_FLAGS) -ex 'file $<'
.PHONY: monitor

# unios rules
include $(PROJMK_PREFIX)rules-gen.mk
include $(PROJMK_PREFIX)rules-obj.mk
include $(PROJMK_PREFIX)rules-asbin.mk
include $(PROJMK_PREFIX)rules-lib.mk
include $(PROJMK_PREFIX)rules-user.mk
include $(PROJMK_PREFIX)rules-kernel.mk
include $(PROJMK_PREFIX)rules-image.mk
include $(PROJMK_PREFIX)rules-deps.mk

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
			echo -e "\e[1K\r\e[33m[SKIP]\e[0m install $${target}";	\
		fi;															\
	done
.PHONY: install

# compile_commands.json rules
$(OBJDIR)/compile_commands.json: force
	@echo -ne "[PROC] dump compile_commands.json\r"
	@mkdir -p $(@D)
	@bear --output $(OBJDIR)/compile_commands.json -- $(MAKE) -B > /dev/null 2>&1
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m dump compile_commands.json"
.PHONY: force

dup-cc: $(OBJDIR)/compile_commands.json

# extra configures
.DELETE_ON_ERROR:
.PRECIOUS: $(OBJECT_FILES) $(CACHED_FILES)

# target aliases
config: dup-cc
conf: config
build: all
b: build
r: run
d: debug
i: install
gdb: monitor
mon: monitor
lib: $(LIBRT_FILE)
user: $(USER_TAR_FILE)
kernel: $(KERNEL_FILE) $(KERNEL_DEBUG_FILE)
krnl: kernel
image: $(IMAGE_FILE)
