#
# toolcheck
#
TOOLCHECK  = find-git find-svn find-gzip find-bzip2 find-patch find-gawk
TOOLCHECK += find-makeinfo find-automake find-gcc find-libtool
TOOLCHECK += find-yacc find-flex find-tic find-pkg-config find-help2man
TOOLCHECK += find-cmake find-gperf

find-%:
	@TOOL=$(patsubst find-%,%,$@); \
		type -p $$TOOL >/dev/null || \
		{ echo -e "$(TERM_RED)required tool $$TOOL missing.$(TERM_NORMAL)"; false; }

toolcheck: $(TOOLCHECK)
	@echo "All required tools seem to be installed."
	@echo
ifeq ($(BOXARCH), sh4)
	@for i in audio_7100 audio_7105 audio_7111 video_7100 video_7105 video_7109 video_7111; do \
		if [ ! -e $(SKEL_ROOT)/boot/$$i.elf ]; then \
			echo -e "\n    $(TERM_RED)ERROR:$(TERM_NORMAL) One or more .elf files are missing in $(SKEL_ROOT)/boot!"; \
			echo "           $$i.elf is one of them"; \
			echo; \
			echo "    Correct this and retry."; \
			echo; \
		fi; \
	done
endif
	@if test "$(subst /bin/,,$(shell readlink /bin/sh))" != bash; then \
		echo "WARNING: /bin/sh is not linked to bash."; \
		echo "         This configuration might work, but is not supported."; \
		echo; \
	fi

#
# host_pkgconfig
#
HOST_PKGCONFIG_VER = 0.29.2
HOST_PKGCONFIG_SOURCE = pkg-config-$(HOST_PKGCONFIG_VER).tar.gz

$(ARCHIVE)/$(HOST_PKGCONFIG_SOURCE):
	$(DOWNLOAD) https://pkgconfig.freedesktop.org/releases/$(HOST_PKGCONFIG_SOURCE)

$(D)/host_pkgconfig: $(D)/directories $(ARCHIVE)/$(HOST_PKGCONFIG_SOURCE)
	$(START_BUILD)
	$(REMOVE)/pkg-config-$(HOST_PKGCONFIG_VER)
	$(UNTAR)/$(HOST_PKGCONFIG_SOURCE)
	$(CHDIR)/pkg-config-$(HOST_PKGCONFIG_VER); \
		./configure \
			--prefix=$(HOST_DIR) \
			--program-prefix=$(TARGET)- \
			--disable-host-tool \
			--with-pc_path=$(PKG_CONFIG_PATH) \
			--with-internal-glib \
		; \
		$(MAKE); \
		$(MAKE) install
	ln -sf $(TARGET)-pkg-config $(HOST_DIR)/bin/pkg-config
	$(REMOVE)/pkg-config-$(HOST_PKGCONFIG_VER)
	$(TOUCH)

#
# host_module_init_tools
#
HOST_MODULE_INIT_TOOLS_VER = 3.16
HOST_MODULE_INIT_TOOLS_SOURCE = module-init-tools-$(HOST_MODULE_INIT_TOOLS_VER).tar.bz2
HOST_MODULE_INIT_TOOLS_PATCH = module-init-tools-$(HOST_MODULE_INIT_TOOLS_VER).patch

$(ARCHIVE)/$(HOST_MODULE_INIT_TOOLS_SOURCE):
	$(DOWNLOAD) http://distro.ibiblio.org/fatdog/source/600/m/$(HOST_MODULE_INIT_TOOLS_SOURCE)

$(D)/host_module_init_tools: $(D)/directories $(ARCHIVE)/$(HOST_MODULE_INIT_TOOLS_SOURCE)
	$(START_BUILD)
	$(REMOVE)/module-init-tools-$(HOST_MODULE_INIT_TOOLS_VER)
	$(UNTAR)/$(HOST_MODULE_INIT_TOOLS_SOURCE)
	$(CHDIR)/module-init-tools-$(HOST_MODULE_INIT_TOOLS_VER); \
		$(call apply_patches,$(HOST_MODULE_INIT_TOOLS_PATCH)); \
		autoreconf -fi; \
		./configure \
			--prefix=$(HOST_DIR) \
			--sbindir=$(HOST_DIR)/bin \
		; \
		$(MAKE) all; \
		$(MAKE) install
	$(REMOVE)/module-init-tools-$(HOST_MODULE_INIT_TOOLS_VER)
	$(TOUCH)

