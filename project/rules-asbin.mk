$(OBJDIR)/%.bin: %.asm $(filter %.inc,$(GENERATED_FILES)) $(CACHED_FLAG_FILES)
	@echo -ne "[PROC] as $(notdir $@)\r"
	@mkdir -p $(@D)
	@$(AS) $(ASFLAGS) -MD $(patsubst %.bin, %.asm.d, $@) -o $@ $<
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m as $(notdir $@)"

$(LOADER_FILE): $(LOADER_OBJECTS) $(LOADER_LINKER) $(CACHED_FLAG_FILES)
	@echo -ne "[PROC] ld $(notdir $@)\r"
	@mkdir -p $(@D)
	@$(LD) $(LDFLAGS) -T $(LOADER_LINKER) -s --oformat binary -o $@ $(LOADER_OBJECTS)
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m ld $(notdir $@)"
