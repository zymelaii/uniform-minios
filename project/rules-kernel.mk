# build kernel
$(KERNEL_FILE): $(KERNEL_OBJECTS) $(LIBRT_FILE) $(LIBGCC_FILE)
	@echo -ne "[PROC] ld $(notdir $@)\r"
	@mkdir -p $(@D)
	@$(LD) $(LDFLAGS) -Ttext $(KERNEL_START_ADDR) -o $@ $^ -s
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m ld $(notdir $@)"

# build kernel debug
$(KERNEL_DEBUG_FILE): $(KERNEL_OBJECTS) $(LIBRT_FILE) $(LIBGCC_FILE)
	@echo -ne "[PROC] ld $(notdir $@)\r"
	@mkdir -p $(@D)
	@$(LD) $(LDFLAGS) -Ttext $(KERNEL_START_ADDR) -o $@ $^
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m ld $(notdir $@)"