#
# host_mtd_utils
#
HOST_MTD_UTILS_VER = 1.5.2
HOST_MTD_UTILS_SOURCE = mtd-utils-$(HOST_MTD_UTILS_VER).tar.bz2
HOST_MTD_UTILS_PATCH = host-mtd-utils-$(HOST_MTD_UTILS_VER).patch
HOST_MTD_UTILS_PATCH += host-mtd-utils-$(HOST_MTD_UTILS_VER)-sysmacros.patch

$(ARCHIVE)/$(HOST_MTD_UTILS_SOURCE):
	$(DOWNLOAD) ftp://ftp.infradead.org/pub/mtd-utils/$(HOST_MTD_UTILS_SOURCE)

$(D)/host_mtd_utils: $(D)/directories $(ARCHIVE)/$(HOST_MTD_UTILS_SOURCE)
	$(START_BUILD)
	$(REMOVE)/mtd-utils-$(HOST_MTD_UTILS_VER)
	$(UNTAR)/$(HOST_MTD_UTILS_SOURCE)
	$(CHDIR)/mtd-utils-$(HOST_MTD_UTILS_VER); \
		$(call apply_patches,$(HOST_MTD_UTILS_PATCH)); \
		$(MAKE) `pwd`/mkfs.jffs2 `pwd`/sumtool BUILDDIR=`pwd` WITHOUT_XATTR=1 DESTDIR=$(HOST_DIR); \
		$(MAKE) install BINDIR=$(HOST_DIR)/bin MANDIR=$(HOST_DIR)/share/man
	$(REMOVE)/mtd-utils-$(HOST_MTD_UTILS_VER)
	$(TOUCH)

#
# host_mkcramfs
#
HOST_MKCRAMFS_VER = 1.1
HOST_MKCRAMFS_SOURCE = cramfs-$(HOST_MKCRAMFS_VER).tar.gz
HOST_MKCRAMFS_PATCH = cramfs-$(HOST_MKCRAMFS_VER)-sysmacros.patch

$(ARCHIVE)/$(HOST_MKCRAMFS_SOURCE):
	$(DOWNLOAD) https://sourceforge.net/projects/cramfs/files/cramfs/$(HOST_MKCRAMFS_VER)/$(HOST_MKCRAMFS_SOURCE)

$(D)/host_mkcramfs: $(D)/directories $(ARCHIVE)/$(HOST_MKCRAMFS_SOURCE)
	$(START_BUILD)
	$(REMOVE)/cramfs-$(HOST_MKCRAMFS_VER)
	$(UNTAR)/$(HOST_MKCRAMFS_SOURCE)
	$(CHDIR)/cramfs-$(HOST_MKCRAMFS_VER); \
		$(call apply_patches,$(HOST_MKCRAMFS_PATCH)); \
		$(MAKE) all
		cp $(BUILD_TMP)/cramfs-$(HOST_MKCRAMFS_VER)/mkcramfs $(HOST_DIR)/bin
		cp $(BUILD_TMP)/cramfs-$(HOST_MKCRAMFS_VER)/cramfsck $(HOST_DIR)/bin
	$(REMOVE)/cramfs-$(HOST_MKCRAMFS_VER)
	$(TOUCH)

#
# host_mksquashfs
#
HOST_MKSQUASHFS_VER = 3.3
HOST_MKSQUASHFS_SOURCE = squashfs$(HOST_MKSQUASHFS_VER).tar.gz

$(ARCHIVE)/$(HOST_MKSQUASHFS_SOURCE):
	$(DOWNLOAD) https://sourceforge.net/projects/squashfs/files/OldFiles/$(HOST_MKSQUASHFS_SOURCE)

