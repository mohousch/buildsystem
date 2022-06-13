BOXARCH = sh4
OPTIMIZATIONS ?= size
WLAN ?= 
LUA ?=lua
PYTHON ?=
FLAVOUR ?= neutrino
MEDIAFW ?= buildinplayer
CICAM ?= ci-cam
SCART ?=
LCD ?= vfd
FKEYS ?=
OSCAM_FLAVOUR ?= oscam

#
# kernel
#
KERNEL_STM ?= p0217

ifeq ($(KERNEL_STM), p0209)
KERNEL_VER             = 2.6.32.46_stm24_0209
KERNEL_REVISION        = 8c676f1a85935a94de1fb103c0de1dd25ff69014
STM_KERNEL_HEADERS_VER = 2.6.32.46-47
P0209                  = p0209
endif

ifeq ($(KERNEL_STM), p0217)
KERNEL_VER             = 2.6.32.71_stm24_0217
KERNEL_REVISION        = 3ec500f4212f9e4b4d2537c8be5ea32ebf68c43b
STM_KERNEL_HEADERS_VER = 2.6.32.46-48
P0217                  = p0217
endif

split_version=$(subst _, ,$(1))
KERNEL_UPSTREAM    =$(word 1,$(call split_version,$(KERNEL_VER)))
KERNEL_STM        :=$(word 2,$(call split_version,$(KERNEL_VER)))
KERNEL_LABEL      :=$(word 3,$(call split_version,$(KERNEL_VER)))
KERNEL_RELEASE    :=$(subst ^0,,^$(KERNEL_LABEL))
KERNEL_STM_LABEL  :=_$(KERNEL_STM)_$(KERNEL_LABEL)
KERNEL_DIR         =$(BUILD_TMP)/linux-sh4-$(KERNEL_VER)
KERNELNAME         = uImage

#
# Patches Kernel 24
#
COMMON_PATCHES_24 = \
		linux-sh4-makefile_stm24.patch \
		linux-stm-gpio-fix-build-CONFIG_BUG.patch \
		linux-kbuild-generate-modules-builtin_stm24_$(KERNEL_LABEL).patch \
		linux-sh4-linuxdvb_stm24_$(KERNEL_LABEL).patch \
		linux-sh4-sound_stm24_$(KERNEL_LABEL).patch \
		linux-sh4-time_stm24_$(KERNEL_LABEL).patch \
		linux-sh4-init_mm_stm24_$(KERNEL_LABEL).patch \
		linux-sh4-copro_stm24_$(KERNEL_LABEL).patch \
		linux-sh4-strcpy_stm24_$(KERNEL_LABEL).patch \
		linux-sh4-ext23_as_ext4_stm24_$(KERNEL_LABEL).patch \
		linux-sh4-bpa2_procfs_stm24_$(KERNEL_LABEL).patch \
		linux-ftdi_sio.c_stm24_$(KERNEL_LABEL).patch \
		linux-sh4-lzma-fix_stm24_$(KERNEL_LABEL).patch \
		linux-tune_stm24.patch \
		linux-net_stm24.patch \
		linux-sh4-permit_gcc_command_line_sections_stm24.patch \
		linux-sh4-mmap_stm24.patch \
		linux-defined_is_deprecated_timeconst.pl_stm24_$(KERNEL_LABEL).patch \
		$(if $(P0217),linux-patch_swap_notify_core_support_stm24_$(KERNEL_LABEL).patch) \
		$(if $(P0209),linux-sh4-dwmac_stm24_$(KERNEL_LABEL).patch)

HL101_PATCHES_24 = $(COMMON_PATCHES_24) \
		linux-sh4-hl101_setup_stm24_$(KERNEL_LABEL).patch \
		linux-usbwait123_stm24.patch \
		linux-sh4-stmmac_stm24_$(KERNEL_LABEL).patch \
		linux-sh4-i2c-st40-pio_stm24_$(KERNEL_LABEL).patch

KERNEL_PATCHES_24  = $(HL101_PATCHES_24)

KERNEL_PATCHES = $(KERNEL_PATCHES_24)
KERNEL_CONFIG = linux-sh4-$(subst _stm24_,_,$(KERNEL_VER))_$(BOXTYPE).config

