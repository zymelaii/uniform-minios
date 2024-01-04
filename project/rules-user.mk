# build user programs
$(OBJDIR)/user/%: $(OBJDIR)/user/%.c.obj $(USER_DEPS_OBJECTS) $(LIBRT_FILE) $(LIBGCC_FILE)
	@echo -ne "[PROC] ld $(notdir $@)\r"
	@mkdir -p $(@D)
	@$(LD) $(LDFLAGS) -o $@ $^
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m ld $(notdir $@)"

# archive user programs
$(USER_TAR_FILE): $(USER_PROG_FILES)
	@echo -ne "[PROC] tar $(notdir $@)\r"
	@mkdir -p $(@D)
	@tar -vcf $@ -C $(OBJDIR)/user $(notdir $(USER_PROG_FILES)) > /dev/null 2>&1
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m tar $(notdir $@)"