$(D)/host_mksquashfs: directories $(ARCHIVE)/$(HOST_MKSQUASHFS_SOURCE)
	$(START_BUILD)
	$(REMOVE)/squashfs$(HOST_MKSQUASHFS_VER)
	$(UNTAR)/$(HOST_MKSQUASHFS_SOURCE)
	$(CHDIR)/squashfs$(HOST_MKSQUASHFS_VER)/squashfs-tools; \
		$(MAKE) CC=gcc all
		mv $(BUILD_TMP)/squashfs$(HOST_MKSQUASHFS_VER)/squashfs-tools/mksquashfs $(HOST_DIR)/bin/mksquashfs3.3
		mv $(BUILD_TMP)/squashfs$(HOST_MKSQUASHFS_VER)/squashfs-tools/unsquashfs $(HOST_DIR)/bin/unsquashfs3.3
	$(REMOVE)/squashfs$(HOST_MKSQUASHFS_VER)
	$(TOUCH)

#
# host_mksquashfs with LZMA support
#
HOST_MKSQUASHFS_LZMA_VER = 4.2
HOST_MKSQUASHFS_LZMA_SOURCE = squashfs$(HOST_MKSQUASHFS_LZMA_VER).tar.gz
HOST_MKSQUASHFS_LZMA_PATCH = squashfs-$(HOST_MKSQUASHFS_LZMA_VER)-sysmacros.patch

LZMA_VER = 4.65
LZMA_SOURCE = lzma-$(LZMA_VER).tar.bz2

$(ARCHIVE)/$(HOST_MKSQUASHFS_LZMA_SOURCE):
	$(DOWNLOAD) https://sourceforge.net/projects/squashfs/files/squashfs/squashfs$(HOST_MKSQUASHFS_LZMA_VER)/$(HOST_MKSQUASHFS_LZMA_SOURCE)

$(ARCHIVE)/$(LZMA_SOURCE):
	$(DOWNLOAD) http://downloads.openwrt.org/sources/$(LZMA_SOURCE)

$(D)/host_mksquashfs_lzma: directories $(ARCHIVE)/$(LZMA_SOURCE) $(ARCHIVE)/$(HOST_MKSQUASHFS_LZMA_SOURCE)
	$(START_BUILD)
	$(REMOVE)/lzma-$(LZMA_VER)
	$(UNTAR)/$(LZMA_SOURCE)
	$(REMOVE)/squashfs$(HOST_MKSQUASHFS_LZMA_VER)
	$(UNTAR)/$(HOST_MKSQUASHFS_LZMA_SOURCE)
	$(CHDIR)/squashfs$(HOST_MKSQUASHFS_LZMA_VER); \
		$(call apply_patches,$(HOST_MKSQUASHFS_LZMA_PATCH)); \
		$(MAKE) -C squashfs-tools EXTRA_CFLAGS=-fgnu89-inline \
			LZMA_SUPPORT=1 \
			LZMA_DIR=$(BUILD_TMP)/lzma-$(LZMA_VER) \
			XATTR_SUPPORT=0 \
			XATTR_DEFAULT=0 \
			install INSTALL_DIR=$(HOST_DIR)/bin
	$(REMOVE)/lzma-$(LZMA_VER)
	$(REMOVE)/squashfs$(HOST_MKSQUASHFS_LZMA_VER)
	$(TOUCH)

#
# host_resize2fs
#
HOST_E2FSPROGS_VER = 1.45.6
HOST_E2FSPROGS_SOURCE = e2fsprogs-$(HOST_E2FSPROGS_VER).tar.gz

$(ARCHIVE)/$(HOST_E2FSPROGS_SOURCE):
	$(DOWNLOAD) https://sourceforge.net/projects/e2fsprogs/files/e2fsprogs/v$(HOST_E2FSPROGS_VER)/$(HOST_E2FSPROGS_SOURCE)

