MOUNT_POINT := $(OBJDIR)/iso

RAW_HD_IMAGE := hd/test1.img

MBR_FILE    := $(OBJDIR)/boot/mbr.bin
BOOT_FILE   := $(OBJDIR)/boot/boot.bin
LOADER_FILE := $(OBJDIR)/boot/loader.bin

ORANGE_FS_FLAG_FILE := $(OBJDIR)/fs_flags/orange_flag.bin
FAT32_FS_FLAG_FILE  := $(OBJDIR)/fs_flags/fat32_flag.bin

$(IMAGE_FILE)p0: $(RAW_HD_IMAGE) $(MBR_FILE) $(BOOT_FILE)
	@echo -ne "[PROC] pre-build image\r"
	@mkdir -p $(@D)
	@cp -f $(RAW_HD_IMAGE) $@
	@dd if=$(MBR_FILE) of=$@ bs=1 count=446 conv=notrunc > /dev/null 2>&1
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m pre-build image -> $(notdir $@)"

	@\
	echo -ne "[PROC] make fat32 fs\r";									\
	loop_device=`losetup -f`;											\
	sudo losetup -P $${loop_device} $@;									\
	sudo mkfs.vfat -F 32 $${loop_device}p1 > /dev/null 2>&1;			\
	echo -e "\e[1K\r\e[32m[DONE]\e[0m make fat32 fs -> $(notdir $@)";	\
																		\
	echo -ne "[PROC] write boot program\r";								\
	dd if=$(BOOT_FILE) of=$@ bs=1 count=420 							\
		seek=$(OSBOOT_START_OFFSET) conv=notrunc > /dev/null 2>&1;		\
	sudo losetup -d $${loop_device};									\
	echo -e "\e[1K\r\e[32m[DONE]\e[0m write boot program -> $(notdir $@)"

$(IMAGE_FILE)p1: $(IMAGE_FILE)p0 $(LOADER_FILE) $(KERNEL_FILE)
	@echo -ne "[PROC] install bootloader & kernel\r"
	@mkdir -p $(@D)
	@cp -f $< $@
	@mkdir -p $(MOUNT_POINT)
	@\
	loop_device=`losetup -f`;						\
	sudo losetup -P $${loop_device} $@;				\
	sudo mount $${loop_device}p1 $(MOUNT_POINT);	\
	sudo cp $(LOADER_FILE) $(MOUNT_POINT)/;			\
	sudo cp $(KERNEL_FILE) $(MOUNT_POINT)/;			\
	sudo umount $(MOUNT_POINT);						\
	sudo losetup -d $${loop_device}
	@rm -rf $(MOUNT_POINT)
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m install bootloader & kernel -> $(notdir $@)"

$(IMAGE_FILE)p2: $(IMAGE_FILE)p1 $(USER_TAR_FILE)
	@echo -ne "[PROC] install user programs\r"
	@mkdir -p $(@D)
	@cp -f $< $@
	@dd if=$(USER_TAR_FILE) of=$@ bs=512 						\
		count=$(INSTALL_NR_SECTORS) seek=$(INSTALL_PHY_SECTOR) 	\
		conv=notrunc > /dev/null 2>&1
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m install user programs -> $(notdir $@)"

$(IMAGE_FILE)p3: $(IMAGE_FILE)p2 $(ORANGE_FS_FLAG_FILE) $(FAT32_FS_FLAG_FILE)
	@echo -ne "[PROC] write fs flags\r"
	@mkdir -p $(@D)
	@cp -f $< $@
	@dd if=$(ORANGE_FS_FLAG_FILE) of=$@ bs=1 count=1	\
		seek=$(ORANGE_FS_START_OFFSET) conv=notrunc > /dev/null 2>&1
	@dd if=$(FAT32_FS_FLAG_FILE) of=$@ bs=1 count=11 	\
		seek=$(FAT32_FS_START_OFFSET) conv=notrunc > /dev/null 2>&1
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m write fs flags -> $(notdir $@)"

# image sha1sum for outdated detection
$(IMAGE_FILE).sha1: $(IMAGE_FILE)p3 force
	@\
	if [ -e "$(IMAGE_FILE)" ]; then						\
		sha1=`sha1sum $(IMAGE_FILE) | cut -d ' ' -f 1`;	\
	else												\
		sha1=`cat $@ 2> /dev/null || echo -n ''`;		\
	fi;													\
	sha1org=`sha1sum $< | cut -d ' ' -f 1`;				\
	if [ "$${sha1}" != "$${sha1org}" ]; then			\
		echo -n "$${sha1org}" > $@;						\
	fi
.PHONY: force

# finalize uniform-os image
$(IMAGE_FILE): $(IMAGE_FILE)p3 $(IMAGE_FILE).sha1
	@echo -ne "[PROC] finalize image $(notdir $@)"
	@cp -f $< $@
	@echo -e "\e[1K\r\e[32m[DONE]\e[0m finalize image $(notdir $@)"
