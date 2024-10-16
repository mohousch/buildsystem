#
# Makefile for protek 4k
#
BOXARCH = arm
CICAM = ci-cam
LCD = 4-digits
FKEYS =

#
# kernel
#
KERNEL_VER             = 4.10.12
KERNEL_DATE            = 20180424
KERNEL_SRC             = ceryon-linux-$(KERNEL_VER)-arm.tar.gz
KERNEL_URL             = http://source.mynonpublic.com/ceryon
KERNEL_CONFIG          = defconfig
KERNEL_DIR             = $(BUILD_TMP)/linux-$(KERNEL_VER)
KERNEL_FILE	       = kernel.bin

KERNEL_PATCHES = \
		TBS-fixes-for-4.10-kernel.patch \
		0001-Support-TBS-USB-drivers-for-4.6-kernel.patch \
		0001-TBS-fixes-for-4.6-kernel.patch \
		0001-STV-Add-PLS-support.patch \
		0001-STV-Add-SNR-Signal-report-parameters.patch \
		blindscan2.patch \
		dvbs2x.patch \
		0001-stv090x-optimized-TS-sync-control.patch \
		reserve_dvb_adapter_0.patch \
		blacklist_mmc0.patch \
		export_pmpoweroffprepare.patch \
		4.10.12_fix-multiple-defs-yyloc.patch \
		v3-1-3-media-si2157-Add-support-for-Si2141-A10.patch \
		v3-2-3-media-si2168-add-support-for-Si2168-D60.patch \
		v3-3-3-media-dvbsky-MyGica-T230C-support.patch \
		v3-3-4-media-dvbsky-MyGica-T230C-support.patch \
		v3-3-5-media-dvbsky-MyGica-T230C-support.patch \
		0002-cp1emu-do-not-use-bools-for-arithmetic.patch \
		move-default-dialect-to-SMB3.patch \
		add-more-devices-rtl8xxxu.patch \
		0005-xbox-one-tuner-4.10.patch \
		0006-dvb-media-tda18250-support-for-new-silicon-tuner.patch

$(ARCHIVE)/$(KERNEL_SRC):
	$(DOWNLOAD) $(KERNEL_URL)/$(KERNEL_SRC)

$(D)/kernel.do_prepare: $(ARCHIVE)/$(KERNEL_SRC) $(BASE_DIR)/machine/$(BOXTYPE)/files/$(KERNEL_CONFIG)
	$(START_BUILD)
	rm -rf $(KERNEL_DIR)
	$(UNTAR)/$(KERNEL_SRC)
	set -e; cd $(KERNEL_DIR); \
		for i in $(KERNEL_PATCHES); do \
			echo -e "==> $(TERM_RED)Applying Patch:$(TERM_NORMAL) $$i"; \
			$(APATCH) $(BASE_DIR)/machine/$(BOXTYPE)/patches/$$i; \
		done
	install -m 644 $(BASE_DIR)/machine/$(BOXTYPE)/files/$(KERNEL_CONFIG) $(KERNEL_DIR)/.config
ifeq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug))
	@echo "Using kernel debug"
	@grep -v "CONFIG_PRINTK" "$(KERNEL_DIR)/.config" > $(KERNEL_DIR)/.config.tmp
	cp $(KERNEL_DIR)/.config.tmp $(KERNEL_DIR)/.config
	@echo "CONFIG_PRINTK=y" >> $(KERNEL_DIR)/.config
	@echo "CONFIG_PRINTK_TIME=y" >> $(KERNEL_DIR)/.config
endif
	@touch $@

$(D)/kernel.do_compile: $(D)/kernel.do_prepare
	set -e; cd $(KERNEL_DIR); \
		$(MAKE) -C $(KERNEL_DIR) ARCH=arm oldconfig
		$(MAKE) -C $(KERNEL_DIR) ARCH=arm CROSS_COMPILE=$(TARGET)- zImage modules
		$(MAKE) -C $(KERNEL_DIR) ARCH=arm CROSS_COMPILE=$(TARGET)- DEPMOD=$(DEPMOD) INSTALL_MOD_PATH=$(TARGET_DIR) modules_install
	@touch $@