$(D)/host_resize2fs: $(D)/directories $(ARCHIVE)/$(HOST_E2FSPROGS_SOURCE)
	$(START_BUILD)
	$(UNTAR)/$(HOST_E2FSPROGS_SOURCE)
	$(CHDIR)/e2fsprogs-$(HOST_E2FSPROGS_VER); \
		./configure; \
		$(MAKE)
	install -D -m 0755 $(BUILD_TMP)/e2fsprogs-$(HOST_E2FSPROGS_VER)/resize/resize2fs $(HOST_DIR)/bin/
	install -D -m 0755 $(BUILD_TMP)/e2fsprogs-$(HOST_E2FSPROGS_VER)/misc/mke2fs $(HOST_DIR)/bin/
	ln -sf mke2fs $(HOST_DIR)/bin/mkfs.ext2
	ln -sf mke2fs $(HOST_DIR)/bin/mkfs.ext3
	ln -sf mke2fs $(HOST_DIR)/bin/mkfs.ext4
	ln -sf mke2fs $(HOST_DIR)/bin/mkfs.ext4dev
	install -D -m 0755 $(BUILD_TMP)/e2fsprogs-$(HOST_E2FSPROGS_VER)/e2fsck/e2fsck $(HOST_DIR)/bin/
	ln -sf e2fsck $(HOST_DIR)/bin/fsck.ext2
	ln -sf e2fsck $(HOST_DIR)/bin/fsck.ext3
	ln -sf e2fsck $(HOST_DIR)/bin/fsck.ext4
	ln -sf e2fsck $(HOST_DIR)/bin/fsck.ext4dev
	$(REMOVE)/e2fsprogs-$(HOST_E2FSPROGS_VER)
	$(TOUCH)

#
# host_parted
#
HOST_PARTED_VER = 3.2
HOST_PARTED_SOURCE = parted-$(HOST_PARTED_VER).tar.xz
HOST_PARTED_PATCH = parted-$(HOST_PARTED_VER)-device-mapper.patch

$(ARCHIVE)/$(HOST_PARTED_SOURCE):
	$(DOWNLOAD) https://ftp.gnu.org/gnu/parted/$(HOST_PARTED_SOURCE)

$(D)/host_parted: $(D)/directories $(ARCHIVE)/$(HOST_PARTED_SOURCE)
	$(START_BUILD)
	$(REMOVE)/parted-$(HOST_PARTED_VER)
	$(UNTAR)/$(HOST_PARTED_SOURCE)
	$(CHDIR)/parted-$(HOST_PARTED_VER); \
		$(call apply_patches,$(HOST_PARTED_PATCH)); \
		./configure \
			--prefix=$(HOST_DIR) \
			--sbindir=$(HOST_DIR)/bin \
			--disable-device-mapper \
			--without-readline \
		; \
		$(MAKE) install
	$(REMOVE)/parted-$(HOST_PARTED_VER)
	$(TOUCH)
	
#
# cortex-strings
#
CORTEX_STRINGS_VER = 48fd30c
CORTEX_STRINGS_SOURCE = cortex-strings-git-$(CORTEX_STRINGS_VER).tar.bz2
CORTEX_STRINGS_URL = http://git.linaro.org/git-ro/toolchain/cortex-strings.git

$(ARCHIVE)/$(CORTEX_STRINGS_SOURCE):
	$(SCRIPTS_DIR)/get-git-archive.sh $(CORTEX_STRINGS_URL) $(CORTEX_STRINGS_VER) $(notdir $@) $(ARCHIVE)

$(D)/cortex_strings: $(D)/directories $(ARCHIVE)/$(CORTEX_STRINGS_SOURCE)
	$(START_BUILD)
	$(REMOVE)/cortex-strings-git-$(CORTEX_STRINGS_VER)
	$(UNTAR)/$(CORTEX_STRINGS_SOURCE)
	$(CHDIR)/cortex-strings-git-$(CORTEX_STRINGS_VER); \
		./autogen.sh; \
		$(MAKE_OPTS) \
		./configure\
			--build=$(BUILD) \
			--host=$(TARGET) \
			--prefix=/usr \
			--disable-shared \
			--enable-static \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(TARGET_DIR)
	$(REWRITE_LIBTOOL)/libcortex-strings.la
	$(REMOVE)/cortex-strings-git-$(CORTEX_STRINGS_VER)
	$(TOUCH)
	
#
# host dm buildimage
#
BUILDIMAGE_PATCH = buildimage.patch

$(D)/buildimage:
	$(START_BUILD)
	set -e; cd $(TOOLS_DIR)/buildimage.git; \
		autoreconf -fi; \
		./configure; \
		$(MAKE); \
	$(MAKE) install DESTDIR=$(HOST_DIR)
	$(TOUCH)
	
