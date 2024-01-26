# build tools
$(OBJDIR)/tools/%: tools/%.c
	@echo -ne "[PROC] build $(notdir $@)\r"
	@mkdir -p $(@D)
	@$(CC) -MD -O3 -w -o $@ $<
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m build $(notdir $@)"
