#
# Makefile for vuplus ultimo 4k
#
BOXARCH = arm
CICAM = ci-cam
SCART = scart
LCD = tftlcd
FKEYS =

#
# kernel
#
KERNEL_VER             = 3.14.28-1.12
KERNEL_SRC_VER         = 3.14-1.12
KERNEL_SRC             = stblinux-${KERNEL_SRC_VER}.tar.bz2
KERNEL_URL		= http://code.vuplus.com/download/release/kernel
KERNEL_CONFIG          = defconfig
KERNEL_DIR             = $(BUILD_TMP)/linux
KERNEL_FILE	       = kernel_auto.bin

KERNEL_PATCHES = \
		3_14_bcm_genet_disable_warn.patch \
		3_14_linux_dvb-core.patch \
		3_14_dvbs2x.patch \
		3_14_dmx_source_dvr.patch \
		3_14_rt2800usb_fix_warn_tx_status_timeout_to_dbg.patch \
		3_14_usb_core_hub_msleep.patch \
		3_14_rtl8712_fix_build_error.patch \
		3_14_kernel-add-support-for-gcc6.patch \
		3_14_kernel-add-support-for-gcc7.patch \
		3_14_kernel-add-support-for-gcc8.patch \
		3_14_kernel-add-support-for-gcc9.patch \
		3_14_kernel-add-support-for-gcc10.patch \
		3_14_0001-Support-TBS-USB-drivers.patch \
		3_14_0001-STV-Add-PLS-support.patch \
		3_14_0001-STV-Add-SNR-Signal-report-parameters.patch \
		3_14_0001-stv090x-optimized-TS-sync-control.patch \
		3_14_blindscan2.patch \
		3_14_genksyms_fix_typeof_handling.patch \
		3_14_0001-tuners-tda18273-silicon-tuner-driver.patch \
		3_14_01-10-si2157-Silicon-Labs-Si2157-silicon-tuner-driver.patch \
		3_14_02-10-si2168-Silicon-Labs-Si2168-DVB-T-T2-C-demod-driver.patch \
		3_14_0003-cxusb-Geniatech-T230-support.patch \
		3_14_CONFIG_DVB_SP2.patch \
		3_14_dvbsky.patch \
		3_14_rtl2832u-2.patch \
		3_14_0004-log2-give-up-on-gcc-constant-optimizations.patch \
		3_14_0005-uaccess-dont-mark-register-as-const.patch \
		3_14_0006-makefile-disable-warnings.patch \
		3_14_linux_dvb_adapter.patch \
		bcmsysport_3.14.28-1.12.patch \
		linux_prevent_usb_dma_from_bmem.patch \
		fix-multiple-defs-yyloc.patch

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
DRIVER_VER = 3.14.28
DRIVER_DATE = 20190424
DRIVER_REV = r0
DRIVER_SRC = vuplus-dvb-proxy-vuultimo4k-$(DRIVER_VER)-$(DRIVER_DATE).$(DRIVER_REV).tar.gz
DRIVER_URL = http://code.vuplus.com/download/release/vuplus-dvb-proxy

$(ARCHIVE)/$(DRIVER_SRC):
	$(DOWNLOAD) $(DRIVER_URL)/$(DRIVER_SRC)

driver: $(D)/driver
$(D)/driver: $(ARCHIVE)/$(DRIVER_SRC) $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	install -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	tar -xf $(ARCHIVE)/$(DRIVER_SRC) -C $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	$(MAKE) platform_util
	$(MAKE) libgles
	$(MAKE) vmlinuz_initrd
	$(DEPMOD) -ae -b $(TARGET_DIR) -r $(KERNEL_VER)
	$(TOUCH)

#
# platform util
#
UTIL_VER = 17.1
UTIL_DATE = $(DRIVER_DATE)
UTIL_REV = r0
UTIL_SRC = platform-util-vuultimo4k-$(UTIL_VER)-$(UTIL_DATE).$(UTIL_REV).tar.gz
UTIL_URL = http://code.vuplus.com/download/release/platform-util

