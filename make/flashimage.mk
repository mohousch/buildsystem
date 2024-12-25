#
# nor-image
#
nor-image-$(BOXTYPE):
	mkdir -p $(IMAGE_DIR)
	cd $(SCRIPTS_DIR)/nor_flash && $(SUDOCMD) ./make_flash.sh $(MAINTAINER) $(BOXTYPE)
	
#
# atevio-image
#
atevio-image-$(BOXTYPE):
	mkdir -p $(IMAGE_DIR)
	cd $(SCRIPTS_DIR)/atevio7500 && $(SUDOCMD) ./atevio7500.sh $(MAINTAINER)
	
#
# spark-image
#
spark-image-$(BOXTYPE):
	mkdir -p $(IMAGE_DIR)
	cd $(SCRIPTS_DIR)/spark && $(SUDOCMD) ./spark.sh $(MAINTAINER) $(BOXTYPE)

#
# ufs912-image
#	
ufs912-image-$(BOXTYPE):
	mkdir -p $(IMAGE_DIR)
	cd $(SCRIPTS_DIR)/ufs912 && $(SUDOCMD) ./ufs912.sh $(MAINTAINER)
	
#
# usb-image
#
usb-image-$(BOXTYPE):
	mkdir -p $(IMAGE_DIR)
	cd $(RELEASE_DIR) && \
	tar cvJf $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.tar.xz --exclude=vmlinux.gz* . > /dev/null 2>&1

#
# ubi-image
#
ubi-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	mkdir -p $(IMAGE_DIR)
	# splash
	cp $(SKEL_ROOT)/boot/splash.bin $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(BOOTLOGO_FILENAME)
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), gbultraue))
	cp $(SKEL_ROOT)/boot/warning.bin $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	cp $(SKEL_ROOT)/boot/lcdsplash.bin $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	cp $(SKEL_ROOT)/boot/lcdwarning220.bin $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/lcdwarning.bin
	cp $(SKEL_ROOT)/boot/lcdwaitkey220.bin $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/lcdwaitkey.bin
endif
	echo $(BOOT_UPDATE_TEXT) > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(BOOT_UPDATE_FILE);
	# kernel
	cp $(TARGET_DIR)/boot/$(KERNEL_FILE) $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), vuduo2))
	cp $(TARGET_DIR)/boot/$(INITRD_NAME) $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(INITRD_FILE)
endif
	# rootfs
	mkfs.ubifs -r $(RELEASE_DIR) -o $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(IMAGE_NAME).$(IMAGE_FSTYPES) $(MKUBIFS_ARGS)
	echo [ubifs] > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/ubinize.cfg
	echo mode=ubi >> $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/ubinize.cfg
	echo image=$(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(IMAGE_NAME).$(IMAGE_FSTYPES) >> $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/ubinize.cfg
	echo vol_id=0 >> $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/ubinize.cfg
	echo vol_type=dynamic >> $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/ubinize.cfg
	echo vol_name=$(UBI_VOLNAME) >> $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/ubinize.cfg
	echo vol_size=$(FLASHSIZE)MiB >> $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/ubinize.cfg
	echo vol_flags=autoresize >> $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/ubinize.cfg
	ubinize -o $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(ROOTFS_FILE) $(UBINIZE_ARGS) $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/ubinize.cfg
	rm -f $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(IMAGE_NAME).$(IMAGE_FSTYPES)
	rm -f $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/ubinize.cfg
	#
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.zip $(FLASHIMAGE_PREFIX)*
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)
	
