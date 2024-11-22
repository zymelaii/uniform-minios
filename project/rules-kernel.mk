# build kernel
$(KERNEL_FILE): $(KERNEL_OBJECTS) $(LIBRT_FILE) $(LIBGCC_FILE)
	@$(call begin-job,ld,$(notdir $@))
	@mkdir -p $(@D)
	@$(LD) $(LDFLAGS) -Ttext $(KERNEL_START_ADDR) -o $@ $^ -s
	@$(call end-job,done,ld,$(notdir $@))

# build kernel debug
$(KERNEL_DEBUG_FILE): $(KERNEL_OBJECTS) $(LIBRT_FILE) $(LIBGCC_FILE)
	@$(call begin-job,ld,$(notdir $@))
	@mkdir -p $(@D)
	@$(LD) $(LDFLAGS) -Ttext $(KERNEL_START_ADDR) -o $@ $^
	@$(call end-job,done,ld,$(notdir $@))