$(ARCHIVE)/$(UTIL_SRC):
	$(DOWNLOAD) $(UTIL_URL)/$(UTIL_SRC)

$(D)/platform_util: $(D)/bootstrap $(ARCHIVE)/$(UTIL_SRC)
	$(START_BUILD)
	$(UNTAR)/$(UTIL_SRC)
	install -m 0755 $(BUILD_TMP)/platform-util-vuultimo4k/* $(TARGET_DIR)/usr/bin
	$(REMOVE)/platform-util-vuultimo4k
	$(TOUCH)

#
# libgles
#
GLES_VER = 17.1
GLES_DATE = $(DRIVER_DATE)
GLES_REV = r0
GLES_SRC = libgles-vuultimo4k-$(GLES_VER)-$(GLES_DATE).$(GLES_REV).tar.gz
GLES_URL = http://code.vuplus.com/download/release/libgles

$(ARCHIVE)/$(GLES_SRC):
	$(DOWNLOAD) $(GLES_URL)/$(GLES_SRC)

$(D)/libgles: $(D)/bootstrap $(ARCHIVE)/$(GLES_SRC)
	$(START_BUILD)
	$(UNTAR)/$(GLES_SRC)
	install -m 0755 $(BUILD_TMP)/libgles-vuultimo4k/lib/* $(TARGET_LIB_DIR)
	ln -sf libv3ddriver.so $(TARGET_LIB_DIR)/libEGL.so
	ln -sf libv3ddriver.so $(TARGET_LIB_DIR)/libGLESv2.so
	cp -a $(BUILD_TMP)/libgles-vuultimo4k/include/* $(TARGET_INCLUDE_DIR)
	$(REMOVE)/libgles-vuultimo4k
	$(TOUCH)

#
# vmlinuz initrd
#
INITRD_DATE = 20170209
INITRD_SRC = vmlinuz-initrd_vuultimo4k_$(INITRD_DATE).tar.gz
INITRD_URL = http://code.vuplus.com/download/release/kernel
INITRD_NAME = vmlinuz-initrd-7445d0
INITRD_FILE = initrd_auto.bin

$(ARCHIVE)/$(INITRD_SRC):
	$(DOWNLOAD) $(INITRD_URL)/$(INITRD_SRC)

$(D)/vmlinuz_initrd: $(D)/bootstrap $(ARCHIVE)/$(INITRD_SRC)
	$(START_BUILD)
	tar -xf $(ARCHIVE)/$(INITRD_SRC) -C $(TARGET_DIR)/boot
	$(TOUCH)

#
# release
#
release-vuultimo4k:
	cp -pa $(TARGET_DIR)/lib/modules/$(KERNEL_VER) $(RELEASE_DIR)/lib/modules
	rm -f $(RELEASE_DIR)/lib/modules/$(KERNEL_VER)/extra/fpga_directc.ko
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	cp -f $(BASE_DIR)/machine/$(BOXTYPE)/files/fstab $(RELEASE_DIR)/etc/

#
# image
#
FLASHIMAGE_PREFIX = vuplus/ultimo4k
	
KERNEL1_FILE = kernel1_auto.bin
KERNEL2_FILE = kernel2_auto.bin
KERNEL3_FILE = kernel3_auto.bin
KERNEL4_FILE = kernel4_auto.bin

BOOT_UPDATE_TEXT = "rename this file to 'force' to force an update without confirmation"
BOOT_UPDATE_FILE = noforce
PART_TEXT = This file forces creating partitions.
PART_FILE = mkpart.update

image-vuultimo4k:
	$(MAKE) vuplus-rootfs-image-$(BOXTYPE) vuplus-multi-rootfs-image-$(BOXTYPE) vuplus-online-image-$(BOXTYPE)

