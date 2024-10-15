#
# Makefile for gigablue 800se
#
BOXARCH = mips
CICAM = ci-cam
SCART = scart
LCD = 4-digits
FKEYS = fkeys

#
# kernel
#
KERNEL_VER             = 3.9.6
KERNEL_DATE            = 20140904
KERNEL_SRC             = gigablue-linux-$(KERNEL_VER)-$(KERNEL_DATE).tgz
KERNEL_URL             = http://source.mynonpublic.com/gigablue/linux
KERNEL_CONFIG          = defconfig
KERNEL_DIR             = $(BUILD_TMP)/linux-$(KERNEL_VER)
KERNEL_FILE 	       = kernel.bin

KERNEL_PATCHES  = \
		nor-maps-gb800solo.patch \
       		add-dmx-source-timecode.patch \
    		af9015-output-full-range-SNR.patch \
    		af9033-output-full-range-SNR.patch \
    		as102-adjust-signal-strength-report.patch \
    		as102-scale-MER-to-full-range.patch \
    		cinergy_s2_usb_r2.patch \
    		cxd2820r-output-full-range-SNR.patch \
    		dvb-usb-dib0700-disable-sleep.patch \
    		dvb_usb_disable_rc_polling.patch \
    		it913x-switch-off-PID-filter-by-default.patch \
    		tda18271-advertise-supported-delsys.patch \
    		fix-dvb-siano-sms-order.patch \
    		mxl5007t-add-no_probe-and-no_reset-parameters.patch \
    		nfs-max-rwsize-8k.patch \
    		0001-rt2800usb-add-support-for-rt55xx.patch \
    		linux-sata_bcm.patch \
    		brcmnand.patch \
    		fix_fuse_for_linux_mips_3-9.patch \
    		rt2800usb_fix_warn_tx_status_timeout_to_dbg.patch \
		linux-3.9-gcc-4.9.3-build-error-fixed.patch \
    		rtl8712-fix-warnings.patch \
    		rtl8187se-fix-warnings.patch \
    		kernel-add-support-for-gcc-5.patch \
    		kernel-add-support-for-gcc6.patch \
    		kernel-add-support-for-gcc7.patch \
    		kernel-add-support-for-gcc8.patch \
    		kernel-add-support-for-gcc9.patch \
    		kernel-add-support-for-gcc10.patch \
		kernel-add-support-for-gcc11.patch \
		kernel-add-support-for-gcc12.patch \
		build-with-gcc12-fixes.patch \
		0001-Support-TBS-USB-drivers-3.9.patch \
    		0001-STV-Add-PLS-support.patch \
    		0001-STV-Add-SNR-Signal-report-parameters.patch \
    		0001-stv090x-optimized-TS-sync-control.patch \
    		blindscan2.patch \
    		genksyms_fix_typeof_handling.patch \
    		0002-cp1emu-do-not-use-bools-for-arithmetic.patch \
    		0003-log2-give-up-on-gcc-constant-optimizations.patch \
    		add-attributes-fix-modules-compile.patch \
    		makefile-silence-warnings.patch \
    		move-default-dialect-to-SMB2.patch

$(ARCHIVE)/$(KERNEL_SRC):
	$(DOWNLOAD) $(KERNEL_URL)/$(KERNEL_SRC)

$(D)/kernel.do_prepare: $(ARCHIVE)/$(KERNEL_SRC) $(BASE_DIR)/machine/$(BOXTYPE)/files/$(KERNEL_CONFIG)
	$(START_BUILD)
	rm -rf $(KERNEL_DIR)
	$(UNTARGZ)/$(KERNEL_SRC)
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
		$(DEPMOD) -ae -b $(TARGET_DIR) -F $(KERNEL_DIR)/System.map -r $(KERNEL_VER)
	@touch $@

$(D)/kernel: $(D)/bootstrap $(D)/kernel.do_compile
	install -m 644 $(KERNEL_DIR)/vmlinux $(TARGET_DIR)/boot/
	install -m 644 $(KERNEL_DIR)/System.map $(TARGET_DIR)/boot/System.map-$(BOXARCH)-$(KERNEL_VER)
	gzip -f -c < $(TARGET_DIR)/boot/vmlinux > $(TARGET_DIR)/boot/$(KERNEL_FILE)
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/build || true
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/source || true
	$(TOUCH)

#
# driver
#
DRIVER_VER = 3.9.6
DRIVER_DATE = 20170803
DRIVER_SRC = gigablue-drivers-$(DRIVER_VER)-BCM7325-$(DRIVER_DATE).zip
DRIVER_URL = http://source.mynonpublic.com/gigablue/drivers

$(ARCHIVE)/$(DRIVER_SRC):
	$(DOWNLOAD) $(DRIVER_URL)/$(DRIVER_SRC)

driver: $(D)/driver
$(D)/driver: $(ARCHIVE)/$(DRIVER_SRC) $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	install -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	unzip -o $(ARCHIVE)/$(DRIVER_SRC) -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	$(DEPMOD) -ae -b $(TARGET_DIR) -r $(KERNEL_VER)
	$(TOUCH)

#
# release
#
release-gb800se:
	cp -pa $(TARGET_DIR)/lib/modules/$(KERNEL_VER) $(RELEASE_DIR)/lib/modules
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	cp -f $(BASE_DIR)/machine/$(BOXTYPE)/files/fstab $(RELEASE_DIR)/etc/

#
# image
#
FLASHIMAGE_PREFIX = gigablue/se

FLASHSIZE = 64
ROOTFS_FILE = rootfs.bin
IMAGE_FSTYPES ?= ubi
IMAGE_NAME = rootfs
UBI_VOLNAME = rootfs
MKUBIFS_ARGS = -m 2048 -e 126976 -c 4096
UBINIZE_ARGS = -m 2048 -p 128KiB
BOOTLOGO_FILENAME = splash.bin
BOOT_UPDATE_TEXT = "rename this file to 'force' to force an update without confirmation"
BOOT_UPDATE_FILE = noforce

image-gb800se:
	$(MAKE) ubi-image-$(BOXTYPE)

