# path to project mk files
PROJMK_PREFIX ?=

# general rules
all: image kernel tools
.PHONY: all

include $(PROJMK_PREFIX)rules-gen.mk
include $(PROJMK_PREFIX)rules-obj.mk
include $(PROJMK_PREFIX)rules-asbin.mk
include $(PROJMK_PREFIX)rules-lib.mk
include $(PROJMK_PREFIX)rules-user.mk
include $(PROJMK_PREFIX)rules-kernel.mk
include $(PROJMK_PREFIX)rules-image.mk
include $(PROJMK_PREFIX)rules-tools.mk
include $(PROJMK_PREFIX)rules-deps.mk

$(OBJDIR)compile_commands.json: force
	@$(call begin-job,dump,$(notdir $@))
	@mkdir -p $(@D)
	@bear --output $@ -- $(MAKE) -B > /dev/null 2>&1
	@$(call end-job,done,dump,$(notdir $@))
.PHONY: force

help: #<! display this information
	@awk 'BEGIN {FS = ":.*?#<! "} /^[a-zA-Z_-]+:.*?#<! / {sub("\\\\n",sprintf("\n%22c"," "), $$2);printf " \033[36m%-12s\033[0m  %s\n", $$1, $$2}' $(MAKEFILE_LIST)
.PHONY: help

clean: #<! clean output files
	@\
	if [ ! -d $(OBJDIR) ]; then exit; fi;          \
	$(call begin-job,clean-up all stuffs,);        \
	if rm -rf $(OBJDIR) 2> /dev/null; then         \
		$(call end-job,done,clean-up all stuffs,); \
	else                                           \
		$(call end-job,fail,clean-up all stuffs,); \
	fi
.PHONY: clean

run: $(IMAGE_FILE) #<! run qemu with image
	@$(QEMU) $(QEMU_FLAGS) -drive file=$<,format=raw
.PHONY: run

debug: $(IMAGE_FILE) $(KERNEL_DEBUG_FILE) #<! run qemu with image in debug mode
	@$(QEMU) $(QEMU_FLAGS) -drive file=$<,format=raw -s -S
.PHONY: debug

monitor: $(KERNEL_DEBUG_FILE) #<! run monitor for qemu
	@\
	if [ ! -e "$<" ]; then                        \
		$(call end-job,fail,missing debug file,); \
		exit;                                     \
	fi;                                           \
	$(GDB) $(GDB_FLAGS)  \
		-ex 'layout-sac' \
		-ex "file $<"    \
		-ex 'b _start'   \
		-ex 'c'
.PHONY: monitor

monitor-real: $(KERNEL_DEBUG_FILE) $(GDB_REALMODE_XML) $(GDB_REALMODE_SCRIPT) #<! run monitor for qemu in real mode
	@\
	if [ ! -e "$<" ]; then                        \
		$(call end-job,fail,missing debug file,); \
		exit;                                     \
	fi;                                           \
	$(GDB) $(GDB_FLAGS)                           \
		-ex 'set tdesc filename $(word 2,$^)'     \
		-x '$(word 3,$^)'                         \
		-ex 'file $<'                             \
		-ex 'enter-real-mode'                     \
		-ex 'b *0x7c00'                           \
		-ex 'c'
.PHONY: monitor-real

format: #<! format *.c and *.h files using clang-format
	@\
	TARGET="clang-format";                                              \
	MINVER=12;                                                          \
	$(call begin-job,format sources);                                   \
	FORMATTER=`which $${TARGET} 2> /dev/null`;                          \
	if [ -z "$${FORMATTER}" ]; then                                     \
		$(call end-job,fail,$${TARGET} is not available,);              \
		exit;                                                           \
	fi;                                                                 \
	VERSION=`$${FORMATTER} --version | grep -Po '(\d+)(?=\.\d+\.\d+)'`; \
	if [ "$${VERSION}" -lt $${MINVER} ]; then                           \
		$(call end-job,fail,requires $${TARGET} >= $${MINVER}.0.0,);    \
		exit;                                                           \
	fi;                                                                 \
	$${FORMATTER} -i `git ls-files '*.c' '*.h'`;                        \
	$(call end-job,done,format sources,)
.PHONY: format

pre-doc: doc/requirements.txt
	@\
	if ! pip install -r $< 2> /dev/null; then               \
		msg="try to install deps via sys package manager";  \
		echo -e "\e[31m[FAIL]\e[0m $${msg}";                \
		msg="install sphinx and python-libs listed in $<";  \
		echo -e "\e[90m[HINT]\e[0m $${msg}";                \
	fi
.PHONY: pre-doc

doc: #<! deploy sphinx doc
	@sphinx-autobuild doc $(OBJDIR)doc
.PHONY: doc

install: #<! install all stuffs to root of build dir
	@\
	files="               \
	$(USER_PROG_FILES)    \
	$(KERNEL_FILE)        \
	$(KERNEL_DEBUG_FILE)  \
	$(LIBRT_FILE)         \
	$(TOOL_PROGRAM_FILES) \
	";                    \
	for file in $${files}; do                        \
		target=`basename $${file}`;                  \
		$(call begin-job,install,$${target});        \
		if [ -e "$${file}" ]; then                   \
			cp -t $(OBJDIR) $${file};                \
			$(call end-job,done,install,$${target}); \
		else                                         \
			$(call end-job,skip,install,$${target}); \
		fi;                                          \
	done
.PHONY: install

dup-cc-win: $(OBJDIR)compile_commands.json #<! dump clangd compile_commands.json for windows
	@sed -r -i 's/(\/[a-z]+)*\/?(gcc|g\+\+)/\2/g' $<
	@sed -r -i 's/\/mnt\/([a-z])\//\1:\//g' $<

dup-cc: $(OBJDIR)compile_commands.json #<! dump clangd compile_commands.json
config: dup-cc $(GENERATED_FILES) #<! configure project
config-win: dup-cc-win $(GENERATED_FILES) #<! configure project for windows
build: all #<! build all stuffs
lib: $(LIBRT_FILE) #<! build library for kernel
user: $(USER_TAR_FILE) #<! build user programs
kernel: $(KERNEL_FILE) $(KERNEL_DEBUG_FILE) #<! build kernel file
tools: $(TOOL_PROGRAM_FILES) #<! build tools
image: $(IMAGE_FILE) #<! build image file

conf: config #<! alias for `config`
conf-win: config-win #<! alias for `config-win`
b: build #<! alias for `build`
r: run #<! alias for `run`
d: debug #<! alias for `debug`
i: install #<! alias for `install`
mon: monitor #<! alias for `monitor`
mon-real: monitor-real #<! alias for `monitor-real`
krnl: kernel #<! alias for `kernel`
fmt: format #<! alias for `format`

.DELETE_ON_ERROR:
.PRECIOUS: $(OBJECT_FILES) $(CACHED_FILES)