$(D)/kernel: $(D)/bootstrap $(D)/kernel.do_compile
	install -m 644 $(KERNEL_DIR)/vmlinux $(TARGET_DIR)/boot/vmlinux-arm-$(KERNEL_VER)
	install -m 644 $(KERNEL_DIR)/System.map $(TARGET_DIR)/boot/System.map-$(BOXARCH)-$(KERNEL_VER)
	cp $(KERNEL_DIR)/arch/arm/boot/zImage $(TARGET_DIR)/boot/
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/build || true
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/source || true
	$(TOUCH)

#
# driver
#
DRIVER_DATE = 20191101
DRIVER_VER = 4.10.12-$(DRIVER_DATE)
DRIVER_SRC = protek4k-drivers-$(DRIVER_VER).zip
DRIVER_URL = http://source.mynonpublic.com/ceryon

LIBGLES_DATE = 20191101
LIBGLES_SRC  = 8100s-v3ddriver-$(LIBGLES_DATE).zip
LIBGLES_URL  = https://source.mynonpublic.com/ceryon

LIBGLES_HEADERS = hd-v3ddriver-headers.tar.gz

$(ARCHIVE)/$(DRIVER_SRC):
	$(DOWNLOAD) $(DRIVER_URL)/$(DRIVER_SRC)

$(ARCHIVE)/$(LIBGLES_SRC):
	$(DOWNLOAD) $(LIBGLES_URL)/$(LIBGLES_SRC)

$(ARCHIVE)/$(LIBGLES_HEADERS):
	$(DOWNLOAD) $(LIBGLES_URL)/$(LIBGLES_HEADERS)

driver: $(D)/driver
$(D)/driver: $(ARCHIVE)/$(DRIVER_SRC) $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	install -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	unzip -o $(ARCHIVE)/$(DRIVER_SRC) -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	ls $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra | sed s/.ko//g > $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/modules.default
	sed -i "s/_4/_4 boxmode=\$$BOXMODE/g" $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/modules.default
	#$(MAKE) install-v3ddriver
	#$(MAKE) install-v3ddriver-header
	$(TOUCH)

$(D)/install-v3ddriver: $(ARCHIVE)/$(LIBGLES_SRC)
	install -d $(TARGET_LIB_DIR)
	unzip -o $(ARCHIVE)/$(LIBGLES_SRC) -d $(TARGET_LIB_DIR)
	#patchelf --set-soname libv3ddriver.so $(TARGET_LIB_DIR)/libv3ddriver.so
	ln -sf libv3ddriver.so $(TARGET_LIB_DIR)/libEGL.so.1.4
	ln -sf libEGL.so.1.4 $(TARGET_LIB_DIR)/libEGL.so.1
	ln -sf libEGL.so.1 $(TARGET_LIB_DIR)/libEGL.so
	ln -sf libv3ddriver.so $(TARGET_LIB_DIR)/libGLESv1_CM.so.1.1
	ln -sf libGLESv1_CM.so.1.1 $(TARGET_LIB_DIR)/libGLESv1_CM.so.1
	ln -sf libGLESv1_CM.so.1 $(TARGET_LIB_DIR)/libGLESv1_CM.so
	ln -sf libv3ddriver.so $(TARGET_LIB_DIR)/libGLESv2.so.2.0
	ln -sf libGLESv2.so.2.0 $(TARGET_LIB_DIR)/libGLESv2.so.2
	ln -sf libGLESv2.so.2 $(TARGET_LIB_DIR)/libGLESv2.so
	ln -sf libv3ddriver.so $(TARGET_LIB_DIR)/libgbm.so.1
	ln -sf libgbm.so.1 $(TARGET_LIB_DIR)/libgbm.so

