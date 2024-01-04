# build runtime library
$(LIBRT_FILE): $(filter $(OBJDIR)/lib/%.obj,$(OBJECT_FILES))
	@echo -ne "[PROC] ar $(notdir $@)\r"
	@mkdir -p $(@D)
	@$(AR) $(ARFLAGS) -o $@ $^ > /dev/null 2>&1
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m ar $(notdir $@)"
