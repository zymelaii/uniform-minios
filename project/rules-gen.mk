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

# config.h for kernel
# NOTE: $(ENVS) is very sensitive to the envs, be careful to update the config.h
$(GENERATED_INCDIR)/config.h: include/kernel/config.h.in $(ENVS)
	@echo -ne "[PROC] generate $(notdir $@)\r"
	@mkdir -p $(@D)
	@cp $< $@.swp
	@\
	vars=`cat $< | grep -Po '(?<=@)\w+(?=@)' | tr '\n' ' '`;	\
	cat $(ENVS) | while IFS= read -r row; do					\
		read -r key value <<< 									\
			`echo "$${row}" | awk -F' = ' '{print $$1, $$2}'`;	\
		if [[ " $${vars} " =~ " $${key} " ]]; then				\
			sed -i "s/@$${key}@/$${value}/g" $@.swp;			\
		fi;														\
	done;
	@\
	if cmp -s $@ $@.swp; then 										\
		echo -e "\e[1K\r\e[33m[SKIP]\e[0m generate $(notdir $@)";	\
	else															\
		cp $@.swp $@;												\
		echo -e "\e[1K\r\e[32m[DONE]\e[0m generate $(notdir $@)";	\
	fi