$(D)/kernel.do_prepare: $(PATCHES)/$(BOXARCH)/$(KERNEL_CONFIG) \
	$(if $(KERNEL_PATCHES),$(KERNEL_PATCHES:%=$(PATCHES)/$(BOXARCH)/stm24/%))
	$(START_BUILD)
	rm -rf $(KERNEL_DIR)
	REPO=https://github.com/Duckbox-Developers/linux-sh4-2.6.32.71.git;protocol=https;branch=stmicro; \
	[ -d "$(ARCHIVE)/linux-sh4-2.6.32.71.git" ] && \
	(echo "Updating STlinux kernel source"; cd $(ARCHIVE)/linux-sh4-2.6.32.71.git; git pull;); \
	[ -d "$(ARCHIVE)/linux-sh4-2.6.32.71.git" ] || \
	(echo "Getting STlinux kernel source"; git clone -n $$REPO $(ARCHIVE)/linux-sh4-2.6.32.71.git); \
	(echo "Copying kernel source code to build environment"; cp -ra $(ARCHIVE)/linux-sh4-2.6.32.71.git $(KERNEL_DIR)); \
	(echo "Applying patch level P$(KERNEL_LABEL)"; cd $(KERNEL_DIR); git checkout -q $(KERNEL_REVISION))
	set -e; cd $(KERNEL_DIR); \
		for i in $(KERNEL_PATCHES); do \
			echo -e "==> $(TERM_RED)Applying Patch:$(TERM_NORMAL) $$i"; \
			$(PATCH)/$(BOXARCH)/stm24/$$i; \
		done
	install -m 644 $(PATCHES)/$(BOXARCH)/$(KERNEL_CONFIG) $(KERNEL_DIR)/.config
	sed -i "s#^\(CONFIG_EXTRA_FIRMWARE_DIR=\).*#\1\"$(BASE_DIR)/root/lib/integrated_firmware\"#" $(KERNEL_DIR)/.config
	-rm $(KERNEL_DIR)/localversion*
	echo "$(KERNEL_STM_LABEL)" > $(KERNEL_DIR)/localversion-stm
ifeq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug))
	@echo "Configuring kernel for debug."
	@grep -v "CONFIG_PRINTK" "$(KERNEL_DIR)/.config" > $(KERNEL_DIR)/.config.tmp
	cp $(KERNEL_DIR)/.config.tmp $(KERNEL_DIR)/.config
	@echo "CONFIG_PRINTK=y" >> $(KERNEL_DIR)/.config
	@echo "CONFIG_PRINTK_TIME=y" >> $(KERNEL_DIR)/.config
endif
ifeq ($(WLAN), $(filter $(WLAN), wlandriver))
	@echo "Using kernel wireless"
	@grep -v "CONFIG_WIRELESS" "$(KERNEL_DIR)/.config" > $(KERNEL_DIR)/.config.tmp
	cp $(KERNEL_DIR)/.config.tmp $(KERNEL_DIR)/.config
	@echo "CONFIG_WIRELESS=y" >> $(KERNEL_DIR)/.config
	@echo "# CONFIG_CFG80211 is not set" >> $(KERNEL_DIR)/.config
	@echo "# CONFIG_WIRELESS_OLD_REGULATORY is not set" >> $(KERNEL_DIR)/.config
	@echo "CONFIG_WIRELESS_EXT=y" >> $(KERNEL_DIR)/.config
	@echo "CONFIG_WIRELESS_EXT_SYSFS=y" >> $(KERNEL_DIR)/.config
	@echo "# CONFIG_LIB80211 is not set" >> $(KERNEL_DIR)/.config
	@echo "CONFIG_WLAN=y" >> $(KERNEL_DIR)/.config
	@echo "# CONFIG_WLAN_PRE80211 is not set" >> $(KERNEL_DIR)/.config
	@echo "CONFIG_WLAN_80211=y" >> $(KERNEL_DIR)/.config
	@echo "# CONFIG_LIBERTAS is not set" >> $(KERNEL_DIR)/.config
	@echo "# CONFIG_USB_ZD1201 is not set" >> $(KERNEL_DIR)/.config
	@echo "# CONFIG_HOSTAP is not set" >> $(KERNEL_DIR)/.config
endif
	@touch $@

$(D)/kernel.do_compile: $(D)/kernel.do_prepare
	set -e; cd $(KERNEL_DIR); \
		$(MAKE) -C $(KERNEL_DIR) ARCH=sh oldconfig
		$(MAKE) -C $(KERNEL_DIR) ARCH=sh include/asm
		$(MAKE) -C $(KERNEL_DIR) ARCH=sh include/linux/version.h
		$(MAKE) -C $(KERNEL_DIR) ARCH=sh CROSS_COMPILE=$(TARGET)- $(KERNELNAME) modules
		$(MAKE) -C $(KERNEL_DIR) ARCH=sh CROSS_COMPILE=$(TARGET)- DEPMOD=$(DEPMOD) INSTALL_MOD_PATH=$(TARGET_DIR) modules_install
		$(DEPMOD) -ae -b $(TARGET_DIR) -F $(KERNEL_DIR)/System.map -r $(KERNEL_VER)
	@touch $@

