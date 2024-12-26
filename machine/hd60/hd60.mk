#
# Makefile for ax hd60
#
BOXARCH = arm
CICAM = ci-cam
SCART = scart
LCD = 4-digits
FKEYS =

#
# kernel
#
KERNEL_VER             = 4.4.35
KERNEL_DATE            = 20180301
KERNEL_SRC             = linux-$(KERNEL_VER)-$(KERNEL_DATE)-arm.tar.gz
KERNEL_URL             = http://source.mynonpublic.com/gfutures
KERNEL_CONFIG          = defconfig
KERNEL_DIR             = $(BUILD_TMP)/linux-$(KERNEL_VER)
KERNEL_DTB_VER         = hi3798mv200.dtb
KERNEL_FILE 	       = uImage

KERNEL_PATCHES = \
		0002-log2-give-up-on-gcc-constant-optimizations.patch \
		0003-dont-mark-register-as-const.patch \
		0001-remote.patch \
		HauppaugeWinTV-dualHD.patch \
		dib7000-linux_4.4.179.patch \
		dvb-usb-linux_4.4.179.patch \
		wifi-linux_4.4.183.patch \
		move-default-dialect-to-SMB3.patch \
		0004-linux-fix-buffer-size-warning-error.patch \
		modules_mark__inittest__exittest_as__maybe_unused.patch \
		includelinuxmodule_h_copy__init__exit_attrs_to_initcleanup_module.patch \
		Backport_minimal_compiler_attributes_h_to_support_GCC_9.patch \
		0005-xbox-one-tuner-4.4.patch \
		0006-dvb-media-tda18250-support-for-new-silicon-tuner.patch \
		0007-dvb-mn88472-staging.patch \
		mn88472_reset_stream_ID_reg_if_no_PLP_given.patch \
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
		$(MAKE) -C $(KERNEL_DIR) ARCH=arm CROSS_COMPILE=$(TARGET)- $(KERNEL_DTB_VER) uImage modules
		$(MAKE) -C $(KERNEL_DIR) ARCH=arm CROSS_COMPILE=$(TARGET)- DEPMOD=$(DEPMOD) INSTALL_MOD_PATH=$(TARGET_DIR) modules_install
	@touch $@

$(D)/kernel: $(D)/bootstrap $(D)/kernel.do_compile
	install -m 644 $(KERNEL_DIR)/vmlinux $(TARGET_DIR)/boot/vmlinux-arm-$(KERNEL_VER)
	install -m 644 $(KERNEL_DIR)/System.map $(TARGET_DIR)/boot/System.map-arm-$(KERNEL_VER)
	cp $(KERNEL_DIR)/arch/arm/boot/uImage $(TARGET_DIR)/boot/
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/build || true
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/source || true
	$(TOUCH)

#
# driver
#
DRIVER_VER = 4.4.35
DRIVER_DATE = 20200731
DRIVER_SRC = $(BOXTYPE)-drivers-$(DRIVER_VER)-$(DRIVER_DATE).zip
DRIVER_URL = http://source.mynonpublic.com/gfutures

EXTRA_PLAYERLIB_DATE = 20180912
EXTRA_PLAYERLIB_SRC = $(BOXTYPE)-libs-$(EXTRA_PLAYERLIB_DATE).zip
EXTRA_PLAYERLIB_URL = http://source.mynonpublic.com/gfutures

EXTRA_MALILIB_DATE = 20180912
EXTRA_MALILIB_SRC = $(BOXTYPE)-mali-$(EXTRA_MALILIB_DATE).zip
EXTRA_MALILIB_URL = http://source.mynonpublic.com/gfutures

EXTRA_MALI_MODULE_VER = DX910-SW-99002-r7p0-00rel0
EXTRA_MALI_MODULE_SRC = $(EXTRA_MALI_MODULE_VER).tgz
EXTRA_MALI_MODULE_PATCH = 0001-hi3798mv200-support.patch
EXTRA_MALI_MODULE_URL = https://developer.arm.com/-/media/Files/downloads/mali-drivers/kernel/mali-utgard-gpu

$(ARCHIVE)/$(DRIVER_SRC):
	$(DOWNLOAD) $(DRIVER_URL)/$(DRIVER_SRC)

$(ARCHIVE)/$(EXTRA_PLAYERLIB_SRC):
	$(DOWNLOAD) $(EXTRA_PLAYERLIB_URL)/$(EXTRA_PLAYERLIB_SRC)

$(ARCHIVE)/$(EXTRA_MALILIB_SRC):
	$(DOWNLOAD) $(EXTRA_MALILIB_URL)/$(EXTRA_MALILIB_SRC)

$(ARCHIVE)/$(EXTRA_MALI_MODULE_SRC):
	$(DOWNLOAD) $(EXTRA_MALI_MODULE_URL)/$(EXTRA_MALI_MODULE_SRC);name=driver

