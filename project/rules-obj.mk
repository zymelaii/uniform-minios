# rules for dirty flags detection
CACHED_FLAG_LIST  :=
CACHED_FLAG_LIST  += CFLAGS
CACHED_FLAG_LIST  += LDFLAGS
CACHED_FLAG_LIST  += ASFLAGS
CACHED_FLAG_FILES := $(patsubst %,$(OBJDIR)/.cache/%,$(CACHED_FLAG_LIST))

$(OBJDIR)/.cache/%: force
	@mkdir -p $(@D)
	@echo "$($*)" | cmp -s $@ || echo "$($*)" > $@
.PHONY: force

# compile c objects
$(OBJDIR)/%.c.obj: %.c $(CACHED_FLAG_FILES)
	@echo -ne "[PROC] cc $<\r"
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m cc $<"

# [fallthrough] compile assembly objects
$(OBJDIR)/%.obj: % $(CACHED_FLAG_FILES)
	@echo -ne "[PROC] as $<\r"
	@mkdir -p $(@D)
	@$(AS) $(ASFLAGS) -f elf -o $@ $<
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m as $<"
