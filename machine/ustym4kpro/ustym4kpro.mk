#
# Makefile for uclan ustym 4k pro
#
BOXARCH = arm
CICAM = ci-cam
LCD = 4-digits

#
# kernel
#
KERNEL_VER             = 4.4.35
KERNEL_DATE            = 20181224
KERNEL_SRC             = uclan-linux-$(KERNEL_VER)-$(KERNEL_DATE).tar.gz
KERNEL_URL             = http://source.mynonpublic.com/uclan
KERNEL_CONFIG          = defconfig
KERNEL_DIR             = $(BUILD_TMP)/linux-$(KERNEL_VER)
KERNEL_DTB_VER         = hi3798mv200.dtb

KERNEL_PATCHES = \
		0002-log2-give-up-on-gcc-constant-optimizations.patch \
		0003-dont-mark-register-as-const.patch \
		0001-remote.patch \
		HauppaugeWinTV-dualHD.patch \
		dib7000-linux_4.4.179.patch \
		dvb-usb-linux_4.4.179.patch \
		wifi-linux_4.4.183.patch \
		move-default-dialect-to-SMB3.patch \
		fix-dvbcore.patch \
		0005-xbox-one-tuner-4.4.patch \
		0006-dvb-media-tda18250-support-for-new-silicon-tuner.patch \
		0007-dvb-mn88472-staging.patch \
		mn88472_reset_stream_ID_reg_if_no_PLP_given.patch \
		af9035.patch \
		4.4.35_fix-multiple-defs-yyloc.patch
		
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
	cp $(BASE_DIR)/machine/$(BOXTYPE)/patches/initramfs-subdirboot.cpio.gz $(KERNEL_DIR)
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
	install -m 644 $(KERNEL_DIR)/System.map $(TARGET_DIR)/boot/System.map-$(BOXARCH)-$(KERNEL_VER)
	cp $(KERNEL_DIR)/arch/arm/boot/uImage $(TARGET_DIR)/boot/
	cat $(KERNEL_DIR)/arch/arm/boot/uImage $(KERNEL_DIR)/arch/arm/boot/dts/$(KERNEL_DTB_VER) > $(TARGET_DIR)/boot/uImage.dtb
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/build || true
	rm $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/source || true
	$(TOUCH)

#
# driver
#
DRIVER_VER     = $(KERNEL_VER)
DRIVER_DATE    = 20230804
HILIB_DATE     = 20190603
LIBGLES_DATE   = 20180301
LIBREADER_DATE = 20221220
HIHALT_DATE    = 20220326
TNTFS_DATE     = 20200528

HICHIPSET = 3798mv200
SOC_FAMILY = hisi3798mv200

DRIVER_SRC = $(BOXTYPE)-hiko-$(DRIVER_DATE).zip

HILIB_SRC = $(BOXTYPE)-hilib-$(HILIB_DATE).tar.gz

LIBGLES_SRC = $(BOXTYPE)-opengl-$(LIBGLES_DATE).tar.gz

LIBREADER_SRC = $(BOXTYPE)-libreader-$(LIBREADER_DATE).zip

HIHALT_SRC = $(BOXTYPE)-hihalt-$(HIHALT_DATE).tar.gz

TNTFS_SRC = $(HICHIPSET)-tntfs-$(TNTFS_DATE).zip

LIBJPEG_SRC = libjpeg.so.62.2.0

WIFI_DIR = RTL8192EU-master
WIFI_SRC = master.zip
WIFI = RTL8192EU.zip

WIFI2_DIR = RTL8822C-main
WIFI2_SRC = main.zip
WIFI2 = RTL8822C.zip

$(ARCHIVE)/$(DRIVER_SRC):
	$(DOWNLOAD) http://source.mynonpublic.com/uclan/$(DRIVER_SRC)

$(ARCHIVE)/$(HILIB_SRC):
	$(DOWNLOAD) http://source.mynonpublic.com/uclan/$(HILIB_SRC)

