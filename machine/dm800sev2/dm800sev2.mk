#
# Makefile for dreambox dm800sev2
#
BOXARCH = mips
CICAM = ci-cam
SCART = scart
LCD = lcd
FKEYS = fkeys

#
#
#
KERNEL_VER             = 3.2
KERNEL_SRC_VER         = 3.2.68
KERNEL_SRC             = linux-${KERNEL_SRC_VER}.tar.xz
KERNEL_URL             = https://cdn.kernel.org/pub/linux/kernel/v3.x
KERNEL_CONFIG          = defconfig
KERNEL_DIR             = $(BUILD_TMP)/linux-$(KERNEL_SRC_VER)
KERNEL_FILE	       = vmlinux-3.2-dm800sev2.gz

KERNEL_PATCHES = \
		kernel-fake-3.2.patch \
		linux-dreambox-3.2-3c7230bc0819495db75407c365f4d1db70008044.patch \
		unionfs-2.6_for_3.2.62.patch \
		0001-correctly-initiate-nand-flash-ecc-config-when-old-2n.patch \
		0001-Revert-MIPS-Fix-potencial-corruption.patch \
		fadvise_dontneed_change.patch \
		fix-proc-cputype.patch \
		rtl8712-backport-b.patch \
		rtl8712-backport-c.patch \
		rtl8712-backport-d.patch \
		0007-CHROMIUM-make-3.82-hack-to-fix-differing-behaviour-b.patch \
		0008-MIPS-Fix-build-with-binutils-2.24.51.patch \
		0009-MIPS-Refactor-clear_page-and-copy_page-functions.patch \
		0010-BRCMSTB-Fix-build-with-binutils-2.24.51.patch \
		0011-staging-rtl8712-rtl8712-avoid-lots-of-build-warnings.patch \
		0001-brmcnand_base-disable-flash-BBT-on-64MB-nand.patch \
		0002-ubifs-add-config-option-to-use-zlib-as-default-compr.patch \
		em28xx_fix_terratec_entries.patch \
		em28xx_add_terratec_h5_rev3.patch \
		dvb-usb-siano-always-load-smsdvb.patch \
		dvb-usb-af9035.patch \
		dvb-usb-a867.patch \
		dvb-usb-rtl2832.patch \
		dvb_usb_disable_rc_polling.patch \
		dvb-usb-smsdvb_fix_frontend.patch \
		0001-it913x-backport-changes-to-3.2-kernel.patch \
		kernel-add-support-for-gcc6.patch \
		kernel-add-support-for-gcc7.patch \
		kernel-add-support-for-gcc8.patch \
		kernel-add-support-for-gcc9.patch \
		kernel-add-support-for-gcc10.patch \
		kernel-add-support-for-gcc11.patch \
		kernel-add-support-for-gcc12.patch \
		misc_latin1_to_utf8_conversions.patch \
		0001-dvb_frontend-backport-multistream-support.patch \
		genksyms_fix_typeof_handling.patch \
		0012-log2-give-up-on-gcc-constant-optimizations.patch \
		0013-cp1emu-do-not-use-bools-for-arithmetic.patch \
		0014-makefile-silence-packed-not-aligned-warn.patch \
		0015-fcrypt-fix-bitoperation-for-gcc.patch \
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
		$(MAKE) -C $(KERNEL_DIR) ARCH=mips oldconfig
		$(MAKE) -C $(KERNEL_DIR) ARCH=mips CROSS_COMPILE=$(TARGET)- vmlinux modules
		$(MAKE) -C $(KERNEL_DIR) ARCH=mips CROSS_COMPILE=$(TARGET)- DEPMOD=$(DEPMOD) INSTALL_MOD_PATH=$(TARGET_DIR) modules_install
		$(DEPMOD) -ae -b $(TARGET_DIR) -F $(KERNEL_DIR)/System.map -r $(KERNEL_VER)-$(BOXTYPE)
	@touch $@

$(D)/kernel: $(D)/bootstrap $(D)/kernel.do_compile
	install -m 644 $(KERNEL_DIR)/vmlinux $(TARGET_DIR)/boot/
	install -m 644 $(KERNEL_DIR)/System.map $(TARGET_DIR)/boot/System.map-$(BOXARCH)-$(KERNEL_VER)-$(BOXTYPE)
	gzip -9c < $(TARGET_DIR)/boot/vmlinux > $(TARGET_DIR)/boot/$(KERNEL_FILE)
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)-$(BOXTYPE)/build || true
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)-$(BOXTYPE)/source || true
	$(TOUCH)
	
#
# driver
#
DRIVER_VER = 3.2
DRIVER_DATE = 20151201
DRIVER_SRC = dreambox-dvb-modules-$(BOXTYPE)-$(DRIVER_VER)-$(BOXTYPE)-$(DRIVER_DATE).tar.bz2

$(ARCHIVE)/$(DRIVER_SRC):
	$(DOWNLOAD) https://github.com/oe-mirrors/dreambox/raw/main/$(DRIVER_SRC)

driver: $(D)/driver	
$(D)/driver: $(ARCHIVE)/$(DRIVER_SRC) $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	install -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)-$(BOXTYPE)/extra
	tar -xf $(ARCHIVE)/$(DRIVER_SRC) -C $(TARGET_DIR)/lib/modules/$(KERNEL_VER)-$(BOXTYPE)/extra
#	tar -xf $(ARCHIVE)/grautec.tar.gz -C $(TARGET_DIR)/
	$(DEPMOD) -ae -b $(TARGET_DIR) -r $(KERNEL_VER)-$(BOXTYPE)
	$(TOUCH)

#
# dm800sev2 second stage loader #89
#
DM800SEV2_2ND_SOURCE = secondstage-dm800sev2-89.bin
#DM800SEV2_2ND_URL = http://sources.dreamboxupdate.com/download/7020/$(DM800SEV2_2ND_SOURCE)
DM800SEV2_2ND_URL = https://github.com/oe-mirrors/dreambox/raw/main/$(DM800SEV2_2ND_SOURCE)
2ND_FILE = secondstage-dm800sev2-89.bin

$(ARCHIVE)/$(DM800SEV2_2ND_SOURCE):
	$(DOWNLOAD) $(DM800SEV2_2ND_URL)

$(D)/dm800sev2_2nd: $(ARCHIVE)/$(DM800SEV2_2ND_SOURCE)
	$(START_BUILD)
	$(TOUCH)
	
#
# release-dm800sev2
#
release-dm800sev2: $(D)/dm800sev2_2nd
	cp -pa $(TARGET_DIR)/lib/modules/$(KERNEL_VER)-$(BOXTYPE) $(RELEASE_DIR)/lib/modules
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	cp -f $(BASE_DIR)/machine/$(BOXTYPE)/files/fstab $(RELEASE_DIR)/etc/
	
#
# flashimage
#
#FLASHSIZE = 100
ROOTFS_FILE = rootfs.ubi
IMAGE_FSTYPES ?= ubifs
IMAGE_NAME = rootfs
UBI_VOLNAME = rootfs
MKUBIFS_ARGS = -m 4096 -e 248KiB -c 1640 -x favor_lzo -F
UBINIZE_ARGS = -m 4096 -p 256KiB -s 4096
ERASE_BLOCK_SIZE = 0x40000
SECTOR_SIZE = 4096
BUILDIMAGE_EXTRA = -B
FLASH_SIZE = 0x4000000
LOADER_SIZE = 0x100000
BOOT_SIZE = 0x700000
ROOT_SIZE = 0x3F800000

image-dm800sev2:
	$(MAKE) dm-nfi-image-$(BOXTYPE)