driver: $(D)/driver
$(D)/driver: $(ARCHIVE)/$(DRIVER_SRC) $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	install -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	unzip -o $(ARCHIVE)/$(DRIVER_SRC) -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	install -d $(TARGET_DIR)/bin
	mv $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra/turnoff_power $(TARGET_DIR)/bin
	$(MAKE) install-extra-libs
	$(MAKE) mali-gpu-modul
	$(DEPMOD) -ae -b $(TARGET_DIR) -r $(KERNEL_VER)
	$(TOUCH)

#
# extra-libs
#
$(D)/install-extra-libs: $(ARCHIVE)/$(EXTRA_PLAYERLIB_SRC) $(ARCHIVE)/$(EXTRA_MALILIB_SRC) $(D)/zlib $(D)/libpng $(D)/freetype $(D)/libcurl $(D)/libxml2 $(D)/libjpeg_turbo2
	install -d $(TARGET_DIR)/usr/lib
	unzip -o $(PATCHES)/libgles-mali-utgard-headers.zip -d $(TARGET_DIR)/usr/include
	unzip -o $(ARCHIVE)/$(EXTRA_PLAYERLIB_SRC) -d $(TARGET_DIR)/usr/lib
	unzip -o $(ARCHIVE)/$(EXTRA_MALILIB_SRC) -d $(TARGET_DIR)/usr/lib
	ln -sf libMali.so $(TARGET_DIR)/usr/lib/libmali.so
	ln -sf libMali.so $(TARGET_DIR)/usr/lib/libEGL.so
	ln -sf libMali.so $(TARGET_DIR)/usr/lib/libGLESv1_CM.so
	ln -sf libMali.so $(TARGET_DIR)/usr/lib/libGLESv2.so

$(D)/mali-gpu-modul: $(ARCHIVE)/$(EXTRA_MALI_MODULE_SRC) $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	$(REMOVE)/$(EXTRA_MALI_MODULE_VER)
	$(UNTAR)/$(EXTRA_MALI_MODULE_SRC)
	$(CHDIR)/$(EXTRA_MALI_MODULE_VER); \
		$(call apply_patches,$(EXTRA_MALI_MODULE_PATCH)); \
		$(MAKE) -C $(KERNEL_DIR) ARCH=arm CROSS_COMPILE=$(TARGET)- \
		M=$(BUILD_TMP)/$(EXTRA_MALI_MODULE_VER)/driver/src/devicedrv/mali \
		EXTRA_CFLAGS="-DCONFIG_MALI_SHARED_INTERRUPTS=y \
		-DCONFIG_MALI400=m \
		-DCONFIG_MALI450=y \
		-DCONFIG_MALI_DVFS=y \
		-DCONFIG_GPU_AVS_ENABLE=y" \
		CONFIG_MALI_SHARED_INTERRUPTS=y \
		CONFIG_MALI400=m \
		CONFIG_MALI450=y \
		CONFIG_MALI_DVFS=y \
		CONFIG_GPU_AVS_ENABLE=y ; \
		$(MAKE) -C $(KERNEL_DIR) ARCH=arm CROSS_COMPILE=$(TARGET)- \
		M=$(BUILD_TMP)/$(EXTRA_MALI_MODULE_VER)/driver/src/devicedrv/mali \
		EXTRA_CFLAGS="-DCONFIG_MALI_SHARED_INTERRUPTS=y \
		-DCONFIG_MALI400=m \
		-DCONFIG_MALI450=y \
		-DCONFIG_MALI_DVFS=y \
		-DCONFIG_GPU_AVS_ENABLE=y" \
		CONFIG_MALI_SHARED_INTERRUPTS=y \
		CONFIG_MALI400=m \
		CONFIG_MALI450=y \
		CONFIG_MALI_DVFS=y \
		CONFIG_GPU_AVS_ENABLE=y \
		DEPMOD=$(DEPMOD) INSTALL_MOD_PATH=$(TARGET_DIR) modules_install
	$(REMOVE)/$(EXTRA_MALI_MODULE_VER)
	$(TOUCH)

#
# release
#
release-hd60:
	cp -pa $(TARGET_DIR)/lib/modules/$(KERNEL_VER) $(RELEASE_DIR)/lib/modules
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	cp -f $(BASE_DIR)/machine/$(BOXTYPE)/files/fstab $(RELEASE_DIR)/etc/

#
# image
#
FLASH_SRCDATE = 20180912
FLASH_BOOTARGS_SRC = $(BOXTYPE)-bootargs-$(FLASH_SRCDATE).zip
FLASH_PARTITONS_SRC = $(BOXTYPE)-partitions-$(FLASH_SRCDATE).zip

$(ARCHIVE)/$(FLASH_BOOTARGS_SRC):
	$(DOWNLOAD) http://source.mynonpublic.com/gfutures/$(FLASH_BOOTARGS_SRC)

$(ARCHIVE)/$(FLASH_PARTITONS_SRC):
	$(DOWNLOAD) http://source.mynonpublic.com/gfutures/$(FLASH_PARTITONS_SRC)

image-hd60:
	$(MAKE) hdfastboot8gb-disk-image-$(BOXTYPE) hdfastboot8gb-rootfs-image-$(BOXTYPE)