#
# dm-nfi-image
#
dm-nfi-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(BOXTYPE)
	mkdir -p $(IMAGE_DIR)
	#
	cp -f $(ARCHIVE)/$(2ND_FILE) $(IMAGE_BUILD_DIR)/$(BOXTYPE)/
	#
	rm -f $(RELEASE_DIR)/boot/*
	cp $(TARGET_DIR)/boot/$(KERNEL_FILE) $(RELEASE_DIR)/boot/
	ln -sf $(KERNEL_FILE) $(RELEASE_DIR)/boot/vmlinux
	echo "/boot/bootlogo-$(BOXTYPE).elf.gz filename=/boot/bootlogo-$(BOXTYPE).jpg" > $(RELEASE_DIR)/boot/autoexec.bat
	echo "/boot/$(KERNEL_FILE) ubi.mtd=root root=ubi0:rootfs rootfstype=ubifs rw console=ttyS0,115200n8" >> $(RELEASE_DIR)/boot/autoexec.bat
	cp $(RELEASE_DIR)/boot/autoexec.bat $(RELEASE_DIR)/boot/autoexec_$(BOXTYPE).bat
	cp $(SKEL_ROOT)/boot/bootlogo-$(BOXTYPE).elf.gz $(RELEASE_DIR)/boot/
	cp $(SKEL_ROOT)/boot/bootlogo.jpg $(RELEASE_DIR)/boot/bootlogo-$(BOXTYPE).jpg
	#
	mkfs.jffs2 --root=$(RELEASE_DIR)/boot/ --disable-compressor=lzo --compression-mode=size --eraseblock=$(ERASE_BLOCK_SIZE) --output=$(IMAGE_BUILD_DIR)/$(BOXTYPE)/boot.jffs2
	mkfs.ubifs -r $(RELEASE_DIR) -o $(IMAGE_BUILD_DIR)/$(BOXTYPE)/$(IMAGE_NAME).$(IMAGE_FSTYPES) $(MKUBIFS_ARGS)
	echo [ubifs] > $(IMAGE_BUILD_DIR)/$(BOXTYPE)/ubinize.cfg
	echo mode=ubi >> $(IMAGE_BUILD_DIR)/$(BOXTYPE)/ubinize.cfg
	echo image=$(IMAGE_BUILD_DIR)/$(BOXTYPE)/$(IMAGE_NAME).$(IMAGE_FSTYPES) >> $(IMAGE_BUILD_DIR)/$(BOXTYPE)/ubinize.cfg
	echo vol_id=0 >> $(IMAGE_BUILD_DIR)/$(BOXTYPE)/ubinize.cfg
	echo vol_type=dynamic >> $(IMAGE_BUILD_DIR)/$(BOXTYPE)/ubinize.cfg
	echo vol_name=$(UBI_VOLNAME) >> $(IMAGE_BUILD_DIR)/$(BOXTYPE)/ubinize.cfg
#	echo vol_size=$(FLASHSIZE)MiB >> $(IMAGE_BUILD_DIR)/$(BOXTYPE)/ubinize.cfg
	echo vol_flags=autoresize >> $(IMAGE_BUILD_DIR)/$(BOXTYPE)/ubinize.cfg
	ubinize -o $(IMAGE_BUILD_DIR)/$(BOXTYPE)/$(ROOTFS_FILE) $(UBINIZE_ARGS) $(IMAGE_BUILD_DIR)/$(BOXTYPE)/ubinize.cfg
	rm -f $(IMAGE_BUILD_DIR)/$(BOXTYPE)/$(IMAGE_NAME).$(IMAGE_FSTYPES)
	rm -f $(IMAGE_BUILD_DIR)/$(BOXTYPE)/ubinize.cfg
	#
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(BOXTYPE)/imageversion
	#
	cd $(IMAGE_BUILD_DIR)/$(BOXTYPE) && \
	buildimage -a $(BOXTYPE) $(BUILDIMAGE_EXTRA) -e $(ERASE_BLOCK_SIZE) -f $(FLASH_SIZE) -s $(SECTOR_SIZE) -b $(LOADER_SIZE):$(2ND_FILE) -d $(BOOT_SIZE):boot.jffs2 -d $(ROOT_SIZE):$(ROOTFS_FILE) > $(BOXTYPE).nfi
	echo $(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(BOXTYPE)/$(BOXTYPE).nfo
	#
	cd $(IMAGE_BUILD_DIR)/$(BOXTYPE) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.zip $(BOXTYPE).{nfi,nfo} imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)

#
# dm-rootfs-image
#
dm-rootfs-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(BOXTYPE)
	mkdir -p $(IMAGE_DIR)
	# kernel
	cp $(TARGET_DIR)/boot/zImage $(IMAGE_BUILD_DIR)/$(BOXTYPE)/$(KERNEL_FILE)
	# rootfs
	cd $(RELEASE_DIR); \
	tar -cvf $(IMAGE_BUILD_DIR)/$(BOXTYPE)/rootfs.tar . > /dev/null 2>&1; \
	bzip2 $(IMAGE_BUILD_DIR)/$(BOXTYPE)/rootfs.tar
	#
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(BOXTYPE)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.zip $(BOXTYPE)/rootfs.tar.bz2 $(BOXTYPE)/$(KERNEL_FILE) $(BOXTYPE)/imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)

#
# vuplus-rootfs image
#
vuplus-rootfs-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	mkdir -p $(IMAGE_DIR)
	# kernel
	cp $(TARGET_DIR)/boot/$(INITRD_NAME) $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(INITRD_FILE)
	cp $(TARGET_DIR)/boot/zImage $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(KERNEL_FILE)
	# rootfs
	cd $(RELEASE_DIR); \
	tar -cvf $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar --exclude=zImage* --exclude=vmlinuz-initrd* . > /dev/null 2>&1; \
	bzip2 $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar
	#
	echo $(BOOT_UPDATE_TEXT) > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(BOOT_UPDATE_FILE)
	echo $(PART_TEXT) > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(PART_FILE)
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.zip $(FLASHIMAGE_PREFIX)/rootfs.tar.bz2 $(FLASHIMAGE_PREFIX)/$(INITRD_FILE) $(FLASHIMAGE_PREFIX)/$(KERNEL_FILE) $(FLASHIMAGE_PREFIX)/$(BOOT_UPDATE_FILE) $(FLASHIMAGE_PREFIX)/$(PART_FILE) $(FLASHIMAGE_PREFIX)/imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)

#
# vuplus-multi-rootfs image
#
vuplus-multi-rootfs-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	mkdir -p $(IMAGE_DIR)
	# kernel
	cp $(TARGET_DIR)/boot/$(INITRD_NAME) $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(INITRD_FILE)
	cp $(TARGET_DIR)/boot/zImage $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(KERNEL1_FILE)
	cp $(TARGET_DIR)/boot/zImage $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(KERNEL2_FILE)
	cp $(TARGET_DIR)/boot/zImage $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(KERNEL3_FILE)
	cp $(TARGET_DIR)/boot/zImage $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(KERNEL4_FILE)
	# rootfs
	cd $(RELEASE_DIR); \
	tar -cvf $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar --exclude=zImage* --exclude=vmlinuz-initrd* . > /dev/null 2>&1; \
	bzip2 $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar
	mv $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar.bz2 $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs1.tar.bz2
	cp $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs1.tar.bz2 $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs2.tar.bz2
	cp $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs1.tar.bz2 $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs3.tar.bz2
	cp $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs1.tar.bz2 $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs4.tar.bz2
	#
	echo $(BOOT_UPDATE_TEXT) > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(BOOT_UPDATE_FILE)
	echo $(PART_TEXT) > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(PART_FILE)
	echo Dummy for update. > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(KERNEL_FILE)
	echo Dummy for update. > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar.bz2
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_multi_usb.zip $(FLASHIMAGE_PREFIX)/rootfs*.tar.bz2 $(FLASHIMAGE_PREFIX)/$(INITRD_FILE) $(FLASHIMAGE_PREFIX)/$(KERNEL_FILE) $(FLASHIMAGE_PREFIX)/$(KERNEL1_FILE) $(FLASHIMAGE_PREFIX)/$(KERNEL2_FILE) $(FLASHIMAGE_PREFIX)/$(KERNEL3_FILE) $(FLASHIMAGE_PREFIX)/$(KERNEL4_FILE) $(FLASHIMAGE_PREFIX)/$(BOOT_UPDATE_FILE) $(FLASHIMAGE_PREFIX)/$(PART_FILE) $(FLASHIMAGE_PREFIX)/imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)
	
#
# vuplus-rootfs-image
#
vuplus-rootfs-image-$(BOXTYPE):
	# Create final USB-image
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	mkdir -p $(IMAGE_DIR)
	# kernel
	cp $(TARGET_DIR)/boot/$(INITRD_NAME) $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(INITRD_FILE)
	cp $(TARGET_DIR)/boot/zImage $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(KERNEL_FILE)
	# rootfs
	cd $(RELEASE_DIR); \
	tar -cvf $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar --exclude=zImage* --exclude=vmlinuz-initrd* . > /dev/null 2>&1; \
	bzip2 $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar
	#
	echo This file forces a reboot after the update. > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/reboot.update
	echo This file forces creating partitions. > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/mkpart.update
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.zip $(FLASHIMAGE_PREFIX)/rootfs.tar.bz2 $(FLASHIMAGE_PREFIX)/initrd_auto.bin $(FLASHIMAGE_PREFIX)/kernel_auto.bin $(FLASHIMAGE_PREFIX)/*.update $(FLASHIMAGE_PREFIX)/imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)
	
#
# octagon-disk-image
#
octagon-disk-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(BOXTYPE)
	mkdir -p $(IMAGE_DIR)
	# kernel
	cp $(TARGET_DIR)/boot/uImage $(IMAGE_BUILD_DIR)/$(BOXTYPE)/
	#
	unzip -o $(ARCHIVE)/$(FLASH_PARTITONS_SRC) -d $(IMAGE_BUILD_DIR)
	install -m 0755 $(IMAGE_BUILD_DIR)/patitions/apploader.bin $(RELEASE_DIR)/usr/share/apploader.bin
	install -m 0755 $(IMAGE_BUILD_DIR)/patitions/bootargs.bin $(RELEASE_DIR)/usr/share/bootargs.bin
	install -m 0755 $(IMAGE_BUILD_DIR)/patitions/fastboot.bin $(RELEASE_DIR)/usr/share/fastboot.bin
	install -m 0755 $(IMAGE_BUILD_DIR)/patitions/apploader.bin $(IMAGE_BUILD_DIR)/apploader.bin
	install -m 0755 $(IMAGE_BUILD_DIR)/patitions/bootargs.bin $(IMAGE_BUILD_DIR)/bootargs.bin
	install -m 0755 $(IMAGE_BUILD_DIR)/patitions/fastboot.bin $(IMAGE_BUILD_DIR)/fastboot.bin
	install -d $(IMAGE_BUILD_DIR)/userdata
	install -d $(IMAGE_BUILD_DIR)/userdata/linuxrootfs1
	install -d $(IMAGE_BUILD_DIR)/userdata/linuxrootfs2
	install -d $(IMAGE_BUILD_DIR)/userdata/linuxrootfs3
	install -d $(IMAGE_BUILD_DIR)/userdata/linuxrootfs4
	cp -a $(RELEASE_DIR) $(IMAGE_BUILD_DIR)/userdata
	dd if=/dev/zero of=$(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_NAME).rootfs.ext4 seek=$(ROOTFS_SIZE) count=0 bs=1024
	mkfs.ext4 -F -i 4096 $(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_NAME).rootfs.ext4 -d $(IMAGE_BUILD_DIR)/userdata
	fsck.ext4 -pvfD $(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_NAME).rootfs.ext4 || [ $? -le 3 ]
	cp $(IMAGE_BUILD_DIR)/$(BOXTYPE)/uImage $(IMAGE_BUILD_DIR)/patitions/kernel.bin
	cp $(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_NAME).rootfs.ext4 $(IMAGE_BUILD_DIR)/patitions/rootfs.ext4
	mkupdate -s 00000003-00000001-01010101 -f $(IMAGE_BUILD_DIR)/patitions/emmc_partitions.xml -d $(IMAGE_BUILD_DIR)/usb_update.bin
	#
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_recovery_emmc.zip apploader.bin bootargs.bin fastboot.bin usb_update.bin imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)

#
# octagon-rootfs-image
#	
octagon-rootfs-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(BOXTYPE)
	mkdir -p $(IMAGE_DIR)
	# kernel
	cp $(TARGET_DIR)/boot/uImage $(IMAGE_BUILD_DIR)/$(BOXTYPE)/
	# rootfs
	cd $(RELEASE_DIR); \
	tar -cvf $(IMAGE_BUILD_DIR)/$(BOXTYPE)/rootfs.tar --exclude=uImage* . > /dev/null 2>&1; \
	bzip2 $(IMAGE_BUILD_DIR)/$(BOXTYPE)/rootfs.tar
	#
	echo "$(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d.%m.%Y-%H.%M')" > $(IMAGE_BUILD_DIR)/$(BOXTYPE)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.zip $(BOXTYPE)/rootfs.tar.bz2 $(BOXTYPE)/uImage $(BOXTYPE)/imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)

#
# edision-disk-image
#
edision-disk-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(BOXTYPE)
	mkdir -p $(IMAGE_DIR)
	# Create a sparse image block
	dd if=/dev/zero of=$(IMAGE_BUILD_DIR)/$(IMAGE_LINK) seek=$(shell expr $(EMMC_IMAGE_SIZE) \* 1024) count=0 bs=1
	$(HOST_DIR)/bin/mkfs.ext4 -F -m0 $(IMAGE_BUILD_DIR)/$(IMAGE_LINK) -d $(RELEASE_DIR)
	# Error codes 0-3 indicate successfull operation of fsck (no errors or errors corrected)
	$(HOST_DIR)/bin/fsck.ext4 -pfD $(IMAGE_BUILD_DIR)/$(IMAGE_LINK) || [ $? -le 3 ]
	dd if=/dev/zero of=$(EMMC_IMAGE) bs=1 count=0 seek=$(shell expr $(EMMC_IMAGE_SIZE) \* 1024)
	parted -s $(EMMC_IMAGE) mklabel gpt
	parted -s $(EMMC_IMAGE) unit KiB mkpart boot fat16 $(IMAGE_ROOTFS_ALIGNMENT) $(shell expr $(IMAGE_ROOTFS_ALIGNMENT) + $(BOOT_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) set 1 boot on
	parted -s $(EMMC_IMAGE) unit KiB mkpart kernel1 $(KERNEL1_PARTITION_OFFSET) $(shell expr $(KERNEL1_PARTITION_OFFSET) + $(KERNEL_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart rootfs1 ext4 $(ROOTFS1_PARTITION_OFFSET) $(shell expr $(ROOTFS1_PARTITION_OFFSET) + $(ROOTFS_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart kernel2 $(KERNEL2_PARTITION_OFFSET) $(shell expr $(KERNEL2_PARTITION_OFFSET) + $(KERNEL_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart rootfs2 ext4 $(ROOTFS2_PARTITION_OFFSET) $(shell expr $(ROOTFS2_PARTITION_OFFSET) + $(ROOTFS_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart kernel3 $(KERNEL3_PARTITION_OFFSET) $(shell expr $(KERNEL3_PARTITION_OFFSET) + $(KERNEL_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart rootfs3 ext4 $(ROOTFS3_PARTITION_OFFSET) $(shell expr $(ROOTFS3_PARTITION_OFFSET) + $(ROOTFS_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart kernel4 $(KERNEL4_PARTITION_OFFSET) $(shell expr $(KERNEL4_PARTITION_OFFSET) + $(KERNEL_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart rootfs4 ext4 $(ROOTFS4_PARTITION_OFFSET) $(shell expr $(ROOTFS4_PARTITION_OFFSET) + $(ROOTFS_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart swap linux-swap $(SWAP_PARTITION_OFFSET) 100%
	dd if=/dev/zero of=$(IMAGE_BUILD_DIR)/boot.img bs=1024 count=$(BOOT_PARTITION_SIZE)
	mkfs.msdos -n boot -S 512 $(IMAGE_BUILD_DIR)/boot.img
	echo "setenv STARTUP \"boot emmcflash0.kernel1 'root=/dev/mmcblk1p3 rootfstype=ext4 rw rootwait'\"" > $(IMAGE_BUILD_DIR)/STARTUP
	echo "setenv STARTUP \"boot emmcflash0.kernel1 'root=/dev/mmcblk1p3 rootfstype=ext4 rw rootwait'\"" > $(IMAGE_BUILD_DIR)/STARTUP_1
	echo "setenv STARTUP \"boot emmcflash0.kernel2 'root=/dev/mmcblk1p5 rootfstype=ext4 rw rootwait'\"" > $(IMAGE_BUILD_DIR)/STARTUP_2
	echo "setenv STARTUP \"boot emmcflash0.kernel3 'root=/dev/mmcblk1p7 rootfstype=ext4 rw rootwait'\"" > $(IMAGE_BUILD_DIR)/STARTUP_3
	echo "setenv STARTUP \"boot emmcflash0.kernel4 'root=/dev/mmcblk1p9 rootfstype=ext4 rw rootwait'\"" > $(IMAGE_BUILD_DIR)/STARTUP_4
	mcopy -i $(IMAGE_BUILD_DIR)/boot.img -v $(IMAGE_BUILD_DIR)/STARTUP ::
	mcopy -i $(IMAGE_BUILD_DIR)/boot.img -v $(IMAGE_BUILD_DIR)/STARTUP_1 ::
	mcopy -i $(IMAGE_BUILD_DIR)/boot.img -v $(IMAGE_BUILD_DIR)/STARTUP_2 ::
	mcopy -i $(IMAGE_BUILD_DIR)/boot.img -v $(IMAGE_BUILD_DIR)/STARTUP_3 ::
	mcopy -i $(IMAGE_BUILD_DIR)/boot.img -v $(IMAGE_BUILD_DIR)/STARTUP_4 ::
	parted -s $(EMMC_IMAGE) unit KiB print
	dd conv=notrunc if=$(IMAGE_BUILD_DIR)/boot.img of=$(EMMC_IMAGE) seek=1 bs=$(shell expr $(IMAGE_ROOTFS_ALIGNMENT) \* 1024)
	dd conv=notrunc if=$(TARGET_DIR)/boot/zImage of=$(EMMC_IMAGE) seek=1 bs=$(shell expr $(IMAGE_ROOTFS_ALIGNMENT) \* 1024 + $(BOOT_PARTITION_SIZE) \* 1024)
	$(HOST_DIR)/bin/resize2fs $(IMAGE_BUILD_DIR)/$(IMAGE_LINK) $(ROOTFS_PARTITION_SIZE)k
	# Truncate on purpose
	dd if=$(IMAGE_BUILD_DIR)/$(IMAGE_LINK) of=$(EMMC_IMAGE) seek=1 bs=$(shell expr $(IMAGE_ROOTFS_ALIGNMENT) \* 1024 + $(BOOT_PARTITION_SIZE) \* 1024 + $(KERNEL_PARTITION_SIZE) \* 1024)
	mv $(EMMC_IMAGE) $(IMAGE_BUILD_DIR)/$(BOXTYPE)/
	#
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(BOXTYPE)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_recovery_emmc.zip $(BOXTYPE)/$(IMAGE_NAME).img $(BOXTYPE)/imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)

#
# edision-rootfs-image
#
edision-rootfs-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(BOXTYPE)
	mkdir -p $(IMAGE_DIR)
	# kernel
	cp $(TARGET_DIR)/boot/zImage $(IMAGE_BUILD_DIR)/$(BOXTYPE)/kernel.bin
	# rootfs
	cd $(RELEASE_DIR) && \
	tar -cvf $(IMAGE_BUILD_DIR)/$(BOXTYPE)/rootfs.tar . >/dev/null 2>&1; \
	bzip2 $(IMAGE_BUILD_DIR)/$(BOXTYPE)/rootfs.tar
	#
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(BOXTYPE)/imageversion
	echo "rename this file to 'force' to force an update without confirmation" > $(IMAGE_BUILD_DIR)/$(BOXTYPE)/noforce; \
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.zip $(BOXTYPE)/rootfs.tar.bz2 $(BOXTYPE)/kernel.bin $(BOXTYPE)/imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)
	
#
# gfuture-disk-image
#
gfuture-disk-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	mkdir -p $(IMAGE_DIR)
	# splash
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), e4hdultra))
	cp $(SKEL_ROOT)/boot/lcdsplash.bmp $(IMAGE_BUILD_DIR)/
endif
	# kernel
	cp $(TARGET_DIR)/boot/zImage* $(IMAGE_BUILD_DIR)/ #???
	# Create a sparse image block
	dd if=/dev/zero of=$(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_LINK) seek=$(shell expr $(FLASH_IMAGE_ROOTFS_SIZE) \* $(BLOCK_SECTOR)) count=0 bs=$(BLOCK_SIZE)
	$(HOST_DIR)/bin/mkfs.ext4 -F $(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_LINK) -d $(RELEASE_DIR)
	# Error codes 0-3 indicate successfull operation of fsck (no errors or errors corrected)
	$(HOST_DIR)/bin/fsck.ext4 -pvfD $(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_LINK) || [ $? -le 3 ]
	dd if=/dev/zero of=$(EMMC_IMAGE) bs=$(BLOCK_SIZE) count=0 seek=$(shell expr $(EMMC_IMAGE_SIZE) \* $(BLOCK_SECTOR))
	parted -s $(EMMC_IMAGE) mklabel gpt
	parted -s $(EMMC_IMAGE) unit KiB mkpart boot fat16 $(IMAGE_ROOTFS_ALIGNMENT) $(shell expr $(IMAGE_ROOTFS_ALIGNMENT) \+ $(BOOT_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart kernel1 $(KERNEL_PARTITION_OFFSET) $(shell expr $(KERNEL_PARTITION_OFFSET) \+ $(KERNEL_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart rootfs1 ext4 $(ROOTFS_PARTITION_OFFSET) $(shell expr $(ROOTFS_PARTITION_OFFSET) \+ $(ROOTFS_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart kernel2 $(SECOND_KERNEL_PARTITION_OFFSET) $(shell expr $(SECOND_KERNEL_PARTITION_OFFSET) \+ $(KERNEL_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart rootfs2 ext4 $(SECOND_ROOTFS_PARTITION_OFFSET) $(shell expr $(SECOND_ROOTFS_PARTITION_OFFSET) \+ $(ROOTFS_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart kernel3 $(THIRD_KERNEL_PARTITION_OFFSET) $(shell expr $(THIRD_KERNEL_PARTITION_OFFSET) \+ $(KERNEL_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart rootfs3 ext4 $(THIRD_ROOTFS_PARTITION_OFFSET) $(shell expr $(THIRD_ROOTFS_PARTITION_OFFSET) \+ $(ROOTFS_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart kernel4 $(FOURTH_KERNEL_PARTITION_OFFSET) $(shell expr $(FOURTH_KERNEL_PARTITION_OFFSET) \+ $(KERNEL_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart rootfs4 ext4 $(FOURTH_ROOTFS_PARTITION_OFFSET) $(shell expr $(FOURTH_ROOTFS_PARTITION_OFFSET) \+ $(ROOTFS_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart swap linux-swap $(SWAP_PARTITION_OFFSET) $(shell expr $(SWAP_PARTITION_OFFSET) \+ $(SWAP_PARTITION_SIZE))
	parted -s $(EMMC_IMAGE) unit KiB mkpart storage ext4 $(STORAGE_PARTITION_OFFSET) 100%
	dd if=/dev/zero of=$(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) bs=$(BLOCK_SIZE) count=$(shell expr $(BOOT_PARTITION_SIZE) \* $(BLOCK_SECTOR))
	mkfs.msdos -S 512 $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE)
	#
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), e4hdultra protek4k))
	echo "boot emmcflash0.kernel1 'brcm_cma=504M@264M brcm_cma=192M@768M brcm_cma=1024M@2048M root=/dev/mmcblk0p3 rw rootwait 8100s_4.boxmode=5'" > $(IMAGE_BUILD_DIR)/STARTUP
	echo "boot emmcflash0.kernel1 'brcm_cma=504M@264M brcm_cma=192M@768M brcm_cma=1024M@2048M root=/dev/mmcblk0p3 rw rootwait 8100s_4.boxmode=5'" > $(IMAGE_BUILD_DIR)/STARTUP_1
	echo "boot emmcflash0.kernel2 'brcm_cma=504M@264M brcm_cma=192M@768M brcm_cma=1024M@2048M root=/dev/mmcblk0p5 rw rootwait 8100s_4.boxmode=5'" > $(IMAGE_BUILD_DIR)/STARTUP_2
	echo "boot emmcflash0.kernel3 'brcm_cma=504M@264M brcm_cma=192M@768M brcm_cma=1024M@2048M root=/dev/mmcblk0p7 rw rootwait 8100s_4.boxmode=5'" > $(IMAGE_BUILD_DIR)/STARTUP_3
	echo "boot emmcflash0.kernel4 'brcm_cma=504M@264M brcm_cma=192M@768M brcm_cma=1024M@2048M root=/dev/mmcblk0p9 rw rootwait 8100s_4.boxmode=5'" > $(IMAGE_BUILD_DIR)/STARTUP_4
else
	echo "boot emmcflash0.kernel1 'root=/dev/mmcblk0p3 rw rootwait $(BOXTYPE)_4.boxmode=1'" > $(IMAGE_BUILD_DIR)/STARTUP
	echo "boot emmcflash0.kernel1 'root=/dev/mmcblk0p3 rw rootwait $(BOXTYPE)_4.boxmode=1'" > $(IMAGE_BUILD_DIR)/STARTUP_1
	echo "boot emmcflash0.kernel2 'root=/dev/mmcblk0p5 rw rootwait $(BOXTYPE)_4.boxmode=1'" > $(IMAGE_BUILD_DIR)/STARTUP_2
	echo "boot emmcflash0.kernel3 'root=/dev/mmcblk0p7 rw rootwait $(BOXTYPE)_4.boxmode=1'" > $(IMAGE_BUILD_DIR)/STARTUP_3
	echo "boot emmcflash0.kernel4 'root=/dev/mmcblk0p9 rw rootwait $(BOXTYPE)_4.boxmode=1'" > $(IMAGE_BUILD_DIR)/STARTUP_4
endif
	#
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_1 ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_2 ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_3 ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_4 ::
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), e4hdultra))
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/lcdsplash.bmp ::
endif
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), hd51 bre2ze4k h7))
	echo "boot emmcflash0.kernel1 'brcm_cma=520M@248M brcm_cma=192M@768M root=/dev/mmcblk0p3 rw rootwait $(BOXTYPE)_4.boxmode=12'" > $(IMAGE_BUILD_DIR)/STARTUP_1_12
	echo "boot emmcflash0.kernel2 'brcm_cma=520M@248M brcm_cma=192M@768M root=/dev/mmcblk0p5 rw rootwait $(BOXTYPE)_4.boxmode=12'" > $(IMAGE_BUILD_DIR)/STARTUP_2_12
	echo "boot emmcflash0.kernel3 'brcm_cma=520M@248M brcm_cma=192M@768M root=/dev/mmcblk0p7 rw rootwait $(BOXTYPE)_4.boxmode=12'" > $(IMAGE_BUILD_DIR)/STARTUP_3_12
	echo "boot emmcflash0.kernel4 'brcm_cma=520M@248M brcm_cma=192M@768M root=/dev/mmcblk0p9 rw rootwait $(BOXTYPE)_4.boxmode=12'" > $(IMAGE_BUILD_DIR)/STARTUP_4_12
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_1_12 ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_2_12 ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_3_12 ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_4_12 ::
endif
	dd conv=notrunc if=$(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) of=$(EMMC_IMAGE) bs=$(BLOCK_SIZE) seek=$(shell expr $(IMAGE_ROOTFS_ALIGNMENT) \* $(BLOCK_SECTOR))
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), e4hdultra protek4k))
	dd conv=notrunc if=$(TARGET_DIR)/boot/zImage of=$(EMMC_IMAGE) bs=$(BLOCK_SIZE) seek=$(shell expr $(KERNEL_PARTITION_OFFSET) \* $(BLOCK_SECTOR))
else
	dd conv=notrunc if=$(TARGET_DIR)/boot/zImage.dtb of=$(EMMC_IMAGE) bs=$(BLOCK_SIZE) seek=$(shell expr $(KERNEL_PARTITION_OFFSET) \* $(BLOCK_SECTOR))
endif
	$(HOST_DIR)/bin/resize2fs $(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_LINK) $(ROOTFS_PARTITION_SIZE)k
	# Truncate on purpose
	dd if=$(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_LINK) of=$(EMMC_IMAGE) bs=$(BLOCK_SIZE) seek=$(shell expr $(ROOTFS_PARTITION_OFFSET) \* $(BLOCK_SECTOR)) count=$(shell expr $(FLASH_IMAGE_ROOTFS_SIZE) \* $(BLOCK_SECTOR))
	mv $(IMAGE_BUILD_DIR)/disk.img $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/
	#
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_recovery_emmc.zip $(FLASHIMAGE_PREFIX)/disk.img $(FLASHIMAGE_PREFIX)/imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)

#
# gfuture-rootfs-image
#
gfuture-rootfs-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	mkdir -p $(IMAGE_DIR)
	# kernel
ifneq ($(KERNEL_DTB_VER),)
	cp $(TARGET_DIR)/boot/zImage.dtb $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(KERNEL_FILE)
else
	cp $(TARGET_DIR)/boot/zImage $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/$(KERNEL_FILE)
endif
	# rootfs
	cd $(RELEASE_DIR); \
	tar -cvf $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar --exclude=zImage* . > /dev/null 2>&1; \
	bzip2 $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar
	#
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d.%m.%Y-%H.%M') > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.zip $(FLASHIMAGE_PREFIX)/rootfs.tar.bz2 $(FLASHIMAGE_PREFIX)/$(KERNEL_FILE) $(FLASHIMAGE_PREFIX)/imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)

#
# hdfastboot8gb-disk-image
#
hdfastboot8gb-disk-image-$(BOXTYPE): $(ARCHIVE)/$(FLASH_BOOTARGS_SRC) $(ARCHIVE)/$(FLASH_PARTITONS_SRC)
	# Create image
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(BOXTYPE)
	mkdir -p $(IMAGE_DIR)
	#
	unzip -o $(ARCHIVE)/$(FLASH_BOOTARGS_SRC) -d $(IMAGE_BUILD_DIR)
#	unzip -o $(ARCHIVE)/$(FLASH_PARTITONS_SRC) -d $(IMAGE_BUILD_DIR)
	#
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(BOXTYPE)/imageversion
	#
	dd if=/dev/zero of=$(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_LINK) seek=$(shell expr $(FLASH_IMAGE_ROOTFS_SIZE) \* $(BLOCK_SECTOR)) count=0 bs=$(BLOCK_SIZE)
	$(HOST_DIR)/bin/mkfs.ext4 -F $(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_LINK) -d $(RELEASE_DIR)
	# Error codes 0-3 indicate successfull operation of fsck (no errors or errors corrected)
	$(HOST_DIR)/bin/fsck.ext4 -pvfD $(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_LINK) || [ $? -le 3 ]
	dd if=/dev/zero of=$(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) bs=1024 count=$(FLASH_BOOTOPTIONS_PARTITION_SIZE)
	mkfs.msdos -S 512 $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE)
	echo "bootcmd=mmc read 0 0x1000000 0x53D000 0x8000; bootm 0x1000000 bootargs=console=ttyAMA0,115200 root=/dev/mmcblk0p21 rootfstype=ext4" > $(IMAGE_BUILD_DIR)/STARTUP
	echo "bootcmd=mmc read 0 0x3F000000 0x70000 0x4000; bootm 0x3F000000; mmc read 0 0x1FFBFC0 0x52000 0xC800; bootargs=androidboot.selinux=enforcing androidboot.serialno=0123456789 console=ttyAMA0,115200" > $(IMAGE_BUILD_DIR)/STARTUP_RED
	echo "bootcmd=mmc read 0 0x1000000 0x53D000 0x8000; bootm 0x1000000 bootargs=console=ttyAMA0,115200 root=/dev/mmcblk0p21 rootfstype=ext4" > $(IMAGE_BUILD_DIR)/STARTUP_GREEN
	echo "bootcmd=mmc read 0 0x1000000 0x53D000 0x8000; bootm 0x1000000 bootargs=console=ttyAMA0,115200 root=/dev/mmcblk0p21 rootfstype=ext4" > $(IMAGE_BUILD_DIR)/STARTUP_YELLOW
	echo "bootcmd=mmc read 0 0x1000000 0x53D000 0x8000; bootm 0x1000000 bootargs=console=ttyAMA0,115200 root=/dev/mmcblk0p21 rootfstype=ext4" > $(IMAGE_BUILD_DIR)/STARTUP_BLUE
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_RED ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_GREEN ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_YELLOW ::
	mcopy -i $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) -v $(IMAGE_BUILD_DIR)/STARTUP_BLUE ::
	cp $(IMAGE_BUILD_DIR)/$(FLASH_BOOT_IMAGE) $(IMAGE_BUILD_DIR)/$(BOXTYPE)/$(FLASH_BOOT_IMAGE)
	ext2simg -zv $(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_LINK) $(IMAGE_BUILD_DIR)/$(BOXTYPE)/rootfs.fastboot.gz
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), hd60))
	mv $(IMAGE_BUILD_DIR)/bootargs-8gb.bin $(IMAGE_BUILD_DIR)/bootargs.bin
endif
	mv $(IMAGE_BUILD_DIR)/$(BOXTYPE)/bootargs-8gb.bin $(IMAGE_BUILD_DIR)/$(BOXTYPE)/bootargs.bin
	cp $(TARGET_DIR)/boot/$(KERNEL_FILE) $(IMAGE_BUILD_DIR)/$(BOXTYPE)/
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_recovery_emmc.zip *
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)

#
# hdfastboot8gb-rootfs-image
#	
hdfastboot8gb-rootfs-image-$(BOXTYPE):
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(BOXTYPE)
	mkdir -p $(IMAGE_DIR)
	#
	cp $(TARGET_DIR)/boot/$(KERNEL_FILE) $(IMAGE_BUILD_DIR)/$(BOXTYPE)/
	#
	cd $(RELEASE_DIR); \
	tar -cvf $(IMAGE_BUILD_DIR)/$(BOXTYPE)/rootfs.tar --exclude=$(KERNEL_FILE)* . > /dev/null 2>&1; \
	bzip2 $(IMAGE_BUILD_DIR)/$(BOXTYPE)/rootfs.tar
	#
	echo "$(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d.%m.%Y-%H.%M')" > $(IMAGE_BUILD_DIR)/$(BOXTYPE)/imageversion
	echo "$(BOXTYPE)_$(shell date '+%d.%m.%Y-%H.%M')_emmc.zip" > $(IMAGE_BUILD_DIR)/unforce_$(BOXTYPE).txt; \
	echo "Rename the unforce_$(BOXTYPE).txt to force_$(BOXTYPE).txt and move it to the root of your usb-stick" > $(IMAGE_BUILD_DIR)/force_$(BOXTYPE)_READ.ME; \
	echo "When you enter the recovery menu then it will force to install the image $$(cat $(IMAGE_BUILD_DIR)/$(BOXTYPE)/imageversion).zip in the image-slot1" >> $(IMAGE_BUILD_DIR)/force_$(BOXTYPE)_READ.ME; \
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.zip $(BOXTYPE)/unforce_$(BOXTYPE).txt $(BOXTYPE)/force_$(BOXTYPE)_READ.ME $(BOXTYPE)/rootfs.tar.bz2 $(BOXTYPE)/$(KERNEL_FILE) $(BOXTYPE)/imageversion
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)

#
# gbue4k-flash-image
#
gbue4k-flash-image: $(ARCHIVE)/$(INITRD_SRC)
	rm -rf $(IMAGE_BUILD_DIR) || true
	mkdir -p $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	mkdir -p $(IMAGE_DIR)
	# splash
	cp $(SKEL_ROOT)/boot/splash.bin $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/splash.bin
	cp $(SKEL_ROOT)/boot/warning.bin $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	cp $(SKEL_ROOT)/boot/lcdsplash.bin $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	cp $(SKEL_ROOT)/boot/lcdwarning220.bin $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/lcdwarning.bin
	cp $(SKEL_ROOT)/boot/lcdwaitkey220.bin $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/lcdwaitkey.bin
	# kernel
	unzip -o $(ARCHIVE)/$(INITRD_SRC) -d $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)
	cp $(TARGET_DIR)/boot/zImage $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/kernel.bin
	# rootfs
	cd $(RELEASE_DIR); \
	tar -cvf $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar --exclude=kernel.bin . > /dev/null 2>&1; \
	bzip2 $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/rootfs.tar
	#
	echo $(BS_NAME)_$(BS_CYCLE)_$(BOXTYPE)_$(shell date '+%d%m%Y-%H%M%S') > $(IMAGE_BUILD_DIR)/$(FLASHIMAGE_PREFIX)/imageversion
	#
	cd $(IMAGE_BUILD_DIR) && \
	zip -r $(IMAGE_DIR)/$(BOXTYPE)_$(shell git log | grep "^commit" | wc -l)_$(shell date '+%d.%m.%Y-%H.%M')_usb.zip $(FLASHIMAGE_PREFIX)*
	# cleanup
	rm -rf $(IMAGE_BUILD_DIR)
	
#
# image
#
image: release
	$(START_BUILD)
	$(MAKE) image-$(BOXTYPE)
	$(END_BUILD)
	
#
# image-clean
#
image-clean:
	cd $(IMAGE_DIR) && rm -rf *