#
# octagon buildimage
#
BUILDIMAGE_SRC = buildimage.zip

$(ARCHIVE)/$(BUILDIMAGE_SRC):
	$(DOWNLOAD) https://github.com/oe-alliance/oe-alliance-core/raw/5.0/meta-brands/meta-octagon/recipes-bsp/octagon-buildimage/$(BUILDIMAGE_SRC)
	
$(D)/buildimage-tool: $(ARCHIVE)/$(BUILDIMAGE_SRC)
	$(START_BUILD)
	$(REMOVE)/buildimage
	unzip -o $(ARCHIVE)/$(BUILDIMAGE_SRC) -d $(BUILD_TMP)/buildimage
	cd $(BUILD_TMP)/buildimage; \
	make; \
	cp -ra $(BUILD_TMP)/buildimage/mkupdate $(HOST_DIR)/bin/mkupdate
	$(REMOVE)/buildimage
	$(TOUCH)

#
# android tools
#
ANDROID_MIRROR = https://android.googlesource.com
HAT_CORE_REV = 2314b11
HAT_CORE_SOURCE = hat-core-git-$(HAT_CORE_REV).tar.bz2
HAT_EXTRAS_REV = 3ecbe8d
HAT_EXTRAS_SOURCE = hat-extras-git-$(HAT_EXTRAS_REV).tar.bz2
HAT_LIBSELINUX_REV = 07e9e13
HAT_LIBSELINUX_SOURCE = hat-libselinux-git-$(HAT_LIBSELINUX_REV).tar.bz2

$(ARCHIVE)/$(HAT_CORE_SOURCE):
	$(SCRIPTS_DIR)/get-git-archive.sh $(ANDROID_MIRROR)/platform/system/core $(HAT_CORE_REV) $(notdir $@) $(ARCHIVE)

$(ARCHIVE)/$(HAT_EXTRAS_SOURCE):
	$(SCRIPTS_DIR)/get-git-archive.sh $(ANDROID_MIRROR)/platform/system/extras $(HAT_EXTRAS_REV) $(notdir $@) $(ARCHIVE)

$(ARCHIVE)/$(HAT_LIBSELINUX_SOURCE):
	$(SCRIPTS_DIR)/get-git-archive.sh $(ANDROID_MIRROR)/platform/external/libselinux $(HAT_LIBSELINUX_REV) $(notdir $@) $(ARCHIVE)

$(D)/host_atools: $(D)/directories $(ARCHIVE)/$(HAT_CORE_SOURCE) $(ARCHIVE)/$(HAT_EXTRAS_SOURCE) $(ARCHIVE)/$(HAT_LIBSELINUX_SOURCE)
	$(START_BUILD)
	$(REMOVE)/hat
	$(MKDIR)/hat/system/core
	tar --strip 1 -C $(BUILD_TMP)/hat/system/core -xf $(ARCHIVE)/$(HAT_CORE_SOURCE)
	$(MKDIR)/hat/system/extras
	tar --strip 1 -C $(BUILD_TMP)/hat/system/extras -xf $(ARCHIVE)/$(HAT_EXTRAS_SOURCE)
	$(MKDIR)/hat/external/libselinux
	tar --strip 1 -C $(BUILD_TMP)/hat/external/libselinux -xf $(ARCHIVE)/$(HAT_LIBSELINUX_SOURCE)
	cp $(PATCHES)/ext4_utils.mk $(BUILD_TMP)/hat
	$(CHDIR)/hat; \
		$(MAKE) --file=ext4_utils.mk SRCDIR=$(BUILD_TMP)/hat
		install -D -m 0755 $(BUILD_TMP)/hat/ext2simg $(HOST_DIR)/bin/
		install -D -m 0755 $(BUILD_TMP)/hat/ext4fixup $(HOST_DIR)/bin/
		install -D -m 0755 $(BUILD_TMP)/hat/img2simg $(HOST_DIR)/bin/
		install -D -m 0755 $(BUILD_TMP)/hat/make_ext4fs $(HOST_DIR)/bin/
		install -D -m 0755 $(BUILD_TMP)/hat/simg2img $(HOST_DIR)/bin/
		install -D -m 0755 $(BUILD_TMP)/hat/simg2simg $(HOST_DIR)/bin/
	$(REMOVE)/hat
	$(TOUCH)

