# build runtime library
$(LIBRT_FILE): $(filter $(OBJDIR)lib/%.obj,$(OBJECT_FILES))
	@$(call begin-job,ar,$(notdir $@))
	@mkdir -p $(@D)
	@$(AR) $(ARFLAGS) -o $@ $^ > /dev/null 2>&1
	@$(call end-job,done,ar,$(notdir $@))
