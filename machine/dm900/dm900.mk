#
# Makefile for dreambox dm900
#
BOXARCH = arm
CICAM = ci-cam
LCD = lcd
FKEYS = fkeys

#
#
#
KERNEL_VER             = 3.14-1.17
KERNEL_SRC_VER         = 3.14.79
KERNEL_SRC             = linux-${KERNEL_SRC_VER}.tar.xz
KERNEL_URL             = https://cdn.kernel.org/pub/linux/kernel/v3.x
KERNEL_CONFIG          = defconfig
KERNEL_DIR             = $(BUILD_TMP)/linux-$(KERNEL_SRC_VER)
KERNEL_DTB	       = dreambox-dm900.dtb
KERNEL_FILE	       = kernel.bin

KERNEL_PATCHES = \
		linux-dreambox-3.14-6fa88d2001194cbff63ad94cb713b6cd5ea02739.patch \
		kernel-fake-3.14.patch \
		dvbs2x.patch \
		0001-Support-TBS-USB-drivers.patch \
		0001-STV-Add-PLS-support.patch \
		0001-STV-Add-SNR-Signal-report-parameters.patch \
		0001-stv090x-optimized-TS-sync-control.patch \
		genksyms_fix_typeof_handling.patch \
		blindscan2.patch \
		0001-tuners-tda18273-silicon-tuner-driver.patch \
		01-10-si2157-Silicon-Labs-Si2157-silicon-tuner-driver.patch \
		02-10-si2168-Silicon-Labs-Si2168-DVB-T-T2-C-demod-driver.patch \
		0003-cxusb-Geniatech-T230-support.patch \
		CONFIG_DVB_SP2.patch \
		dvbsky.patch \
		rtl2832u-2.patch \
		0004-log2-give-up-on-gcc-constant-optimizations.patch \
		0005-uaccess-dont-mark-register-as-const.patch \
		0006-makefile-silence-packed-not-aligned-warn.patch \
		0007-overlayfs.patch \
		move-default-dialect-to-SMB3.patch \
		fix-multiple-defs-yyloc.patch \
		fix-build-with-binutils-2.41.patch

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
		$(MAKE) -C $(KERNEL_DIR) ARCH=arm CROSS_COMPILE=$(TARGET)- $(KERNEL_DTB) zImage modules
		$(MAKE) -C $(KERNEL_DIR) ARCH=arm CROSS_COMPILE=$(TARGET)- DEPMOD=$(DEPMOD) INSTALL_MOD_PATH=$(TARGET_DIR) modules_install
		$(DEPMOD) -ae -b $(TARGET_DIR) -F $(KERNEL_DIR)/System.map -r $(KERNEL_VER)-$(BOXTYPE)
	@touch $@

$(D)/kernel: $(D)/bootstrap $(D)/kernel.do_compile
	install -m 644 $(KERNEL_DIR)/vmlinux $(TARGET_DIR)/boot/vmlinux-arm-$(KERNEL_VER)
	install -m 644 $(KERNEL_DIR)/System.map $(TARGET_DIR)/boot/System.map-arm-$(KERNEL_VER)
	cp $(KERNEL_DIR)/arch/arm/boot/zImage $(TARGET_DIR)/boot/
	cat $(KERNEL_DIR)/arch/arm/boot/zImage $(KERNEL_DIR)/arch/arm/boot/dts/$(KERNEL_DTB) > $(TARGET_DIR)/boot/zImage.dtb
	cp $(KERNEL_DIR)/arch/arm/boot/dts/$(KERNEL_DTB) $(TARGET_DIR)/boot/
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/build || true
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/source || true
	$(TOUCH)
	
#
# driver
#
DRIVER_VER = 3.14-1.17
DRIVER_DATE = 20200226
DRIVER_SRC = dreambox-dvb-modules_$(DRIVER_VER)-$(BOXTYPE)-$(DRIVER_DATE)_$(BOXTYPE).tar.xz

$(ARCHIVE)/$(DRIVER_SRC):
	$(DOWNLOAD) https://github.com/oe-mirrors/dreambox/raw/main/$(DRIVER_SRC)

driver: $(D)/driver	
$(D)/driver: $(ARCHIVE)/$(DRIVER_SRC) $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	install -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)-$(BOXTYPE)/extra
	tar -xf $(ARCHIVE)/$(DRIVER_SRC) -C $(TARGET_DIR)/lib/modules/$(KERNEL_VER)-$(BOXTYPE)/extra --transform='s/.*\///'
	find $(TARGET_DIR)/lib/modules/$(KERNEL_VER)-$(BOXTYPE)/extra -type d -empty -delete
	$(DEPMOD) -ae -b $(TARGET_DIR) -r $(KERNEL_VER)-$(BOXTYPE)
	$(TOUCH)

#
# release-dm900
#
release-dm900:
	cp -pa $(TARGET_DIR)/lib/modules/$(KERNEL_VER)-$(BOXTYPE) $(RELEASE_DIR)/lib/modules
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	cp -f $(BASE_DIR)/machine/$(BOXTYPE)/files/fstab $(RELEASE_DIR)/etc/
	
#
# image
#
image-dm900:
	$(MAKE) dm-rootfs-image-$(BOXTYPE)

