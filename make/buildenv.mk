# set up environment for other makefiles

CONFIG_SITE =
export CONFIG_SITE

LD_LIBRARY_PATH =
export LD_LIBRARY_PATH

#
# BS version
#
export BS_NAME	     := NeutrinoNG
export BS_CYCLE	     := 204

#
BASE_DIR             := $(shell pwd)

#
ARCHIVE               = $(HOME)/Archive

#
TOOLS_DIR             = $(BASE_DIR)/tools
DRIVER_DIR            = $(BASE_DIR)/driver
PATCHES               = $(BASE_DIR)/patches
SCRIPTS_DIR           = $(BASE_DIR)/scripts
SKEL_ROOT             = $(BASE_DIR)/root

# BOXTYPE
-include $(BASE_DIR)/.config

#
TUFSBOX_DIR           = $(BASE_DIR)/tufsbox/$(BOXTYPE)

#
BUILD_TMP             = $(TUFSBOX_DIR)/build_tmp
SOURCE_DIR            = $(TUFSBOX_DIR)/build_source
TARGET_DIR            = $(TUFSBOX_DIR)/cdkroot
CROSS_DIR             = $(TUFSBOX_DIR)/cross
HOST_DIR              = $(TUFSBOX_DIR)/host
RELEASE_DIR           = $(TUFSBOX_DIR)/release
IMAGE_DIR             = $(TUFSBOX_DIR)/image
PKGS_DIR	      = $(TUFSBOX_DIR)/pkgs
D                     = $(TUFSBOX_DIR)/.deps

#
PKGPREFIX	      = $(BUILD_TMP)/data
IMAGE_BUILD_DIR       = $(BUILD_TMP)/image-build

# 
-include $(BASE_DIR)/machine/$(BOXTYPE)/$(BOXTYPE).mk

# for local extensions
-include $(BASE_DIR)/config.local

# default platform...
MAKEFLAGS            += --no-print-directory
GIT_PROTOCOL         ?= http
ifneq ($(GIT_PROTOCOL), http)
GITHUB               ?= git://github.com
else
GITHUB               ?= https://github.com
endif
GIT_NAME             ?= mohousch

# backwards compatibility
DEPDIR                = $(D)

#SUDOCMD               = echo $(SUDOPASSWD) | sudo -S
SUDOCMD			= fakeroot

MAINTAINER           ?= $(shell whoami)
MAIN_ID               = $(shell echo -en "\x74\x68\x6f\x6d\x61\x73")

CCACHE                = /usr/bin/ccache
CCACHE_DIR            = $(HOME)/.ccache-bs-$(BOXARCH)
export CCACHE_DIR

BUILD                ?= $(shell /usr/share/libtool/config.guess 2>/dev/null || /usr/share/libtool/config/config.guess 2>/dev/null || /usr/share/misc/config.guess 2>/dev/null)

ifeq ($(BOXARCH), sh4)
TARGET               ?= sh4-linux
TARGET_MARCH_CFLAGS   =
endif

ifeq ($(BOXARCH), arm)
TARGET               ?= arm-cortex-linux-gnueabihf
TARGET_MARCH_CFLAGS   = -march=armv7ve -mtune=cortex-a15 -mfpu=neon-vfpv4 -mfloat-abi=hard
endif

ifeq ($(BOXARCH), mips)
TARGET		     ?= mipsel-unknown-linux-gnu
TARGET_MARCH_CFLAGS   = -march=mips32 -mtune=mips32
endif

OPTIMIZATIONS        ?= size
ifeq ($(OPTIMIZATIONS), size)
TARGET_O_CFLAGS       = -Os
TARGET_EXTRA_CFLAGS   = -ffunction-sections -fdata-sections
TARGET_EXTRA_LDFLAGS  = -Wl,--gc-sections
endif
ifeq ($(OPTIMIZATIONS), normal)
TARGET_O_CFLAGS       = -O2
TARGET_EXTRA_CFLAGS   =
TARGET_EXTRA_LDFLAGS  =
endif
ifeq ($(OPTIMIZATIONS), kerneldebug)
TARGET_O_CFLAGS       = -O2
TARGET_EXTRA_CFLAGS   =
TARGET_EXTRA_LDFLAGS  =
endif
ifeq ($(OPTIMIZATIONS), debug)
TARGET_O_CFLAGS       = -O0 -g
TARGET_EXTRA_CFLAGS   =
TARGET_EXTRA_LDFLAGS  =
endif