$(ARCHIVE)/$(LIBGLES_SRC):
	$(DOWNLOAD) http://source.mynonpublic.com/uclan/$(LIBGLES_SRC)

$(ARCHIVE)/$(LIBREADER_SRC):
	$(DOWNLOAD) http://source.mynonpublic.com/uclan/$(LIBREADER_SRC)

$(ARCHIVE)/$(HIHALT_SRC):
	$(DOWNLOAD) http://source.mynonpublic.com/uclan/$(HIHALT_SRC)

$(ARCHIVE)/$(TNTFS_SRC):
	$(DOWNLOAD) http://source.mynonpublic.com/tntfs/$(TNTFS_SRC)

$(ARCHIVE)/$(LIBJPEG_SRC):	
	$(DOWNLOAD) https://github.com/oe-alliance/oe-alliance-core/raw/5.3/meta-brands/meta-uclan/recipes-graphics/files/$(LIBJPEG_SRC)

$(ARCHIVE)/$(WIFI_SRC):
	$(DOWNLOAD) https://github.com/zukon/RTL8192EU/archive/refs/heads/$(WIFI_SRC) -O $(ARCHIVE)/$(WIFI)

$(ARCHIVE)/$(WIFI2_SRC):
	$(DOWNLOAD) https://github.com/zukon/RTL8822C/archive/refs/heads/$(WIFI2_SRC) -O $(ARCHIVE)/$(WIFI2)

