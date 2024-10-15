#
# Makefile for dreambox dm7080
#
BOXARCH = mips
CICAM = ci-cam
SCART = scart
LCD = lcd
FKEYS = fkeys

#
# kernel
#
KERNEL_VER             = 3.4-4.0
KERNEL_SRC_VER         = 3.4.113
KERNEL_SRC             = linux-${KERNEL_SRC_VER}.tar.xz
KERNEL_URL             = https://cdn.kernel.org/pub/linux/kernel/v3.x
KERNEL_CONFIG          = defconfig
KERNEL_DIR             = $(BUILD_TMP)/linux-$(KERNEL_SRC_VER)

KERNEL_PATCHES = \
		linux-dreambox-3.4-30070c78a23d461935d9db0b6ce03afc70a10c51.patch \
		kernel-fake-3.4.patch \
		dvb_frontend-Multistream-support-3.4.patch \
		kernel-add-support-for-gcc6.patch \
		kernel-add-support-for-gcc7.patch \
		kernel-add-support-for-gcc8.patch \
		kernel-add-support-for-gcc9.patch \
		kernel-add-support-for-gcc10.patch \
		kernel-add-support-for-gcc11.patch \
		kernel-add-support-for-gcc12.patch \
		kernel-add-support-for-gcc13.patch \
		kernel-add-support-for-gcc14.patch \
		build-with-gcc12-fixes.patch \
		genksyms_fix_typeof_handling.patch \
		0001-log2-give-up-on-gcc-constant-optimizations.patch \
		0002-cp1emu-do-not-use-bools-for-arithmetic.patch \
		0003-makefile-silence-packed-not-aligned-warn.patch \
		0004-fcrypt-fix-bitoperation-for-gcc.patch

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
		$(MAKE) -C $(KERNEL_DIR) ARCH=mips CROSS_COMPILE=$(TARGET)- vmlinux.bin modules
		$(MAKE) -C $(KERNEL_DIR) ARCH=mips CROSS_COMPILE=$(TARGET)- DEPMOD=$(DEPMOD) INSTALL_MOD_PATH=$(TARGET_DIR) modules_install
		$(DEPMOD) -ae -b $(TARGET_DIR) -F $(KERNEL_DIR)/System.map -r $(KERNEL_VER)-$(BOXTYPE)
	@touch $@

$(D)/kernel: $(D)/bootstrap $(D)/kernel.do_compile
	install -m 644 $(KERNEL_DIR)/vmlinux $(TARGET_DIR)/boot/
	install -m 644 $(KERNEL_DIR)/System.map $(TARGET_DIR)/boot/System.map-$(BOXARCH)-$(KERNEL_VER)
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/build || true
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/source || true
	$(TOUCH)

#
# driver
#
DRIVER_VER = 3.4-4.0
DRIVER_DATE = 20190502
DRIVER_SRC = dreambox-dvb-modules_$(DRIVER_VER)-$(BOXTYPE)-$(DRIVER_DATE)_$(BOXTYPE).tar.xz

$(ARCHIVE)/$(DRIVER_SRC):
#	$(DOWNLOAD) https://sources.dreamboxupdate.com/download/opendreambox/2.0.0/dreambox-dvb-modules/$(DRIVER_SRC)
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
# release-dm7080
#
release-dm7080:
	cp -pa $(TARGET_DIR)/lib/modules/$(KERNEL_VER)-$(BOXTYPE) $(RELEASE_DIR)/lib/modules
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	cp -f $(BASE_DIR)/machine/$(BOXTYPE)/files/fstab $(RELEASE_DIR)/etc/
	
#
# image
#
image-dm7080:
	$(MAKE) usb-image-$(BOXTYPE)