TARGET_LIB_DIR        = $(TARGET_DIR)/usr/lib
TARGET_INCLUDE_DIR    = $(TARGET_DIR)/usr/include

TARGET_CFLAGS         = -pipe $(TARGET_O_CFLAGS) $(TARGET_MARCH_CFLAGS) $(TARGET_EXTRA_CFLAGS) -I$(TARGET_INCLUDE_DIR)
TARGET_CPPFLAGS       = $(TARGET_CFLAGS)
TARGET_CXXFLAGS       = $(TARGET_CFLAGS)
TARGET_LDFLAGS        = -Wl,-rpath -Wl,/usr/lib -Wl,-rpath-link -Wl,$(TARGET_LIB_DIR) -L$(TARGET_LIB_DIR) -L$(TARGET_DIR)/lib $(TARGET_EXTRA_LDFLAGS)
LD_FLAGS              = $(TARGET_LDFLAGS)

PKG_CONFIG            = $(HOST_DIR)/bin/$(TARGET)-pkg-config
PKG_CONFIG_PATH       = $(TARGET_LIB_DIR)/pkgconfig

VPATH                 = $(D)

PATH                 := $(HOST_DIR)/bin:$(HOST_DIR)/usr/local/bin:$(CROSS_DIR)/bin:$(PATH):/sbin:/usr/sbin:/usr/local/sbin

