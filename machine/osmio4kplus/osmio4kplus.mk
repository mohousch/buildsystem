#
# Makefile for edision osmio 4k plus
#
BOXARCH = arm
CICAM = ci-cam
SCART = scart
LCD = lcd
FKEYS =

#
# kernel
#
KERNEL_VER             = 5.15.0
KERNEL_SRC_VER         = 5.15
KERNEL_SRC             = linux-edision-$(KERNEL_SRC_VER).tar.gz
KERNEL_URL             = http://source.mynonpublic.com/edision
KERNEL_CONFIG          = defconfig
KERNEL_DIR             = $(BUILD_TMP)/linux-brcmstb-$(KERNEL_SRC_VER)

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
	install -m 644 $(KERNEL_DIR)/System.map $(TARGET_DIR)/boot/System.map-arm-$(KERNEL_VER)
	cp $(KERNEL_DIR)/arch/arm/boot/zImage $(TARGET_DIR)/boot/
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/build || true
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/source || true
	$(TOUCH)

#
# driver
#
DRIVER_VER = 5.15.0
DRIVER_DATE = 20211228
DRIVER_REV =
DRIVER_SRC = $(BOXTYPE)-drivers-$(DRIVER_VER)-$(DRIVER_DATE).zip
DRIVER_URL = http://source.mynonpublic.com/edision

LIBGLES_VER = 2.0
LIBGLES_DIR = edision-libv3d-$(LIBGLES_VER)
LIBGLES_SRC = edision-libv3d-$(LIBGLES_VER).tar.xz
LIBGLES_URL = http://source.mynonpublic.com/edision

$(ARCHIVE)/$(DRIVER_SRC):
	$(DOWNLOAD) $(DRIVER_URL)/$(DRIVER_SRC)

$(ARCHIVE)/$(LIBGLES_SRC):
	$(DOWNLOAD) $(LIBGLES_URL)/$(LIBGLES_SRC)

$(D)/driver: $(ARCHIVE)/$(DRIVER_SRC) $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	install -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	unzip -o $(ARCHIVE)/$(DRIVER_SRC) -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	for i in brcmstb-$(BOXTYPE) brcmstb-decoder ci si2183 avl6862 avl6261; do \
		echo $$i >> $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/modules.default; \
	done
	$(MAKE) install-v3ddriver
#	$(MAKE) wlan-qcom
	$(DEPMOD) -ae -b $(TARGET_DIR) -r $(KERNEL_VER)
	$(TOUCH)

