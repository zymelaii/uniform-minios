define add-tool-program-deps
$(OBJDIR)tools/dist/$(1): $(TOOL_$(1)_OBJECTS)
endef

# attach deps for each tool program
$(foreach program,$(TOOL_PROGRAMS),$(eval $(call add-tool-program-deps,$(program))))

# dispatch recipe to all user program rules
$(TOOL_PROGRAM_FILES):
	@$(call begin-job,ld,$(notdir $@))
	@mkdir -p $(@D)
	@$(CC) -o $@ $^
	@$(call end-job,done,ld,$(notdir $@))
