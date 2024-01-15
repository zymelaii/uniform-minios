$(OBJDIR)/%.bin: %.asm $(filter %.inc,$(GENERATED_FILES))
	@echo -ne "[PROC] as $(notdir $@)\r"
	@mkdir -p $(@D)
	@$(AS) $(ASFLAGS) -o $@ $<
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m as $(notdir $@)"
