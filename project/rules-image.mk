MOUNT_POINT  := $(OBJDIR)iso
RAW_HD_IMAGE := hd/test1.img

$(IMAGE_FILE)p0: $(RAW_HD_IMAGE) $(MBR_FILE) $(BOOT_FILE)
	@$(call begin-job,pre-build image,)
	@mkdir -p $(@D)
	@cp -f $(RAW_HD_IMAGE) $@
	@dd if=$(MBR_FILE) of=$@ bs=1 count=446 conv=notrunc > /dev/null 2>&1
	@$(call end-job,done,pre-build image -> $(notdir $@),)

	@\
	$(call begin-job,make fat32 fs,);                                   \
	loop_device=`sudo losetup -f`;                                      \
	sudo losetup -P $${loop_device} $@;                                 \
	sudo mkfs.vfat -F 32 $${loop_device}p1 > /dev/null 2>&1;            \
	$(call end-job,done,make fat32 fs -> $(notdir $@),);                \
																		\
	$(call begin-job,write boot program,);                              \
	dd if=$(BOOT_FILE) of=$@ bs=1 count=420                             \
		seek=$(OSBOOT_START_OFFSET) conv=notrunc > /dev/null 2>&1;      \
	sudo losetup -d $${loop_device};                                    \
	$(call end-job,done,write boot program -> $(notdir $@),)

$(IMAGE_FILE)p1: $(IMAGE_FILE)p0 $(LOADER_FILE) $(KERNEL_FILE)
	@$(call begin-job,install bootloader & kernel,)
	@\
	loader_size=`stat -c "%s" $(LOADER_FILE)`;                  \
	if [ "$$loader_size" -gt "$(LOADER_SIZE_LIMIT)" ]; then     \
		echo -ne "\e[1K\r\e[31m[FAIL]\e[0m LOADER too large";   \
		echo -e " ($${loader_size}/$(LOADER_SIZE_LIMIT))"       \
		exit 1;                                                 \
	fi
	@mkdir -p $(@D)
	@cp -f $< $@
	@mkdir -p $(MOUNT_POINT)
	@\
	loop_device=`losetup -f`;                       \
	sudo losetup -P $${loop_device} $@;             \
	sudo mount $${loop_device}p1 $(MOUNT_POINT);    \
	sudo cp $(LOADER_FILE) $(MOUNT_POINT)/;         \
	sudo cp $(KERNEL_FILE) $(MOUNT_POINT)/;         \
	sudo umount $(MOUNT_POINT);                     \
	sudo losetup -d $${loop_device}
	@rm -rf $(MOUNT_POINT)
	@$(call end-job,done,install bootloader & kernel -> $(notdir $@),)

$(IMAGE_FILE)p2: $(IMAGE_FILE)p1 $(USER_TAR_FILE)
	@$(call begin-job,install user programs,)
	@mkdir -p $(@D)
	@cp -f $< $@
	@dd if=$(USER_TAR_FILE) of=$@ bs=512                        \
		count=$(INSTALL_NR_SECTORS) seek=$(INSTALL_PHY_SECTOR)  \
		conv=notrunc > /dev/null 2>&1
	@$(call end-job,done,install user programs -> $(notdir $@),)

$(IMAGE_FILE)p3: $(IMAGE_FILE)p2 $(ORANGE_FS_FLAG_FILE)
	@$(call begin-job,write fs flags,)
	@mkdir -p $(@D)
	@cp -f $< $@
	@dd if=$(ORANGE_FS_FLAG_FILE) of=$@ bs=1 count=1    \
		seek=$(ORANGE_FS_SB_OFFSET) conv=notrunc > /dev/null 2>&1
	@$(call end-job,done,write fs flags -> $(notdir $@),)

# image sha1sum for outdated detection
$(IMAGE_FILE).sha1: $(IMAGE_FILE)p3 force
	@\
	if [ -e "$(IMAGE_FILE)" ]; then                     \
		sha1=`sha1sum $(IMAGE_FILE) | cut -d ' ' -f 1`; \
	else                                                \
		sha1=`cat $@ 2> /dev/null || echo -n ''`;       \
	fi;                                                 \
	sha1org=`sha1sum $< | cut -d ' ' -f 1`;             \
	if [ "$${sha1}" != "$${sha1org}" ]; then            \
		echo -n "$${sha1org}" > $@;                     \
	fi
.PHONY: force

# finalize uniform-os image
$(IMAGE_FILE): $(IMAGE_FILE)p3 $(IMAGE_FILE).sha1
	@$(call begin-job,finalize image,$(notdir $@))
	@cp -f $< $@
	@$(call end-job,done,finalize image,$(notdir $@))