driver: $(D)/driver
$(D)/driver: $(ARCHIVE)/$(DRIVER_SRC) $(D)/bootstrap $(D)/kernel
	$(START_BUILD)
	install -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	unzip -o $(ARCHIVE)/$(DRIVER_SRC) -d $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	install -d $(TARGET_DIR)/bin
	mv $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra/hiko/* $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	rmdir $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra/hiko
	$(MAKE) install-tntfs
	$(MAKE) install-wifi
	$(MAKE) install-wifi2
	ls $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra | sed s/.ko//g > $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/modules.default
	$(MAKE) install-hisiplayer-libs
	$(MAKE) install-hilib
	$(MAKE) install-libjpeg
	$(MAKE) install-hihalt
	$(MAKE) install-libreader
	$(DEPMOD) -ae -b $(TARGET_DIR) -r $(KERNEL_VER)
	$(TOUCH)

$(D)/install-hilib: $(ARCHIVE)/$(HILIB_SRC)
	install -d $(BUILD_TMP)/hilib
	tar xzf $(ARCHIVE)/$(HILIB_SRC) -C $(BUILD_TMP)/hilib
	cp -R $(BUILD_TMP)/hilib/hilib/* $(TARGET_LIB_DIR)
	$(REMOVE)/hilib

$(D)/install-hisiplayer-libs: $(ARCHIVE)/$(LIBGLES_SRC)
	install -d $(BUILD_TMP)/hiplay
	tar xzf $(ARCHIVE)/$(LIBGLES_SRC) -C $(BUILD_TMP)/hiplay
	cp -d $(BUILD_TMP)/hiplay/usr/lib/* $(TARGET_LIB_DIR)
	$(REMOVE)/hiplay

$(D)/install-libreader: $(ARCHIVE)/$(LIBREADER_SRC)
	install -d $(BUILD_TMP)/libreader
	unzip -o $(ARCHIVE)/$(LIBREADER_SRC) -d $(BUILD_TMP)/libreader
	install -m 0755 $(BUILD_TMP)/libreader/libreader $(TARGET_DIR)/usr/bin/libreader
	$(REMOVE)/libreader

$(D)/install-tntfs: $(ARCHIVE)/$(TNTFS_SRC)
	install -d $(BUILD_TMP)/tntfs
	unzip -o $(ARCHIVE)/$(TNTFS_SRC) -d $(BUILD_TMP)/tntfs
	install -m 0755 $(BUILD_TMP)/tntfs/tntfs.ko $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	$(REMOVE)/tntfs

$(D)/install-hihalt: $(ARCHIVE)/$(HIHALT_SRC)
	install -d $(BUILD_TMP)/hihalt
	tar xzf $(ARCHIVE)/$(HIHALT_SRC) -C $(BUILD_TMP)/hihalt
	install -m 0755 $(BUILD_TMP)/hihalt/hihalt $(TARGET_DIR)/usr/bin/hihalt
	$(REMOVE)/hihalt

$(D)/install-libjpeg: $(ARCHIVE)/$(LIBJPEG_SRC)
	cp $(ARCHIVE)/$(LIBJPEG_SRC) $(TARGET_LIB_DIR)

$(D)/install-wifi: $(D)/bootstrap $(D)/kernel $(ARCHIVE)/$(WIFI_SRC)
	$(START_BUILD)
	$(REMOVE)/$(WIFI_DIR)
	unzip -o $(ARCHIVE)/$(WIFI) -d $(BUILD_TMP)
	echo $(KERNEL_DIR)
	$(CHDIR)/$(WIFI_DIR); \
		$(MAKE) ARCH=arm CROSS_COMPILE=$(TARGET)- KVER=$(DRIVER_VER) KSRC=$(KERNEL_DIR); \
		install -m 644 8192eu.ko $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	$(REMOVE)/$(WIFI_DIR)
	$(TOUCH)

$(D)/install-wifi2: $(D)/bootstrap $(D)/kernel $(ARCHIVE)/$(WIFI2_SRC)
	$(START_BUILD)
	$(REMOVE)/$(WIFI2_DIR)
	unzip -o $(ARCHIVE)/$(WIFI2) -d $(BUILD_TMP)
	echo $(KERNEL_DIR)
	$(CHDIR)/$(WIFI2_DIR); \
		$(MAKE) ARCH=arm CROSS_COMPILE=$(TARGET)- KVER=$(DRIVER_VER) KSRC=$(KERNEL_DIR); \
		install -m 644 88x2cu.ko $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra
	$(REMOVE)/$(WIFI2_DIR)
	$(TOUCH)
	
FLASH_PARTITONS_SRC = $(BOXTYPE)-partitions-20220326.zip

$(ARCHIVE)/$(FLASH_PARTITONS_SRC):
	$(DOWNLOAD) http://source.mynonpublic.com/uclan/$(FLASH_PARTITONS_SRC)

#
# release
#
release-ustym4kpro: $(ARCHIVE)/$(FLASH_PARTITONS_SRC)
	cp -pa $(TARGET_DIR)/lib/modules/$(KERNEL_VER) $(RELEASE_DIR)/lib/modules
	install -m 0755 $(SKEL_ROOT)/etc/init.d/mmcblk-by-name $(RELEASE_DIR)/etc/init.d/mmcblk-by-name
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/halt $(RELEASE_DIR)/etc/init.d/
	cp -f $(BASE_DIR)/machine/$(BOXTYPE)/files/fstab $(RELEASE_DIR)/etc/
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/showiframe $(RELEASE_DIR)/bin
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/libreader.sh  $(RELEASE_DIR)/usr/bin/libreader.sh
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/root  $(RELEASE_DIR)/var/spool/cron/crontabs/root
	touch $(RELEASE_DIR)/var/tuxbox/config/.crond
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/suspend  $(RELEASE_DIR)/etc/init.d/suspend
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/libreader $(RELEASE_DIR)/etc/init.d/
	cd $(RELEASE_DIR)/etc/rc.d/rc0.d; ln -sf ../../init.d/libreader ./S05libreader
	cd $(RELEASE_DIR)/etc/rc.d/rc6.d; ln -sf ../../init.d/libreader ./S05libreader
	
#
# image
#

image-ustym4kpro:
	$(MAKE) octagon-disk-image-$(BOXTYPE) octagon-rootfs-image-$(BOXTYPE)