#
#
#
$(D)/install-v3ddriver: $(ARCHIVE)/$(LIBGLES_SRC)
	install -d $(TARGET_LIB_DIR)
	$(REMOVE)/$(LIBGLES_DIR)
	$(UNTAR)/$(LIBGLES_SRC)
	cp -a $(BUILD_TMP)/$(LIBGLES_DIR)/* $(TARGET_DIR)/usr/
	ln -sf libv3ddriver.so.$(LIBGLES_VER) $(TARGET_LIB_DIR)/libEGL.so
	ln -sf libv3ddriver.so.$(LIBGLES_VER) $(TARGET_LIB_DIR)/libGLESv2.so
	$(REMOVE)/$(LIBGLES_DIR)

#
# wlan-qcom osmio4k | osmio4kplus | osmini4
#
WLAN_QCOM_VER    = 4.5.25.55
WLAN_QCOM_DIR    = qcacld-2.0-$(WLAN_QCOM_VER)
WLAN_QCOM_SOURCE = qcacld-2.0-$(WLAN_QCOM_VER).tar.gz
WLAN_QCOM_URL    = https://source.codeaurora.org/external/wlan/qcacld-2.0/snapshot

$(ARCHIVE)/$(WLAN_QCOM_SOURCE):
	$(DOWNLOAD) $(WLAN_QCOM_URL)/$(WLAN_QCOM_SOURCE)

WLAN_QCOM_PATCH  = \
	qcacld-2.0-support.patch

$(D)/wlan-qcom: $(D)/bootstrap $(D)/kernel $(D)/wlan-qcom-firmware $(ARCHIVE)/$(WLAN_QCOM_SOURCE)
	$(START_BUILD)
	$(REMOVE)/$(WLAN_QCOM_DIR)
	$(UNTAR)/$(WLAN_QCOM_SOURCE)
	$(CHDIR)/$(WLAN_QCOM_DIR); \
		$(call apply_patches, $(WLAN_QCOM_PATCH)); \
		$(MAKE) KERNEL_SRC=$(KERNEL_DIR) ARCH=arm CROSS_COMPILE=$(TARGET)- CROSS_COMPILE_COMPAT=$(TARGET)- all; \
	install -m 644 wlan.ko $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	$(REMOVE)/$(WLAN_QCOM_DIR)
	$(TOUCH)

#
# wlan-qcom-firmware
#
WLAN_QCOM_FIRMWARE_VER    = qca6174_v2
WLAN_QCOM_FIRMWARE_DIR    = firmware-$(WLAN_QCOM_FIRMWARE_VER)
WLAN_QCOM_FIRMWARE_SOURCE = firmware-$(WLAN_QCOM_FIRMWARE_VER).zip
WLAN_QCOM_FIRMWARE_URL    = http://source.mynonpublic.com/edision

$(ARCHIVE)/$(WLAN_QCOM_FIRMWARE_SOURCE):
	$(DOWNLOAD) $(WLAN_QCOM_FIRMWARE_URL)/$(WLAN_QCOM_FIRMWARE_SOURCE)

$(D)/wlan-qcom-firmware: $(D)/bootstrap $(ARCHIVE)/$(WLAN_QCOM_FIRMWARE_SOURCE)
	$(START_BUILD)
	$(REMOVE)/$(WLAN_QCOM_FIRMWARE_DIR)
	unzip -o $(ARCHIVE)/$(WLAN_QCOM_FIRMWARE_SOURCE) -d $(BUILD_TMP)/$(WLAN_QCOM_FIRMWARE_DIR)
	$(CHDIR)/$(WLAN_QCOM_FIRMWARE_DIR); \
		install -d $(TARGET_DIR)/lib/firmware/ath10k/QCA6174/hw3.0; \
		install -m 644 board.bin $(TARGET_DIR)/lib/firmware/ath10k/QCA6174/hw3.0/board.bin; \
		install -m 644 firmware-4.bin $(TARGET_DIR)/lib/firmware/ath10k/QCA6174/hw3.0/firmware-4.bin; \
		install -d $(TARGET_DIR)/lib/firmware/wlan; \
		install -m 644 bdwlan30.bin $(TARGET_DIR)/lib/firmware/bdwlan30.bin; \
		install -m 644 otp30.bin $(TARGET_DIR)/lib/firmware/otp30.bin; \
		install -m 644 qwlan30.bin $(TARGET_DIR)/lib/firmware/qwlan30.bin; \
		install -m 644 utf30.bin $(TARGET_DIR)/lib/firmware/utf30.bin; \
		install -m 644 wlan/cfg.dat $(TARGET_DIR)/lib/firmware/wlan/cfg.dat; \
		install -m 644 wlan/qcom_cfg.ini $(TARGET_DIR)/lib/firmware/wlan/qcom_cfg.ini; \
		install -m 644 btfw32.tlv $(TARGET_DIR)/lib/firmware/btfw32.tlv
	$(REMOVE)/$(WLAN_QCOM_FIRMWARE_DIR)
	$(TOUCH)

#
# release
#
release-osmio4kplus:
	cp -pa $(TARGET_DIR)/lib/modules/$(KERNEL_VER) $(RELEASE_DIR)/lib/modules
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	cp -f $(BASE_DIR)/machine/$(BOXTYPE)/files/fstab $(RELEASE_DIR)/etc/

#
# image
#
IMAGE_NAME = emmc
IMAGE_LINK = $(IMAGE_NAME).ext4

# emmc image
EMMC_IMAGE = $(IMAGE_BUILD_DIR)/$(IMAGE_NAME).img
EMMC_IMAGE_SIZE = 7634944

# partition offsets/sizes
IMAGE_ROOTFS_ALIGNMENT = 1024
BOOT_PARTITION_SIZE    = 3072
KERNEL_PARTITION_SIZE  = 8192
ROOTFS_PARTITION_SIZE  = 1767424

KERNEL1_PARTITION_OFFSET = $(shell expr $(IMAGE_ROOTFS_ALIGNMENT)   + $(BOOT_PARTITION_SIZE))
ROOTFS1_PARTITION_OFFSET = $(shell expr $(KERNEL1_PARTITION_OFFSET) + $(KERNEL_PARTITION_SIZE))

KERNEL2_PARTITION_OFFSET = $(shell expr $(ROOTFS1_PARTITION_OFFSET) + $(ROOTFS_PARTITION_SIZE))
ROOTFS2_PARTITION_OFFSET = $(shell expr $(KERNEL2_PARTITION_OFFSET) + $(KERNEL_PARTITION_SIZE))

KERNEL3_PARTITION_OFFSET = $(shell expr $(ROOTFS2_PARTITION_OFFSET) + $(ROOTFS_PARTITION_SIZE))
ROOTFS3_PARTITION_OFFSET = $(shell expr $(KERNEL3_PARTITION_OFFSET) + $(KERNEL_PARTITION_SIZE))

KERNEL4_PARTITION_OFFSET = $(shell expr $(ROOTFS3_PARTITION_OFFSET) + $(ROOTFS_PARTITION_SIZE))
ROOTFS4_PARTITION_OFFSET = $(shell expr $(KERNEL4_PARTITION_OFFSET) + $(KERNEL_PARTITION_SIZE))

SWAP_PARTITION_OFFSET = $(shell expr $(ROOTFS4_PARTITION_OFFSET) + $(ROOTFS_PARTITION_SIZE))

image-osmio4kplus:
	$(MAKE) edision-disk-image-$(BOXTYPE) edision-rootfs-image-$(BOXTYPE) edision-online-image-$(BOXTYPE)