TERM_RED             := \033[00;31m
TERM_RED_BOLD        := \033[01;31m
TERM_GREEN           := \033[00;32m
TERM_GREEN_BOLD      := \033[01;32m
TERM_YELLOW          := \033[00;33m
TERM_YELLOW_BOLD     := \033[01;33m
TERM_NORMAL          := \033[0m

# certificates
CA_BUNDLE             = ca-certificates.crt
CA_BUNDLE_DIR         = /etc/ssl/certs

# helper-"functions"
REWRITE_LIBTOOL       = sed -i "s,^libdir=.*,libdir='$(TARGET_DIR)/usr/lib'," $(TARGET_DIR)/usr/lib
REWRITE_LIBTOOLDEP    = sed -i -e "s,\(^dependency_libs='\| \|-L\|^dependency_libs='\)/usr/lib,\ $(TARGET_DIR)/usr/lib,g" $(TARGET_DIR)/usr/lib
REWRITE_PKGCONF       = sed -i "s,^prefix=.*,prefix='$(TARGET_DIR)/usr',"

# unpack tarballs, clean up
UNTAR                 = tar -C $(BUILD_TMP) -xf $(ARCHIVE)
UNTARGZ               = tar -C $(BUILD_TMP) -xzf $(ARCHIVE)
REMOVE                = rm -rf $(BUILD_TMP)

#
CHDIR                 = set -e; cd $(BUILD_TMP)
MKDIR                 = mkdir -p $(BUILD_TMP)
STRIP                 = $(TARGET)-strip

#
INSTALL               = install
INSTALL_CONF          = $(INSTALL) -m 0600
INSTALL_DATA          = $(INSTALL) -m 0644
INSTALL_EXEC          = $(INSTALL) -m 0755

#
GET-GIT-ARCHIVE       = $(SCRIPTS_DIR)/get-git-archive.sh
GET-GIT-SOURCE        = $(SCRIPTS_DIR)/get-git-source.sh

#
split_deps_dir=$(subst ., ,$(1))
DEPS_DIR              = $(subst $(D)/,,$@)
PKG_NAME              = $(word 1,$(call split_deps_dir,$(DEPS_DIR)))
PKG_NAME_HELPER       = $(shell echo $(PKG_NAME) | sed 's/.*/\U&/')
PKG_VER_HELPER        = A$($(PKG_NAME_HELPER)_VER)A
PKG_VER               = $($(PKG_NAME_HELPER)_VER)

START_BUILD           = echo; \
			echo -e "Start build of $(TERM_GREEN_BOLD)$(PKG_NAME) $(PKG_VER)$(TERM_NORMAL)"; \
                        echo
                        
END_BUILD             = echo; \
			echo -e "Build of $(TERM_GREEN_BOLD)$(PKG_NAME) $(PKG_VER)$(TERM_NORMAL) completed"; \
                        echo

TOUCH                 = @touch $@; \
			echo -e "Build of $(TERM_GREEN_BOLD)$(PKG_NAME) $(PKG_VER)$(TERM_NORMAL) completed"; \
                        echo

#
PATCH                 = patch -p1 -i $(PATCHES)
APATCH                = patch -p1 -i
define apply_patches
    for i in $(1); do \
        if [ -d $$i ]; then \
            for p in $$i/*; do \
                if [ $${p:0:1} == "/" ]; then \
                    echo -e "==> $(TERM_RED)Applying Patch:$(TERM_NORMAL) $$p"; $(APATCH) $$p; \
                else \
                    echo -e "==> $(TERM_RED)Applying Patch:$(TERM_NORMAL) $$p"; $(PATCH)/$$p; \
                fi; \
            done; \
        else \
            if [ $${i:0:1} == "/" ]; then \
                echo -e "==> $(TERM_RED)Applying Patch:$(TERM_NORMAL) $$i"; $(APATCH) $$i; \
            else \
                echo -e "==> $(TERM_RED)Applying Patch:$(TERM_NORMAL) $$i"; $(PATCH)/$$i; \
            fi; \
        fi; \
    done; \
    echo -e "Patching $(TERM_GREEN_BOLD)$(PKG_NAME) $(PKG_VER)$(TERM_NORMAL) completed"; \
    echo
endef

# download tarballs into archive directory
DOWNLOAD = wget --no-check-certificate -t6 -T20 -c -P $(ARCHIVE)

#
#
#
CONFIGURE_OPTS = \
	--build=$(BUILD) --host=$(TARGET)

BUILDENV = \
	CC=$(TARGET)-gcc \
	CXX=$(TARGET)-g++ \
	LD=$(TARGET)-ld \
	NM=$(TARGET)-nm \
	AR=$(TARGET)-ar \
	AS=$(TARGET)-as \
	RANLIB=$(TARGET)-ranlib \
	STRIP=$(TARGET)-strip \
	OBJCOPY=$(TARGET)-objcopy \
	OBJDUMP=$(TARGET)-objdump \
	LN_S="ln -s" \
	CFLAGS="$(TARGET_CFLAGS)" \
	CPPFLAGS="$(TARGET_CPPFLAGS)" \
	CXXFLAGS="$(TARGET_CXXFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	PKG_CONFIG_PATH=$(PKG_CONFIG_PATH)

CONFIGURE = \
	test -f ./configure || ./autogen.sh && \
	$(BUILDENV) \
	./configure $(CONFIGURE_OPTS)

CONFIGURE_TOOLS = \
	./autogen.sh && \
	$(BUILDENV) \
	./configure $(CONFIGURE_OPTS)

MAKE_OPTS := \
	CC=$(TARGET)-gcc \
	CXX=$(TARGET)-g++ \
	LD=$(TARGET)-ld \
	NM=$(TARGET)-nm \
	AR=$(TARGET)-ar \
	AS=$(TARGET)-as \
	RANLIB=$(TARGET)-ranlib \
	STRIP=$(TARGET)-strip \
	OBJCOPY=$(TARGET)-objcopy \
	OBJDUMP=$(TARGET)-objdump \
	LN_S="ln -s" \
	ARCH=sh \
	CROSS_COMPILE=$(TARGET)-

#
#
#	
DEPMOD = $(HOST_DIR)/bin/depmod

#
#
#
ifeq ($(BOXARCH), sh4)
ifeq ($(WLAN), wlandriver)
WLANDRIVER = WLANDRIVER=wlandriver
endif
endif

