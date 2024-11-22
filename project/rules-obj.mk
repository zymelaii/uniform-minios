# rules for dirty flags detection
CACHED_FLAG_LIST  :=
CACHED_FLAG_LIST  += CFLAGS
CACHED_FLAG_LIST  += LDFLAGS
CACHED_FLAG_LIST  += ASFLAGS
CACHED_FLAG_FILES := $(patsubst %,$(OBJDIR).cache/%,$(CACHED_FLAG_LIST))
CACHED_FILES      += $(CACHED_FLAG_FILES)

$(OBJDIR).cache/%: force
	@mkdir -p $(@D)
	@$(ECHO) "$($*)" | cmp -s $@ || echo "$($*)" > $@
.PHONY: force

# compile c objects
$(OBJDIR)tools/%.c.obj: tools/%.c
	@$(call begin-job,cc,$<)
	@mkdir -p $(@D)
	@$(CC) -MD -O3 -w -c -o $@ $<
	@$(call end-job,done,cc,$<)

$(OBJDIR)%.c.obj: %.c $(filter %.h,$(GENERATED_FILES)) $(CACHED_FLAG_FILES)
	@$(call begin-job,cc,$<)
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $<
	@$(call end-job,done,cc,$<)

# compile assembly objects
$(OBJDIR)%.asm.obj: %.asm $(filter %.inc,$(GENERATED_FILES)) $(CACHED_FLAG_FILES)
	@$(call begin-job,as,$<)
	@mkdir -p $(@D)
	@$(AS) $(ASFLAGS) -MD $(patsubst %.obj,%.d,$@) -f elf -o $@ $<
	@$(call end-job,done,as,$<)
