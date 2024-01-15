ENVS         := $(OBJDIR)/.cache/.envs
CACHED_FILES += $(ENVS)

dup-envs:
	@$(foreach v, $(.VARIABLES), $(info $(v) = $($(v)))):
.PHONY: dup-envs

# make environs
$(ENVS): force
	@echo -ne "[PROC] write out make envs\r"
	@mkdir -p $(@D)
	@$(MAKE) -s dup-envs | grep -P '^\w+\s*=.*$$' > $@.swp
	@cmp -s $@ $@.swp || cp $@.swp $@
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m write out make envs";
.PHONY: force

# kernel entry address constant for loader
$(GENERATED_INCDIR)/kernel_entry.inc: $(KERNEL_DEBUG_FILE)
	@echo -ne "[PROC] generate $(notdir $@)\r"
	@mkdir -p $(@D)
	@echo -n "KernelEntryPointPhyAddr equ 0x`nm $< | grep -Po '(\w+)(?= T _start)'`" > $@
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m generate $(notdir $@)"

# FAT filename of kernel for loader
$(GENERATED_INCDIR)/kernel_file.inc: $(KERNEL_FILE)
	@echo -ne "[PROC] generate $(notdir $@)\r"
	@mkdir -p $(@D)
	@\
	basename=$(shell echo $(basename $(notdir $<)) | tr 'a-z' 'A-Z');		\
	suffix=$(shell echo $(patsubst .%,%,$(suffix $<)) | tr 'a-z' 'A-Z');	\
	n1=`echo -n $${basename} | wc -c`; 										\
	n2=`echo -n $${suffix} | wc -c`; 										\
	left=$$[11-$${n1}-$${n2}]; 												\
	filename=`printf "%s%*s%s" "$${basename}" "$${left}" ' ' "$${suffix}"`;	\
	echo "KernelFileName db \"$${filename}\",0" > $@
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m generate $(notdir $@)"

# config.h for kernel
# NOTE: $(ENVS) is very sensitive to the envs, be careful to update the config.h
$(GENERATED_INCDIR)/config.h: include/kernel/config.h.in $(ENVS)
	@echo -ne "[PROC] generate $(notdir $@)\r"
	@mkdir -p $(@D)
	@cp $< $@.swp
	@\
	vars=`cat $< | grep -Po '(?<=@)\w+(?=@)' | tr '\n' ' '`;	\
	cat $(ENVS) | while IFS= read -r row; do					\
		key=`echo $${row} | grep -Po '^(\w+)'`;					\
		value=`echo $${row} | grep -Po "(?<=$${key} = )(.*)"`;	\
		if [[ " $${vars} " =~ " $${key} " ]]; then				\
			sed -i "s/@$${key}@/$${value}/g" $@.swp;			\
		fi;														\
	done;
	@\
	if cmp -s $@ $@.swp; then 										\
		echo -e "\e[1K\r\e[32m[SKIP]\e[0m generate $(notdir $@)";	\
	else															\
		cp $@.swp $@;												\
		echo -e "\e[1K\r\e[32m[DONE]\e[0m generate $(notdir $@)";	\
	fi
