match_asbin_src = $(foreach file,$2,$(if $(filter $1,$(basename $(file))),$(file)))

$(OBJDIR)/%.bin: %.asm $(GENERATED_FILES)
	@echo -ne "[PROC] as $(notdir $@)\r"
	@mkdir -p $(@D)
	@$(AS) $(ASFLAGS) -o $@ $<
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m as $(notdir $@)"
