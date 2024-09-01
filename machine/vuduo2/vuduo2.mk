#
# Makefile for vuplus duo2
#
BOXARCH = mips
CICAM = ci-cam
SCART = scart
LCD = lcd
FKEYS =

#
# kernel
#
KERNEL_VER             = 3.13.5
KERNEL_SRC             = stblinux-${KERNEL_VER}.tar.bz2
KERNEL_URL		= http://code.vuplus.com/download/release/kernel
KERNEL_CONFIG          = defconfig
KERNEL_DIR             = $(BUILD_TMP)/linux
KERNEL_FILE            = kernel_cfe_auto.bin

KERNEL_PATCHES = \
		kernel-add-support-for-gcc5.patch \
		kernel-add-support-for-gcc6.patch \
		kernel-add-support-for-gcc7.patch \
		kernel-add-support-for-gcc8.patch \
		kernel-add-support-for-gcc9.patch \
		kernel-add-support-for-gcc10.patch \
		rt2800usb_fix_warn_tx_status_timeout_to_dbg.patch \
		add-dmx-source-timecode.patch \
		af9015-output-full-range-SNR.patch \
		af9033-output-full-range-SNR.patch \
		as102-adjust-signal-strength-report.patch \
		as102-scale-MER-to-full-range.patch \
		cxd2820r-output-full-range-SNR.patch \
		dvb-usb-dib0700-disable-sleep.patch \
		dvb_usb_disable_rc_polling.patch \
		it913x-switch-off-PID-filter-by-default.patch \
		tda18271-advertise-supported-delsys.patch \
		mxl5007t-add-no_probe-and-no_reset-parameters.patch \
		linux-tcp_output.patch \
		linux-3.13-gcc-4.9.3-build-error-fixed.patch \
		rtl8712-fix-warnings.patch \
		0001-Support-TBS-USB-drivers-3.13.patch \
		0001-STV-Add-PLS-support.patch \
		0001-STV-Add-SNR-Signal-report-parameters.patch \
		0001-stv090x-optimized-TS-sync-control.patch \
		0002-cp1emu-do-not-use-bools-for-arithmetic.patch \
		0003-log2-give-up-on-gcc-constant-optimizations.patch \
		blindscan2.patch \
		linux_dvb_adapter.patch \
		genksyms_fix_typeof_handling.patch \
		test.patch \
		0001-tuners-tda18273-silicon-tuner-driver.patch \
		T220-kern-13.patch \
		01-10-si2157-Silicon-Labs-Si2157-silicon-tuner-driver.patch \
		02-10-si2168-Silicon-Labs-Si2168-DVB-T-T2-C-demod-driver.patch \
		CONFIG_DVB_SP2.patch \
		dvbsky.patch \
		fix_hfsplus.patch \
		mac80211_hwsim-fix-compiler-warning-on-MIPS.patch \
		prism2fw.patch \
		mm-Move-__vma_address-to-internal.h-to-be-inlined-in-huge_memory.c.patch \
		compile-with-gcc9.patch

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
		$(MAKE) -C $(KERNEL_DIR) ARCH=mips oldconfig
		$(MAKE) -C $(KERNEL_DIR) ARCH=mips CROSS_COMPILE=$(TARGET)- vmlinux modules
		$(MAKE) -C $(KERNEL_DIR) ARCH=mips CROSS_COMPILE=$(TARGET)- DEPMOD=$(DEPMOD) INSTALL_MOD_PATH=$(TARGET_DIR) modules_install
	@touch $@

$(D)/kernel: $(D)/bootstrap $(D)/kernel.do_compile
	install -m 644 $(KERNEL_DIR)/vmlinux $(TARGET_DIR)/boot/
	install -m 644 $(KERNEL_DIR)/System.map $(TARGET_DIR)/boot/System.map-$(BOXARCH)-$(KERNEL_VER)
	gzip -f -9c < $(TARGET_DIR)/boot/vmlinux > $(TARGET_DIR)/boot/$(KERNEL_FILE)
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/build || true
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/source || true
	$(TOUCH)

#
# driver
#
DRIVER_VER = 3.13.5
DRIVER_DATE = 20190429
DRIVER_REV = r0
DRIVER_SRC = vuplus-dvb-proxy-vuduo2-$(DRIVER_VER)-$(DRIVER_DATE).$(DRIVER_REV).tar.gz
DRIVER_URL = http://code.vuplus.com/download/release/vuplus-dvb-proxy

$(ARCHIVE)/$(DRIVER_SRC):
	$(DOWNLOAD) $(DRIVER_URL)/$(DRIVER_SRC)

driver: $(D)/driver
$(D)/driver: $(ARCHIVE)/$(DRIVER_SRC) $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	install -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	tar -xf $(ARCHIVE)/$(DRIVER_SRC) -C $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	$(MAKE) platform_util
	$(MAKE) vmlinuz_initrd
	$(DEPMOD) -ae -b $(TARGET_DIR) -r $(KERNEL_VER)
	$(TOUCH)

#
# platform util
#
UTIL_VER = 15.1
UTIL_DATE = $(DRIVER_DATE)
UTIL_REV = r0
UTIL_SRC = platform-util-vuduo2-$(UTIL_VER)-$(UTIL_DATE).$(UTIL_REV).tar.gz
UTIL_URL = http://code.vuplus.com/download/release/platform-util

$(ARCHIVE)/$(UTIL_SRC):
	$(DOWNLOAD) $(UTIL_URL)/$(UTIL_SRC)

$(D)/platform_util: $(D)/bootstrap $(ARCHIVE)/$(UTIL_SRC)
	$(START_BUILD)
	$(UNTAR)/$(UTIL_SRC)
	install -m 0755 $(BUILD_TMP)/platform-util-vuduo2/* $(TARGET_DIR)/usr/bin
	$(REMOVE)/platform-util-$(KERNEL_TYPE)
	$(TOUCH)

#
# vmlinuz initrd
#
INITRD_DATE = 20130220
INITRD_SRC = vmlinuz-initrd_vuduo2_$(INITRD_DATE).tar.gz
INITRD_URL = http://code.vuplus.com/download/release/kernel
INITRD_NAME = vmlinuz-initrd-7425b0
INITRD_FILE = initrd_cfe_auto.bin

$(ARCHIVE)/$(INITRD_SRC):
	$(DOWNLOAD) $(INITRD_URL)/$(INITRD_SRC)

$(D)/vmlinuz_initrd: $(D)/bootstrap $(ARCHIVE)/$(INITRD_SRC)
	$(START_BUILD)
	tar -xf $(ARCHIVE)/$(INITRD_SRC) -C $(TARGET_DIR)/boot
	$(TOUCH)

#
# release
#
release-vuduo2:
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	cp -f $(BASE_DIR)/machine/$(BOXTYPE)/files/fstab $(RELEASE_DIR)/etc/
	cp -pa $(TARGET_DIR)/lib/modules/$(KERNEL_VER) $(RELEASE_DIR)/lib/modules

#
# flashimage
#
FLASHIMAGE_PREFIX = vuplus/duo2

FLASHSIZE = 1024
ROOTFS_FILE = root_cfe_auto.jffs2
IMAGE_FSTYPES ?= ubi
IMAGE_NAME = root_cfe_auto
UBI_VOLNAME = rootfs
MKUBIFS_ARGS = -m 2048 -e 126976 -c 8192
UBINIZE_ARGS = -m 2048 -p 128KiB
BOOTLOGO_FILENAME = splash.bin
BOOT_UPDATE_TEXT = "This file forces a reboot after the update."
BOOT_UPDATE_FILE = reboot.update