#
# bootstrap
#
BOOTSTRAP  = $(D)/directories
BOOTSTRAP += $(D)/ccache
BOOTSTRAP += $(CROSSTOOL)
BOOTSTRAP += $(TARGET_DIR)/lib/libc.so.6
BOOTSTRAP += $(D)/host_pkgconfig
BOOTSTRAP += $(D)/host_module_init_tools
BOOTSTRAP += $(D)/host_mtd_utils
BOOTSTRAP += $(D)/host_resize2fs
ifeq ($(BOXARCH), sh4)
BOOTSTRAP += $(D)/host_mksquashfs_lzma
BOOTSTRAP += host_u_boot_tools
endif
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), dm8000 dm7020hd dm7020hdv2 dm800se dm800sev2))
BOOTSTRAP += $(D)/buildimage
endif
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), hd60 multiboxse))
BOOTSTRAP += $(D)/host_atools
endif
ifeq ($(BOXTYPE), $(filter $(BOXTYPE), sf8008 ustym4kpro))
BOOTSTRAP += $(D)/buildimage-tool
endif

$(D)/bootstrap: $(BOOTSTRAP)
	@touch $@	

#
# directories
#
ifneq ($(BOXTYPE),)
$(D)/directories:
	$(START_BUILD)
	test -d $(ARCHIVE) || mkdir $(ARCHIVE)
	test -d $(BASE_DIR)/tufsbox || mkdir $(BASE_DIR)/tufsbox
	test -d $(BASE_DIR)/tufsbox/$(BOXTYPE) || mkdir $(BASE_DIR)/tufsbox/$(BOXTYPE)
	test -d $(D) || mkdir $(D)
	test -d $(BUILD_TMP) || mkdir $(BUILD_TMP)
	test -d $(SOURCE_DIR) || mkdir $(SOURCE_DIR)
	install -d $(TARGET_DIR)
	install -d $(CROSS_DIR)
	install -d $(HOST_DIR)
	install -d $(HOST_DIR)/{bin,lib,share}
	install -d $(IMAGE_DIR)
	install -d $(PKGS_DIR)
	install -d $(TARGET_DIR)/{bin,boot,etc,lib,sbin,usr,var}
	install -d $(TARGET_DIR)/etc/{init.d,mdev,network,rc.d,default,samba}
	install -d $(TARGET_DIR)/etc/rc.d/{rc0.d,rc6.d}
	ln -sf ../init.d $(TARGET_DIR)/etc/rc.d/init.d
	install -d $(TARGET_DIR)/lib/{lsb,firmware}
	install -d $(TARGET_DIR)/usr/{bin,lib,sbin,share}
	install -d $(TARGET_DIR)/usr/lib/pkgconfig
	install -d $(TARGET_DIR)/usr/include/linux
	install -d $(TARGET_DIR)/usr/include/linux/dvb
	install -d $(TARGET_DIR)/var/{etc,lib,run}
	install -d $(TARGET_DIR)/var/lib/{misc,nfs,opkg}
	install -d $(TARGET_DIR)/var/bin
	$(TOUCH)
endif	

#
# ccache
#
CCACHE_BINDIR = $(HOST_DIR)/bin
CCACHE_BIN = $(CCACHE)

CCACHE_LINKS = \
	ln -sf $(CCACHE_BIN) $(CCACHE_BINDIR)/cc; \
	ln -sf $(CCACHE_BIN) $(CCACHE_BINDIR)/gcc; \
	ln -sf $(CCACHE_BIN) $(CCACHE_BINDIR)/g++; \
	ln -sf $(CCACHE_BIN) $(CCACHE_BINDIR)/$(TARGET)-gcc; \
	ln -sf $(CCACHE_BIN) $(CCACHE_BINDIR)/$(TARGET)-g++

CCACHE_ENV = install -d $(CCACHE_BINDIR); \
	$(CCACHE_LINKS)

$(D)/ccache: directories
	$(CCACHE_ENV)
	touch $@

# hack to make sure they are always copied
PHONY += ccache crosstool

