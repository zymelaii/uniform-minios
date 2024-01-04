# kernel entry address constant for loader
$(GENERATED_INCDIR)/kernel_entry.inc: $(KERNEL_DEBUG_FILE)
	@echo -ne "[PROC] generate $(notdir $@)\r"
	@mkdir -p $(@D)
	@echo -n "KernelEntryPointPhyAddr equ 0x`nm $< | grep -Po '(\w+)(?= T _start)'`" > $@
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m generate $(notdir $@)"

# FAT filename of kernel for loader
$(GENERATED_INCDIR)/kernel_file.inc: $(KERNEL_FILE)
	@echo -ne "[PROC] generate $(notdir $@)\r"
	@mkdir -p $(@D)
	@\
	basename=$(shell echo $(basename $(notdir $<)) | tr 'a-z' 'A-Z');		\
	suffix=$(shell echo $(patsubst .%,%,$(suffix $<)) | tr 'a-z' 'A-Z');	\
	n1=`echo -n $${basename} | wc -c`; 										\
	n2=`echo -n $${suffix} | wc -c`; 										\
	left=$$[11-$${n1}-$${n2}]; 												\
	filename=`printf "%s%*s%s" "$${basename}" "$${left}" ' ' "$${suffix}"`;	\
	echo "KernelFileName db \"$${filename}\",0" > $@
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m generate $(notdir $@)"
