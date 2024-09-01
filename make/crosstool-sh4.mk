#
STM_RELOCATE = /opt/STM/STLinux-2.4

# updates / downloads
STL_FTP = $(GITHUB)/Duckbox-Developers/stlinux/raw/master

## ordering is important here. The /host/ rule must stay before the less
## specific %.sh4/%.i386/%.noarch rule. No idea if this is portable or
## even reliable :-(
$(ARCHIVE)/stlinux24-host-%.i386.rpm \
$(ARCHIVE)/stlinux24-host-%noarch.rpm:
	$(DOWNLOAD) $(STL_FTP)/$(subst $(ARCHIVE)/,"",$@)

$(ARCHIVE)/stlinux24-host-%.src.rpm:
	$(DOWNLOAD) $(STL_FTP)/$(subst $(ARCHIVE)/,"",$@)

$(ARCHIVE)/stlinux24-sh4-%.sh4.rpm \
$(ARCHIVE)/stlinux24-cross-%.i386.rpm \
$(ARCHIVE)/stlinux24-sh4-%.noarch.rpm:
	$(DOWNLOAD) $(STL_FTP)/$(subst $(ARCHIVE)/,"",$@)

#
# install the RPMs
#
BINUTILS_VER 		= 2.24.51.0.3-77
GCC_VER      		= 4.8.4-139
LIBGCC_VER   		= 4.8.4-149
GLIBC_VER    		= 2.14.1-59
STM_KERNEL_HEADERS_VER 	= 2.6.32.46-48

crosstool-rpminstall: \
$(ARCHIVE)/stlinux24-cross-sh4-binutils-$(BINUTILS_VER).i386.rpm \
$(ARCHIVE)/stlinux24-cross-sh4-binutils-dev-$(BINUTILS_VER).i386.rpm \
$(ARCHIVE)/stlinux24-cross-sh4-cpp-$(GCC_VER).i386.rpm \
$(ARCHIVE)/stlinux24-cross-sh4-gcc-$(GCC_VER).i386.rpm \
$(ARCHIVE)/stlinux24-cross-sh4-g++-$(GCC_VER).i386.rpm \
$(ARCHIVE)/stlinux24-sh4-linux-kernel-headers-$(STM_KERNEL_HEADERS_VER).noarch.rpm \
$(ARCHIVE)/stlinux24-sh4-glibc-$(GLIBC_VER).sh4.rpm \
$(ARCHIVE)/stlinux24-sh4-glibc-dev-$(GLIBC_VER).sh4.rpm \
$(ARCHIVE)/stlinux24-sh4-libgcc-$(LIBGCC_VER).sh4.rpm \
$(ARCHIVE)/stlinux24-sh4-libstdc++-$(LIBGCC_VER).sh4.rpm \
$(ARCHIVE)/stlinux24-sh4-libstdc++-dev-$(LIBGCC_VER).sh4.rpm
	$(SCRIPTS_DIR)/unpack-rpm.sh $(BUILD_TMP) $(STM_RELOCATE)/devkit/sh4 $(CROSS_DIR) $^
	touch $(D)/$(notdir $@)

CROSSTOOL = crosstool
crosstool: $(D)/directories crosstool-rpminstall
	cp $(DRIVER_DIR)/stgfb/stmfb/linux/drivers/video/stmfb.h $(TARGET_DIR)/usr/include/linux
	cp $(DRIVER_DIR)/player2/linux/include/linux/dvb/stm_ioctls.h $(TARGET_DIR)/usr/include/linux/dvb
	@touch $(D)/$(notdir $@)

$(TARGET_DIR)/lib/libc.so.6:
	set -e; cd $(CROSS_DIR); rm -f sh4-linux/sys-root; ln -s ../target sh4-linux/sys-root; \
	if [ -e $(CROSS_DIR)/target/usr/lib/libstdc++.la ]; then \
		sed -i "s,^libdir=.*,libdir='$(CROSS_DIR)/target/usr/lib'," $(CROSS_DIR)/target/usr/lib/lib{std,sup}c++.la; \
	fi
	if test -e $(CROSS_DIR)/target/usr/lib/libstdc++.so; then \
		cp -a $(CROSS_DIR)/target/usr/lib/libstdc++.s*[!y] $(TARGET_DIR)/lib; \
		cp -a $(CROSS_DIR)/target/usr/lib/libdl.so $(TARGET_DIR)/usr/lib; \
		cp -a $(CROSS_DIR)/target/usr/lib/libm.so $(TARGET_DIR)/usr/lib; \
		cp -a $(CROSS_DIR)/target/usr/lib/librt.so $(TARGET_DIR)/usr/lib; \
		cp -a $(CROSS_DIR)/target/usr/lib/libutil.so $(TARGET_DIR)/usr/lib; \
		cp -a $(CROSS_DIR)/target/usr/lib/libpthread.so $(TARGET_DIR)/usr/lib; \
		cp -a $(CROSS_DIR)/target/usr/lib/libresolv.so $(TARGET_DIR)/usr/lib; \
		ln -sf $(CROSS_DIR)/target/usr/lib/libc.so $(TARGET_DIR)/usr/lib/libc.so; \
		ln -sf $(CROSS_DIR)/target/usr/lib/libc_nonshared.a $(TARGET_DIR)/usr/lib/libc_nonshared.a; \
	fi
	if test -e $(CROSS_DIR)/target/lib; then \
		cp -a $(CROSS_DIR)/target/lib/*so* $(TARGET_DIR)/lib; \
	fi
	if test -e $(CROSS_DIR)/target/sbin/ldconfig; then \
		cp -a $(CROSS_DIR)/target/sbin/ldconfig $(TARGET_DIR)/sbin; \
		cp -a $(CROSS_DIR)/target/etc/ld.so.conf $(TARGET_DIR)/etc; \
		cp -a $(CROSS_DIR)/target/etc/host.conf $(TARGET_DIR)/etc; \
	fi

#
# host_u_boot_tools
#
HOST_U_BOOT_TOOLS_VER = 1.3.1_stm24-9

host_u_boot_tools: $(ARCHIVE)/stlinux24-host-u-boot-tools-$(HOST_U_BOOT_TOOLS_VER).i386.rpm
	$(START_BUILD)
	$(SCRIPTS_DIR)/unpack-rpm.sh $(BUILD_TMP) $(STM_RELOCATE)/host/bin $(HOST_DIR)/bin $^
	@touch $(D)/$(notdir $@)
	$(END_BUILD)

