# build user programs
$(OBJDIR)user/%: $(OBJDIR)user/%.c.obj $(USER_DEPS_OBJECTS) $(LIBRT_FILE) $(LIBGCC_FILE)
	@$(call begin-job,ld,$(notdir $@))
	@mkdir -p $(@D)
	@$(LD) $(LDFLAGS) -o $@ $^
	@$(call end-job,done,ld,$(notdir $@))

# archive user programs
$(USER_TAR_FILE): $(USER_PROG_FILES)
	@$(call begin-job,tar,$(notdir $@))
	@mkdir -p $(@D)
	@tar -vcf $@ -C $(OBJDIR)user $(notdir $(USER_PROG_FILES)) > /dev/null 2>&1
	@$(call end-job,done,tar,$(notdir $@))