KERNEL = $(D)/kernel
$(D)/kernel: $(D)/bootstrap host_u_boot_tools $(D)/kernel.do_compile
	install -m 644 $(KERNEL_DIR)/arch/sh/boot/$(KERNELNAME) $(BOOT_DIR)/vmlinux.ub
	install -m 644 $(KERNEL_DIR)/vmlinux $(TARGET_DIR)/boot/vmlinux-sh4-$(KERNEL_VER)
	install -m 644 $(KERNEL_DIR)/System.map $(TARGET_DIR)/boot/System.map-$(BOXARCH)-$(KERNEL_VER)
	cp $(KERNEL_DIR)/arch/sh/boot/$(KERNELNAME) $(TARGET_DIR)/boot/
	$(DEPMOD) -ae -b $(TARGET_DIR) -F $(KERNEL_DIR)/System.map -r $(KERNEL_VER)
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/build || true
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/source || true
	$(TOUCH)

#
# driver
#
DRIVER_PLATFORM   = HL101=hl101
DRIVER_PLATFORM   += $(WLANDRIVER)

#
# driver-symlink
#
driver-symlink:
	cp $(DRIVER_DIR)/stgfb/stmfb/linux/drivers/video/stmfb.h $(TARGET_DIR)/usr/include/linux
	cp $(DRIVER_DIR)/player2/linux/include/linux/dvb/stm_ioctls.h $(TARGET_DIR)/usr/include/linux/dvb
	touch $(D)/$(notdir $@)

#
# driver
#
driver: $(D)/driver
$(D)/driver: $(DRIVER_DIR)/Makefile $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	$(MAKE) -C $(KERNEL_DIR) ARCH=sh CONFIG_DEBUG_SECTION_MISMATCH=y \
		CONFIG_MODULES_PATH=$(CROSS_DIR)/target \
		KERNEL_LOCATION=$(KERNEL_DIR) \
		DRIVER_TOPDIR=$(DRIVER_DIR) \
		M=$(DRIVER_DIR) \
		$(DRIVER_PLATFORM) \
		CROSS_COMPILE=$(TARGET)- \
		modules
	$(MAKE) -C $(KERNEL_DIR) ARCH=sh CONFIG_DEBUG_SECTION_MISMATCH=y \
		CONFIG_MODULES_PATH=$(CROSS_DIR)/target \
		KERNEL_LOCATION=$(KERNEL_DIR) \
		DRIVER_TOPDIR=$(DRIVER_DIR) \
		M=$(DRIVER_DIR) \
		$(DRIVER_PLATFORM) \
		CROSS_COMPILE=$(TARGET)- \
		BIN_DEST=$(TARGET_DIR)/bin \
		INSTALL_MOD_PATH=$(TARGET_DIR) \
		modules_install
	$(DEPMOD) -ae -b $(TARGET_DIR) -F $(KERNEL_DIR)/System.map -r $(KERNEL_VER)
	$(TOUCH)

#
# release
#
release-hl101:
	cp $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra/frontcontroller/proton/proton.ko $(RELEASE_DIR)/lib/modules/
	cp $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra/frontends/*.ko $(RELEASE_DIR)/lib/modules/
	cp $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra/stgfb/stmfb/stmcore-display-stx7109c3.ko $(RELEASE_DIR)/lib/modules/
	cp $(SKEL_ROOT)/boot/video_7109.elf $(RELEASE_DIR)/lib/firmware/video.elf
	cp $(SKEL_ROOT)/boot/audio_7109.elf $(RELEASE_DIR)/lib/firmware/audio.elf
	cp $(SKEL_ROOT)/lib/firmware/dvb-fe-avl2108.fw $(RELEASE_DIR)/lib/firmware/
	cp $(SKEL_ROOT)/lib/firmware/dvb-fe-stv6306.fw $(RELEASE_DIR)/lib/firmware/
	cp $(SKEL_ROOT)/lib/firmware/as102_data1_st.hex $(RELEASE_DIR)/lib/firmware/
	cp $(SKEL_ROOT)/lib/firmware/as102_data2_st.hex $(RELEASE_DIR)/lib/firmware/
	cp -dp $(BASE_DIR)/machine/$(BOXTYPE)/files/lircd.conf $(RELEASE_DIR)/etc/lircd.conf
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/rcS $(RELEASE_DIR)/etc/init.d/rcS

#
# flashimage
#

#
# usbimage
#
usb-image-$(BOXTYPE):
	mkdir -p $(FLASH_DIR)
	cd $(RELEASE_DIR) && \
		tar -cvzf $(FLASH_DIR)/$(BOXTYPE)_$(FLAVOUR)_$(shell date '+%d.%m.%Y-%H.%M')_usb.tgz *