$(D)/install-v3ddriver-header: $(ARCHIVE)/$(LIBGLES_HEADERS)
	install -d $(TARGET_INCLUDE_DIR)
	tar -xf $(ARCHIVE)/$(LIBGLES_HEADERS) -C $(TARGET_INCLUDE_DIR)
#
# release
#
release-protek4k:
	cp -pa $(TARGET_DIR)/lib/modules/$(KERNEL_VER) $(RELEASE_DIR)/lib/modules
#	install -m 0755 $(SKEL_ROOT)/etc/init.d/mmcblk-by-name $(RELEASE_DIR)/etc/init.d/mmcblk-by-name
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	cp -f $(BASE_DIR)/machine/$(BOXTYPE)/files/fstab $(RELEASE_DIR)/etc/

#
# image
#
FLASHIMAGE_PREFIX = $(BOXTYPE)

# general
FLASH_IMAGE_NAME = disk
FLASH_BOOT_IMAGE = boot.img
FLASH_IMAGE_LINK = $(FLASH_IMAGE_NAME).ext4
FLASH_IMAGE_ROOTFS_SIZE = 294912

# emmc image
EMMC_IMAGE_SIZE = 3817472
EMMC_IMAGE = $(IMAGE_BUILD_DIR)/$(FLASH_IMAGE_NAME).img

# partition sizes
BLOCK_SIZE = 512
BLOCK_SECTOR = 2
IMAGE_ROOTFS_ALIGNMENT = 1024

BOOT_PARTITION_SIZE = 3072
KERNEL_PARTITION_SIZE = 8192
SWAP_PARTITION_SIZE = 262144
ROOTFS_PARTITION_SIZE = 768000

KERNEL_PARTITION_OFFSET = $(shell expr $(IMAGE_ROOTFS_ALIGNMENT) \+ $(BOOT_PARTITION_SIZE))
ROOTFS_PARTITION_OFFSET = $(shell expr $(KERNEL_PARTITION_OFFSET) \+ $(KERNEL_PARTITION_SIZE))

# calc the offsets
SECOND_KERNEL_PARTITION_OFFSET = $(shell expr $(ROOTFS_PARTITION_OFFSET) \+ $(ROOTFS_PARTITION_SIZE))
SECOND_ROOTFS_PARTITION_OFFSET = $(shell expr $(SECOND_KERNEL_PARTITION_OFFSET) \+ $(KERNEL_PARTITION_SIZE))

THIRD_KERNEL_PARTITION_OFFSET = $(shell expr $(SECOND_ROOTFS_PARTITION_OFFSET) \+ $(ROOTFS_PARTITION_SIZE))
THIRD_ROOTFS_PARTITION_OFFSET = $(shell expr $(THIRD_KERNEL_PARTITION_OFFSET) \+ $(KERNEL_PARTITION_SIZE))

FOURTH_KERNEL_PARTITION_OFFSET = $(shell expr $(THIRD_ROOTFS_PARTITION_OFFSET) \+ $(ROOTFS_PARTITION_SIZE))
FOURTH_ROOTFS_PARTITION_OFFSET = $(shell expr $(FOURTH_KERNEL_PARTITION_OFFSET) \+ $(KERNEL_PARTITION_SIZE))

SWAP_PARTITION_OFFSET = $(shell expr $(FOURTH_ROOTFS_PARTITION_OFFSET) \+ $(ROOTFS_PARTITION_SIZE))
STORAGE_PARTITION_OFFSET = $(shell expr $(SWAP_PARTITION_OFFSET) \+ $(SWAP_PARTITION_SIZE))

image-protek4k:
	$(MAKE) gfuture-disk-image-$(BOXTYPE) gfuture-multi-rootfs-image-$(BOXTYPE) gfuture-online-image-$(BOXTYPE)

