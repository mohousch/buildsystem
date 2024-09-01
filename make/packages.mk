#
# libupnp-ipk
#
libupnp-ipk: $(D)/bootstrap $(ARCHIVE)/$(LIBUPNP_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/libupnp-$(LIBUPNP_VER)
	$(UNTAR)/$(LIBUPNP_SOURCE)
	$(CHDIR)/libupnp-$(LIBUPNP_VER); \
		$(CONFIGURE) \
			--prefix=/usr \
		; \
		$(MAKE) all; \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	rm -r $(PKGPREFIX)/usr/include $(PKGPREFIX)/usr/lib/pkgconfig
	$(REMOVE)/libupnp-$(LIBUPNP_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/libupnp/control
	touch $(BUILD_TMP)/libupnp/control/control
	echo Package: libupnp > $(BUILD_TMP)/libupnp/control/control
	echo Version: $(LIBUPNP_VER) >> $(BUILD_TMP)/libupnp/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/libupnp/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/libupnp/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/libupnp/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/libupnp/control/control 
	echo Depends:  >> $(BUILD_TMP)/libupnp/control/control
	touch $(BUILD_TMP)/libupnp/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/libupnp/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/libupnp/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/libupnp/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/libupnp/control/preint
	echo 'fi' >> $(BUILD_TMP)/libupnp/control/preint
	pushd $(BUILD_TMP)/libupnp/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/libupnp-$(LIBUPNP_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/libupnp
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# libdvbsi-ipk
#
libdvbsi-ipk: $(D)/bootstrap $(ARCHIVE)/$(LIBDVBSI_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/libdvbsi-git-$(LIBDVBSI_VER)
	$(UNTAR)/$(LIBDVBSI_SOURCE)
	$(CHDIR)/libdvbsi-git-$(LIBDVBSI_VER); \
		$(call apply_patches, $(LIBDVBSI_PATCH)); \
		$(CONFIGURE) \
			--prefix=/usr \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/libdvbsi-git-$(LIBDVBSI_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/libdvbsi/control
	touch $(BUILD_TMP)/libdvbsi/control/control
	echo Package: libdvbsi > $(BUILD_TMP)/libdvbsi/control/control
	echo Version: $(LIBDVBSI_VER) >> $(BUILD_TMP)/libdvbsi/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/libdvbsi/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/libdvbsi/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/libdvbsi/control/control 
endif
	echo Maintainer: $(MAINTAINER) >> $(BUILD_TMP)/libdvbsi/control/control 
	echo Depends:  >> $(BUILD_TMP)/libdvbsi/control/control
	touch $(BUILD_TMP)/libdvbsi/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/libdvbsi/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/libdvbsi/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/libdvbsi/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/libdvbsi/control/preint
	echo 'fi' >> $(BUILD_TMP)/libdvbsi/control/preint
	pushd $(BUILD_TMP)/libdvbsi/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/libdvbsi-$(LIBDVBSI_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/libdvbsi
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# pugixml-ipk
#
pugixml-ipk: $(D)/bootstrap $(ARCHIVE)/$(PUGIXML_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/pugixml-$(PUGIXML_VER)
	$(UNTAR)/$(PUGIXML_SOURCE)
	$(CHDIR)/pugixml-$(PUGIXML_VER); \
		$(call apply_patches, $(PUGIXML_PATCH)); \
		cmake  --no-warn-unused-cli \
			-DCMAKE_INSTALL_PREFIX=/usr \
			-DBUILD_SHARED_LIBS=ON \
			-DCMAKE_BUILD_TYPE=Linux \
			-DCMAKE_C_COMPILER=$(TARGET)-gcc \
			-DCMAKE_CXX_COMPILER=$(TARGET)-g++ \
			-DCMAKE_C_FLAGS="-pipe -Os" \
			-DCMAKE_CXX_FLAGS="-pipe -Os" \
			| tail -n +90 \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/pugixml-$(PUGIXML_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/pugixml/control
	touch $(BUILD_TMP)/pugixml/control/control
	echo Package: pugixml > $(BUILD_TMP)/pugixml/control/control
	echo Version: $(PUGIXML_VER) >> $(BUILD_TMP)/pugixml/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/pugixml/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/pugixml/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/pugixml/control/control 
endif
	echo Maintainer: $(MAINTAINER) >> $(BUILD_TMP)/pugixml/control/control 
	echo Depends:  >> $(BUILD_TMP)/pugixml/control/control
	touch $(BUILD_TMP)/pugixml/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/pugixml/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/pugixml/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/pugixml/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/pugixml/control/preint
	echo 'fi' >> $(BUILD_TMP)/pugixml/control/preint
	pushd $(BUILD_TMP)/pugixml/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/pugixml-$(PUGIXML_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/pugixml
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# libid3tag-ipk
#
libid3tag-ipk: $(D)/bootstrap $(D)/zlib $(ARCHIVE)/$(LIBID3TAG_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/libid3tag-$(LIBID3TAG_VER)
	$(UNTAR)/$(LIBID3TAG_SOURCE)
	$(CHDIR)/libid3tag-$(LIBID3TAG_VER); \
		$(call apply_patches, $(LIBID3TAG_PATCH)); \
		touch NEWS AUTHORS ChangeLog; \
		autoreconf -fi; \
		$(CONFIGURE) \
			--prefix=/usr \
			--enable-shared=yes \
		; \
		$(MAKE) all; \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/libid3tag-$(LIBID3TAG_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/libid3tag/control
	touch $(BUILD_TMP)/libid3tag/control/control
	echo Package: libid3tag > $(BUILD_TMP)/libid3tag/control/control
	echo Version: $(LIBID3TAG_VER) >> $(BUILD_TMP)/libid3tag/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/libid3tag/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/libid3tag/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/libid3tag/control/control 
endif
	echo Maintainer: $(MAINTAINER) >> $(BUILD_TMP)/libid3tag/control/control 
	echo Depends:  >> $(BUILD_TMP)/libid3tag/control/control
	touch $(BUILD_TMP)/libid3tag/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/libid3tag/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/libid3tag/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/libid3tag/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/libid3tag/control/preint
	echo 'fi' >> $(BUILD_TMP)/libid3tag/control/preint
	pushd $(BUILD_TMP)/libid3tag/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/libid3tag-$(LIBID3TAG_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/libid3tag
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# libmad-ipk
#
libmad-ipk: $(D)/bootstrap $(ARCHIVE)/$(LIBMAD_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/libmad-$(LIBMAD_VER)
	$(UNTAR)/$(LIBMAD_SOURCE)
	$(CHDIR)/libmad-$(LIBMAD_VER); \
		$(call apply_patches, $(LIBMAD_PATCH)); \
		touch NEWS AUTHORS ChangeLog; \
		autoreconf -fi; \
		$(CONFIGURE) \
			--prefix=/usr \
			--disable-debugging \
			--enable-shared=yes \
			--enable-speed \
			--enable-sso \
		; \
		$(MAKE) all; \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/libmad-$(LIBMAD_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/libmad/control
	touch $(BUILD_TMP)/libmad/control/control
	echo Package: libmad > $(BUILD_TMP)/libmad/control/control
	echo Version: $(LIBMAD_VER) >> $(BUILD_TMP)/libmad/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/libmad/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/libmad/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/libmad/control/control 
endif
	echo Maintainer: $(MAINTAINER) >> $(BUILD_TMP)/libmad/control/control 
	echo Depends:  >> $(BUILD_TMP)/libmad/control/control
	touch $(BUILD_TMP)/libmad/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/libmad/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/libmad/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/libmad/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/libmad/control/preint
	echo 'fi' >> $(BUILD_TMP)/libmad/control/preint
	pushd $(BUILD_TMP)/libmad/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/libmad-$(LIBMAD_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/libmad
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# libogg-ipk
#
libogg-ipk: $(D)/bootstrap $(ARCHIVE)/$(LIBOGG_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/libogg-$(LIBOGG_VER)
	$(UNTAR)/$(LIBOGG_SOURCE)
	$(CHDIR)/libogg-$(LIBOGG_VER); \
		$(CONFIGURE) \
			--prefix=/usr \
			--docdir=/.remove \
			--enable-shared \
			--disable-static \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/libogg-$(LIBOGG_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/libogg/control
	touch $(BUILD_TMP)/libogg/control/control
	echo Package: libogg > $(BUILD_TMP)/libogg/control/control
	echo Version: $(LIBOGG_VER) >> $(BUILD_TMP)/libogg/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/libogg/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/libogg/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/libogg/control/control 
endif
	echo Maintainer: $(MAINTAINER) >> $(BUILD_TMP)/libogg/control/control 
	echo Depends:  >> $(BUILD_TMP)/libogg/control/control
	touch $(BUILD_TMP)/libogg/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/libogg/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/libogg/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/libogg/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/libogg/control/preint
	echo 'fi' >> $(BUILD_TMP)/libogg/control/preint
	pushd $(BUILD_TMP)/libogg/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/libogg-$(LIBOGG_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/libogg
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# libflac-ipk
#
libflac-ipk: $(D)/bootstrap $(ARCHIVE)/$(FLAC_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/flac-$(FLAC_VER)
	$(UNTAR)/$(FLAC_SOURCE)
	$(CHDIR)/flac-$(FLAC_VER); \
		$(call apply_patches, $(FLAC_PATCH)); \
		touch NEWS AUTHORS ChangeLog; \
		autoreconf -fi; \
		$(CONFIGURE) \
			--prefix=/usr \
			--mandir=/.remove \
			--datarootdir=/.remove \
			--disable-cpplibs \
			--disable-debug \
			--disable-asm-optimizations \
			--disable-sse \
			--disable-altivec \
			--disable-doxygen-docs \
			--disable-thorough-tests \
			--disable-exhaustive-tests \
			--disable-valgrind-testing \
			--disable-ogg \
			--disable-oggtest \
			--disable-local-xmms-plugin \
			--disable-xmms-plugin \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX) docdir=/.remove
	$(REMOVE)/flac-$(FLAC_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/libflac/control
	touch $(BUILD_TMP)/libflac/control/control
	echo Package: libflac > $(BUILD_TMP)/libflac/control/control
	echo Version: $(LIBFLAC_VER) >> $(BUILD_TMP)/libflac/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/libflac/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/libflac/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/libflac/control/control 
endif
	echo Maintainer: $(MAINTAINER) >> $(BUILD_TMP)/libflac/control/control 
	echo Depends:  >> $(BUILD_TMP)/libflac/control/control
	touch $(BUILD_TMP)/libflac/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/libflac/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/libflac/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/libflac/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/libflac/control/preint
	echo 'fi' >> $(BUILD_TMP)/libflac/control/preint
	pushd $(BUILD_TMP)/libflac/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/libflac-$(LIBFLAC_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/libflac
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# libvorbis-ipk
#
libvorbis-ipk: $(D)/bootstrap $(D)/libogg $(ARCHIVE)/$(LIBVORBIS_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/libvorbis-$(LIBVORBIS_VER)
	$(UNTAR)/$(LIBVORBIS_SOURCE)
	$(CHDIR)/libvorbis-$(LIBVORBIS_VER); \
		$(CONFIGURE) \
			--prefix=/usr \
			--docdir=/.remove \
			--mandir=/.remove \
			--disable-docs \
			--disable-examples \
			--disable-oggtest \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX) docdir=/.remove
	$(REMOVE)/libvorbis-$(LIBVORBIS_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/libvorbis/control
	touch $(BUILD_TMP)/libvorbis/control/control
	echo Package: libvorbis > $(BUILD_TMP)/libvorbis/control/control
	echo Version: $(LIBVORBIS_VER) >> $(BUILD_TMP)/libvorbis/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/libvorbis/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/libvorbis/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/libvorbis/control/control 
endif
	echo Maintainer: $(MAINTAINER) >> $(BUILD_TMP)/libvorbis/control/control 
	echo Depends:  >> $(BUILD_TMP)/libvorbis/control/control
	touch $(BUILD_TMP)/libvorbis/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/libvorbis/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/libvorbis/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/libvorbis/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/libvorbis/control/preint
	echo 'fi' >> $(BUILD_TMP)/libvorbis/control/preint
	pushd $(BUILD_TMP)/libvorbis/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/libvorbis-$(LIBVORBIS_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/libvorbis
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# libsigc-ipk
#
libsigc-ipk: $(D)/bootstrap $(ARCHIVE)/$(LIBSIGC_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/libsigc++-$(LIBSIGC_VER)
	$(UNTAR)/$(LIBSIGC_SOURCE)
	$(CHDIR)/libsigc++-$(LIBSIGC_VER); \
		$(CONFIGURE) \
			--prefix=/usr \
			--enable-shared \
			--disable-documentation \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX); \
		if [ -d $(PKGPREFIX)/usr/include/sigc++-2.0/sigc++ ] ; then \
			ln -sf ./sigc++-2.0/sigc++ $(PKGPREFIX)/usr/include/sigc++; \
		fi;
		mv $(PKGPREFIX)/usr/lib/sigc++-2.0/include/sigc++config.h $(PKGPREFIX)/usr/include; \
		rm -fr $(PKGPREFIX)/usr/lib/sigc++-2.0
	$(REMOVE)/libsigc++-$(LIBSIGC_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/libsigc/control
	touch $(BUILD_TMP)/libsigc/control/control
	echo Package: libsigc > $(BUILD_TMP)/libsigc/control/control
	echo Version: $(LIBSIGC_VER) >> $(BUILD_TMP)/libsigc/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/libsigc/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/libsigc/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/libsigc/control/control 
endif
	echo Maintainer: $(MAINTAINER) >> $(BUILD_TMP)/libsigc/control/control 
	echo Depends:  >> $(BUILD_TMP)/libsigc/control/control
	touch $(BUILD_TMP)/libsigc/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/libsigc/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/libsigc/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/libsigc/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/libsigc/control/preint
	echo 'fi' >> $(BUILD_TMP)/libsigc/control/preint
	pushd $(BUILD_TMP)/libsigc/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/libsigc-$(LIBSIGC_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/libsigc
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# libvorbisidec-ipk
#
libvorbisidec-ipk: $(D)/bootstrap $(D)/libogg $(ARCHIVE)/$(LIBVORBISIDEC_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/libvorbisidec-$(LIBVORBISIDEC_VER)
	$(UNTAR)/$(LIBVORBISIDEC_SOURCE)
	$(CHDIR)/libvorbisidec-$(LIBVORBISIDEC_VER); \
		$(call apply_patches, $(LIBVORBISIDEC_PATCH)); \
		ACLOCAL_FLAGS="-I . -I $(TARGET_DIR)/usr/share/aclocal" \
		$(BUILDENV) \
		./autogen.sh \
			--host=$(TARGET) \
			--build=$(BUILD) \
			--prefix=/usr \
		; \
		$(MAKE) all; \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/libvorbisidec-$(LIBVORBISIDEC_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/libvorbisidec/control
	touch $(BUILD_TMP)/libvorbisidec/control/control
	echo Package: libvorbisidec > $(BUILD_TMP)/libvorbisidec/control/control
	echo Version: $(LIBVORBISIDEC_VER) >> $(BUILD_TMP)/libvorbisidec/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/libvorbisidec/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/libvorbisidec/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/libvorbisidec/control/control 
endif
	echo Maintainer: $(MAINTAINER) >> $(BUILD_TMP)/libvorbisidec/control/control 
	echo Depends:  >> $(BUILD_TMP)/libvorbisidec/control/control
	touch $(BUILD_TMP)/libvorbisidec/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/libvorbisidec/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/libvorbisidec/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/libvorbisidec/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/libvorbisidec/control/preint
	echo 'fi' >> $(BUILD_TMP)/libvorbisidec/control/preint
	pushd $(BUILD_TMP)/libvorbisidec/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/libvorbisidec-$(LIBVORBISIDEC_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/libvorbisidec
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# libass-ipk
#
libass-ipk: $(D)/bootstrap $(D)/freetype $(D)/libfribidi $(ARCHIVE)/$(LIBASS_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/libass-$(LIBASS_VER)
	$(UNTAR)/$(LIBASS_SOURCE)
	$(CHDIR)/libass-$(LIBASS_VER); \
		$(call apply_patches, $(LIBASS_PATCH)); \
		$(CONFIGURE) \
			--prefix=/usr \
			--disable-static \
			--disable-test \
			--disable-fontconfig \
			--disable-harfbuzz \
			--disable-require-system-font-provider \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/libass-$(LIBASS_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/libass/control
	touch $(BUILD_TMP)/libass/control/control
	echo Package: libass > $(BUILD_TMP)/libass/control/control
	echo Version: $(LIBASS_VER) >> $(BUILD_TMP)/libass/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/libass/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/libass/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/libass/control/control 
endif
	echo Maintainer: $(MAINTAINER) >> $(BUILD_TMP)/libass/control/control 
	echo Depends:  >> $(BUILD_TMP)/libass/control/control
	touch $(BUILD_TMP)/libass/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/libass/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/libass/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/libass/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/libass/control/preint
	echo 'fi' >> $(BUILD_TMP)/libass/control/preint
	pushd $(BUILD_TMP)/libass/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/libass-$(LIBASS_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/libass
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)

#
# minidlna-ipk
#	
minidlna-ipk: $(D)/bootstrap $(D)/zlib $(D)/sqlite $(D)/libexif $(D)/libjpeg $(D)/libid3tag $(D)/libogg $(D)/libvorbis $(D)/flac $(D)/ffmpeg $(ARCHIVE)/$(MINIDLNA_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/minidlna-$(MINIDLNA_VER)
	$(UNTAR)/$(MINIDLNA_SOURCE)
	$(CHDIR)/minidlna-$(MINIDLNA_VER); \
		$(call apply_patches, $(MINIDLNA_PATCH)); \
		autoreconf -fi; \
		$(CONFIGURE) \
			--prefix=/usr \
		; \
		$(MAKE); \
		$(MAKE) install prefix=/usr DESTDIR=$(PKGPREFIX)
	$(REMOVE)/minidlna-$(MINIDLNA_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/minidlna/control
	touch $(BUILD_TMP)/minidlna/control/control
	echo Package: minidlna > $(BUILD_TMP)/minidlna/control/control
	echo Version: $(MINIDLNA_VER) >> $(BUILD_TMP)/minidlna/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/minidlna/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/minidlna/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/minidlna/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/minidlna/control/control 
	echo Depends:  >> $(BUILD_TMP)/minidlna/control/control
	touch $(BUILD_TMP)/minidlna/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/minidlna/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/minidlna/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/minidlna/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/minidlna/control/preint
	echo 'fi' >> $(BUILD_TMP)/minidlna/control/preint
	pushd $(BUILD_TMP)/minidlna/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/minidlna-$(MINIDLNA_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/minidlna
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)

#
# fbshot-ipk
#
fbshot-ipk: $(D)/bootstrap $(D)/libpng $(ARCHIVE)/$(FBSHOT_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/fbshot-$(FBSHOT_VER)
	$(UNTAR)/$(FBSHOT_SOURCE)
	$(CHDIR)/fbshot-$(FBSHOT_VER); \
		$(call apply_patches, $(FBSHOT_PATCH)); \
		sed -i s~'gcc'~"$(TARGET)-gcc $(TARGET_CFLAGS) $(TARGET_LDFLAGS)"~ Makefile; \
		sed -i 's/strip fbshot/$(TARGET)-strip fbshot/' Makefile; \
		$(MAKE) all; \
		install -D -m 755 fbshot $(PKGPREFIX)/bin/fbshot
	$(REMOVE)/fbshot-$(FBSHOT_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/fbshot/control
	touch $(BUILD_TMP)/fbshot/control/control
	echo Package: fbshot > $(BUILD_TMP)/fbshot/control/control
	echo Version: $(FBSHOT_VER) >> $(BUILD_TMP)/fbshot/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/fbshot/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/fbshot/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/fbshot/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/fbshot/control/control 
	echo Depends:  >> $(BUILD_TMP)/fbshot/control/control
	touch $(BUILD_TMP)/fbshot/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/fbshot/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/fbshot/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/fbshot/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/fbshot/control/preint
	echo 'fi' >> $(BUILD_TMP)/fbshot/control/preint
	pushd $(BUILD_TMP)/fbshot/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/fbshot-$(FBSHOT_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(PKGPREFIX)
	rm -rf $(BUILD_TMP)/fbshot
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)

#
# samba-ipk
#	
samba-ipk: $(D)/bootstrap $(ARCHIVE)/$(SAMBA_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGPREFIX)/etc/init.d
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/samba-$(SAMBA_VER)
	$(UNTAR)/$(SAMBA_SOURCE)
	$(CHDIR)/samba-$(SAMBA_VER); \
		$(call apply_patches, $(SAMBA_PATCH)); \
		cd source3; \
		./autogen.sh; \
		$(BUILDENV) \
		ac_cv_lib_attr_getxattr=no \
		ac_cv_search_getxattr=no \
		ac_cv_file__proc_sys_kernel_core_pattern=yes \
		libreplace_cv_HAVE_C99_VSNPRINTF=yes \
		libreplace_cv_HAVE_GETADDRINFO=yes \
		libreplace_cv_HAVE_IFACE_IFCONF=yes \
		LINUX_LFS_SUPPORT=no \
		samba_cv_CC_NEGATIVE_ENUM_VALUES=yes \
		samba_cv_HAVE_GETTIMEOFDAY_TZ=yes \
		samba_cv_HAVE_IFACE_IFCONF=yes \
		samba_cv_HAVE_KERNEL_OPLOCKS_LINUX=yes \
		samba_cv_HAVE_SECURE_MKSTEMP=yes \
		samba_cv_HAVE_WRFILE_KEYTAB=no \
		samba_cv_USE_SETREUID=yes \
		samba_cv_USE_SETRESUID=yes \
		samba_cv_have_setreuid=yes \
		samba_cv_have_setresuid=yes \
		ac_cv_header_zlib_h=no \
		samba_cv_zlib_1_2_3=no \
		ac_cv_path_PYTHON="" \
		ac_cv_path_PYTHON_CONFIG="" \
		libreplace_cv_HAVE_GETADDRINFO=no \
		libreplace_cv_READDIR_NEEDED=no \
		./configure \
			--build=$(BUILD) \
			--host=$(TARGET) \
			--prefix= \
			--includedir=/usr/include \
			--exec-prefix=/usr \
			--disable-pie \
			--disable-avahi \
			--disable-cups \
			--disable-relro \
			--disable-swat \
			--disable-shared-libs \
			--disable-socket-wrapper \
			--disable-nss-wrapper \
			--disable-smbtorture4 \
			--disable-fam \
			--disable-iprint \
			--disable-dnssd \
			--disable-pthreadpool \
			--disable-dmalloc \
			--with-included-iniparser \
			--with-included-popt \
			--with-sendfile-support \
			--without-aio-support \
			--without-cluster-support \
			--without-ads \
			--without-krb5 \
			--without-dnsupdate \
			--without-automount \
			--without-ldap \
			--without-pam \
			--without-pam_smbpass \
			--without-winbind \
			--without-wbclient \
			--without-syslog \
			--without-nisplus-home \
			--without-quotas \
			--without-sys-quotas \
			--without-utmp \
			--without-acl-support \
			--with-configdir=/etc/samba \
			--with-privatedir=/etc/samba \
			--with-mandir=no \
			--with-piddir=/var/run \
			--with-logfilebase=/var/log \
			--with-lockdir=/var/lock \
			--with-swatdir=/usr/share/swat \
			--disable-cups \
			--without-winbind \
			--without-libtdb \
			--without-libtalloc \
			--without-libnetapi \
			--without-libsmbclient \
			--without-libsmbsharemodes \
			--without-libtevent \
			--without-libaddns \
		; \
		$(MAKE) $(MAKE_OPTS); \
		$(MAKE) $(MAKE_OPTS) installservers installbin installscripts installdat installmodules \
			SBIN_PROGS="bin/samba_multicall" DESTDIR=$(PKGPREFIX) prefix=./. ; \
			ln -s samba_multicall $(PKGPREFIX)/usr/sbin/nmbd
			ln -s samba_multicall $(PKGPREFIX)/usr/sbin/smbd
			ln -s samba_multicall $(PKGPREFIX)/usr/sbin/smbpasswd
	install -m 755 $(SKEL_ROOT)/etc/init.d/samba $(PKGPREFIX)/etc/init.d/
	install -m 644 $(SKEL_ROOT)/etc/samba/smb.conf $(PKGPREFIX)/etc/samba/
	$(REMOVE)/samba-$(SAMBA_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/samba/control
	touch $(BUILD_TMP)/samba/control/control
	echo Package: samba > $(BUILD_TMP)/samba/control/control
	echo Version: $(SAMBA_VER) >> $(BUILD_TMP)/samba/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/samba/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/samba/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/samba/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/samba/control/control 
	echo Depends:  >> $(BUILD_TMP)/samba/control/control
	touch $(BUILD_TMP)/samba/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/samba/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/samba/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/samba/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/samba/control/preint
	echo 'fi' >> $(BUILD_TMP)/samba/control/preint
	pushd $(BUILD_TMP)/samba/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/samba-$(SAMBA_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/samba
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# ofgwrite-ipk
#
ofgwrite-ipk: $(D)/bootstrap $(ARCHIVE)/$(OFGWRITE_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGPREFIX)/usr/bin
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/ofgwrite-ddt
	set -e; if [ -d $(ARCHIVE)/ofgwrite-ddt.git ]; \
		then cd $(ARCHIVE)/ofgwrite-ddt.git; git pull; \
		else cd $(ARCHIVE); git clone https://github.com/Duckbox-Developers/ofgwrite-ddt.git ofgwrite-ddt.git; \
		fi
	cp -ra $(ARCHIVE)/ofgwrite-ddt.git $(BUILD_TMP)/ofgwrite-ddt
	$(CHDIR)/ofgwrite-ddt; \
		$(call apply_patches,$(OFGWRITE_PATCH)); \
		$(BUILDENV) \
		$(MAKE); \
	install -m 755 $(BUILD_TMP)/ofgwrite-ddt/ofgwrite_bin $(PKGPREFIX)/usr/bin
	install -m 755 $(BUILD_TMP)/ofgwrite-ddt/ofgwrite_caller $(PKGPREFIX)/usr/bin
	install -m 755 $(BUILD_TMP)/ofgwrite-ddt/ofgwrite $(PKGPREFIX)/usr/bin
	$(REMOVE)/ofgwrite-ddt
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/ofgwrite/control
	touch $(BUILD_TMP)/ofgwrite/control/control
	echo Package: ofgwrite > $(BUILD_TMP)/ofgwrite/control/control
	echo Version: $(OFGWRITE_VER) >> $(BUILD_TMP)/ofgwrite/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/ofgwrite/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/ofgwrite/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/ofgwrite/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/ofgwrite/control/control 
	echo Depends:  >> $(BUILD_TMP)/ofgwrite/control/control
	touch $(BUILD_TMP)/ofgwrite/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/ofgwrite/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/ofgwrite/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/ofgwrite/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/ofgwrite/control/preint
	echo 'fi' >> $(BUILD_TMP)/ofgwrite/control/preint
	pushd $(BUILD_TMP)/ofgwrite/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/ofgwrite-$(OFGWRITE_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/ofgwrite
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)

#
# xupnpd-ipk
#	
xupnpd-ipk: $(D)/bootstrap $(D)/openssl
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGPREFIX)/etc/init.d
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/xupnpd
	set -e; if [ -d $(ARCHIVE)/xupnpd.git ]; \
		then cd $(ARCHIVE)/xupnpd.git; git pull; \
		else cd $(ARCHIVE); git clone https://github.com/clark15b/xupnpd.git xupnpd.git; \
		fi
	cp -ra $(ARCHIVE)/xupnpd.git $(BUILD_TMP)/xupnpd
	($(CHDIR)/xupnpd; git checkout -q $(XUPNPD_BRANCH);)
	$(CHDIR)/xupnpd; \
		$(call apply_patches, $(XUPNPD_PATCH))
	$(CHDIR)/xupnpd/src; \
		$(BUILDENV) \
		$(MAKE) embedded TARGET=$(TARGET) PKG_CONFIG=$(PKG_CONFIG) LUAFLAGS="$(TARGET_LDFLAGS) -I$(TARGET_INCLUDE_DIR)"; \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	install -m 755 $(SKEL_ROOT)/etc/init.d/xupnpd $(PKGPREFIX)/etc/init.d/
	mkdir -p $(PKGPREFIX)/usr/share/xupnpd/config
	$(REMOVE)/xupnpd
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/xupnpd/control
	touch $(BUILD_TMP)/xupnpd/control/control
	echo Package: xupnpd > $(BUILD_TMP)/xupnpd/control/control
	echo Version: $(XUPNPD_VER) >> $(BUILD_TMP)/xupnpd/control/control
	echo Section: base/application >> $(BUILD_TMP)/xupnpd/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/xupnpd/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/xupnpd/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/xupnpd/control/control 
	echo Depends:  >> $(BUILD_TMP)/xupnpd/control/control
	touch $(BUILD_TMP)/xupnpd/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/xupnpd/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/xupnpd/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/xupnpd/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/xupnpd/control/preint
	echo 'fi' >> $(BUILD_TMP)/xupnpd/control/preint
	pushd $(BUILD_TMP)/xupnpd/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/xupnpd-$(XUPNPD_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/xupnpd
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)

#
# graphlcd-ipk
#
graphlcd-ipk: $(D)/bootstrap $(D)/freetype $(D)/libusb $(ARCHIVE)/$(GRAPHLCD_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGPREFIX)/etc
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/graphlcd-git-$(GRAPHLCD_VER)
	$(UNTAR)/$(GRAPHLCD_SOURCE)
	$(CHDIR)/graphlcd-git-$(GRAPHLCD_VER); \
		$(call apply_patches, $(GRAPHLCD_PATCH)); \
		$(MAKE) -C glcdgraphics all TARGET=$(TARGET)- DESTDIR=$(PKGPREFIX); \
		$(MAKE) -C glcddrivers all TARGET=$(TARGET)- DESTDIR=$(PKGPREFIX); \
		$(MAKE) -C glcdgraphics install DESTDIR=$(PKGPREFIX); \
		$(MAKE) -C glcddrivers install DESTDIR=$(PKGPREFIX); \
		cp -a graphlcd.conf $(PKGPREFIX)/etc
		rm -r $(PKGPREFIX)/usr/include
	$(REMOVE)/graphlcd-git-$(GRAPHLCD_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/graphlcd/control
	touch $(BUILD_TMP)/graphlcd/control/control
	echo Package: graphlcd > $(BUILD_TMP)/graphlcd/control/control
	echo Version: $(GRAPHLCD_VER) >> $(BUILD_TMP)/graphlcd/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/graphlcd/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/graphlcd/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/graphlcd/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/graphlcd/control/control 
	echo Depends:  >> $(BUILD_TMP)/graphlcd/control/control
	touch $(BUILD_TMP)/graphlcd/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/graphlcd/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/graphlcd/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/graphlcd/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/graphlcd/control/preint
	echo 'fi' >> $(BUILD_TMP)/graphlcd/control/preint
	pushd $(BUILD_TMP)/graphlcd/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/graphlcd-$(GRAPHLCD_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/graphlcd
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# lcd4linux-ipk
#
lcd4linux-ipk: $(D)/bootstrap $(D)/libusb_compat $(D)/gd $(D)/libusb $(D)/libdpf $(ARCHIVE)/$(LCD4LINUX_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGPREFIX)/etc/init.d
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/lcd4linux-git-$(LCD4LINUX_VER)
	$(UNTAR)/$(LCD4LINUX_SOURCE)
	$(CHDIR)/lcd4linux-git-$(LCD4LINUX_VER); \
		$(call apply_patches, $(LCD4LINUX_PATCH)); \
		$(BUILDENV) ./bootstrap; \
		$(BUILDENV) ./configure $(CONFIGURE_OPTS) \
			--prefix=/usr \
			--with-drivers='DPF,SamsungSPF$(LCD4LINUX_DRV),PNG' \
			--with-plugins='all,!apm,!asterisk,!dbus,!dvb,!gps,!hddtemp,!huawei,!imon,!isdn,!kvv,!mpd,!mpris_dbus,!mysql,!pop3,!ppp,!python,!qnaplog,!raspi,!sample,!seti,!w1retap,!wireless,!xmms' \
			--without-ncurses \
		; \
		$(MAKE) vcs_version all; \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	install -m 755 $(SKEL_ROOT)/etc/init.d/lcd4linux $(PKGPREFIX)/etc/init.d/
	install -D -m 0600 $(SKEL_ROOT)/etc/lcd4linux.conf $(PKGPREFIX)/etc/lcd4linux.conf
	$(REMOVE)/lcd4linux-git-$(LCD4LINUX_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/lcd4linux/control
	touch $(BUILD_TMP)/lcd4linux/control/control
	echo Package: lcd4linux > $(BUILD_TMP)/lcd4linux/control/control
	echo Version: $(LCD4LINUX_VER) >> $(BUILD_TMP)/lcd4linux/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/lcd4linux/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/lcd4linux/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/lcd4linux/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/lcd4linux/control/control 
	echo Depends:  >> $(BUILD_TMP)/lcd4linux/control/control
	touch $(BUILD_TMP)/lcd4linux/control/preint
	echo '#!/bin/sh' > $(BUILD_TMP)/lcd4linux/control/preint
	echo 'if test -x /sbin/ldconfig; then' >> $(BUILD_TMP)/lcd4linux/control/preint
	echo '	echo "updating dynamic linker cache..."' >> $(BUILD_TMP)/lcd4linux/control/preint
	echo '	/sbin/ldconfig' >> $(BUILD_TMP)/lcd4linux/control/preint
	echo 'fi' >> $(BUILD_TMP)/lcd4linux/control/preint
	pushd $(BUILD_TMP)/lcd4linux/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/lcd4linux-$(LCD4LINUX_VER)_$(BOXARCH)_$(BOXTYPE).ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/lcd4linux
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)

#
# gstreamer-ipk
#
gstreamer-ipk: $(D)/bootstrap $(D)/libglib2 $(D)/libxml2 $(D)/glib_networking $(ARCHIVE)/$(GSTREAMER_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/gstreamer-$(GSTREAMER_VER)
	$(UNTAR)/$(GSTREAMER_SOURCE)
	$(CHDIR)/gstreamer-$(GSTREAMER_VER); \
		$(call apply_patches, $(GSTREAMER_PATCH)); \
		./autogen.sh --noconfigure; \
		$(CONFIGURE) \
			--prefix=/usr \
			--libexecdir=/usr/lib \
			--datarootdir=/.remove \
			--enable-silent-rules \
			$(GST_PLUGIN_CONFIG_DEBUG) \
			--disable-tests \
			--disable-valgrind \
			--disable-gst-tracer-hooks \
			--disable-dependency-tracking \
			--disable-examples \
			--disable-check \
			$(GST_MAIN_CONFIG_DEBUG) \
			--disable-benchmarks \
			--disable-gtk-doc-html \
			ac_cv_header_valgrind_valgrind_h=no \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	rm -r $(PKGPREFIX)/usr/include $(PKGPREFIX)/usr/lib/pkgconfig
	$(REMOVE)/gstreamer-$(GSTREAMER_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/gstreamer/control
	touch $(BUILD_TMP)/gstreamer/control/control
	echo Package: gstreamer > $(BUILD_TMP)/gstreamer/control/control
	echo Version: $(GSTREAMER_VER) >> $(BUILD_TMP)/gstreamer/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/gstreamer/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/gstreamer/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/gstreamer/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/gstreamer/control/control 
	echo Depends:  >> $(BUILD_TMP)/gstreamer/control/control
	pushd $(BUILD_TMP)/gstreamer/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/gstreamer-$(GSTREAMER_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/gstreamer
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# gst_plugins_base-ipk
#
gst_plugins_base-ipk: $(D)/bootstrap $(D)/zlib $(D)/libglib2 $(D)/orc $(D)/gstreamer $(D)/alsa_lib $(D)/libogg $(D)/libvorbis $(ARCHIVE)/$(GST_PLUGINS_BASE_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/gst-plugins-base-$(GST_PLUGINS_BASE_VER)
	$(UNTAR)/$(GST_PLUGINS_BASE_SOURCE)
	$(CHDIR)/gst-plugins-base-$(GST_PLUGINS_BASE_VER); \
		$(call apply_patches, $(GST_PLUGINS_BASE_PATCH)); \
		./autogen.sh --noconfigure; \
		$(CONFIGURE) \
			--prefix=/usr \
			--datarootdir=/.remove \
			--enable-silent-rules \
			--disable-valgrind \
			$(GST_PLUGIN_CONFIG_DEBUG) \
			--disable-examples \
			--disable-gtk-doc-html \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	rm -r $(PKGPREFIX)/usr/include $(PKGPREFIX)/usr/lib/pkgconfig
	$(REMOVE)/gst-plugins-base-$(GST_PLUGINS_BASE_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/gst_plugins_base/control
	touch $(BUILD_TMP)/gst_plugins_base/control/control
	echo Package: gst_plugins_base > $(BUILD_TMP)/gst_plugins_base/control/control
	echo Version: $(GST_PLUGINS_BASE_VER) >> $(BUILD_TMP)/gst_plugins_base/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/gst_plugins_base/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/gst_plugins_base/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/gst_plugins_base/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/gst_plugins_base/control/control 
	echo Depends:  >> $(BUILD_TMP)/gst_plugins_base/control/control
	pushd $(BUILD_TMP)/gst_plugins_base/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/gst_plugins_base-$(GST_PLUGINS_BASE_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/gst_plugins_base
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)

#
# gst_plugins_good-ipk
#
gst_plugins_good-ipk: $(D)/bootstrap $(D)/libpng $(D)/libjpeg $(D)/gstreamer $(D)/gst_plugins_base $(D)/flac $(ARCHIVE)/$(GST_PLUGINS_GOOD_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/gst-plugins-good-$(GST_PLUGINS_GOOD_VER)
	$(UNTAR)/$(GST_PLUGINS_GOOD_SOURCE)
	$(CHDIR)/gst-plugins-good-$(GST_PLUGINS_GOOD_VER); \
		$(call apply_patches, $(GST_PLUGINS_GOOD_PATCH)); \
		./autogen.sh --noconfigure; \
		$(CONFIGURE) \
			--build=$(BUILD) \
			--host=$(TARGET) \
			--prefix=/usr \
			--datarootdir=/.remove \
			--enable-silent-rules \
			--disable-valgrind \
			--disable-aalib \
			--disable-aalibtest \
			--disable-cairo \
			--disable-orc \
			--disable-soup \
			$(GST_PLUGIN_CONFIG_DEBUG) \
			--disable-examples \
			--disable-gtk-doc-html \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/gst-plugins-good-$(GST_PLUGINS_GOOD_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/gst_plugins_good/control
	touch $(BUILD_TMP)/gst_plugins_good/control/control
	echo Package: gst_plugins_good > $(BUILD_TMP)/gst_plugins_good/control/control
	echo Version: $(GST_PLUGINS_GOOD_VER) >> $(BUILD_TMP)/gst_plugins_good/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/gst_plugins_good/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/gst_plugins_good/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/gst_plugins_good/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/gst_plugins_good/control/control 
	echo Depends:  >> $(BUILD_TMP)/gst_plugins_good/control/control
	pushd $(BUILD_TMP)/gst_plugins_good/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/gst_plugins_good-$(GST_PLUGINS_GOOD_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/gst_plugins_good
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# gst_plugins_bad-ipk
#
gst_plugins_bad-ipk: $(D)/bootstrap $(D)/libass $(D)/libcurl $(D)/libxml2 $(D)/openssl $(D)/librtmp $(D)/gstreamer $(D)/gst_plugins_base $(ARCHIVE)/$(GST_PLUGINS_BAD_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/gst-plugins-bad-$(GST_PLUGINS_BAD_VER)
	$(UNTAR)/$(GST_PLUGINS_BAD_SOURCE)
	$(CHDIR)/gst-plugins-bad-$(GST_PLUGINS_BAD_VER); \
		$(call apply_patches, $(GST_PLUGINS_BAD_PATCH)); \
		./autogen.sh --noconfigure; \
		$(CONFIGURE) \
			--build=$(BUILD) \
			--host=$(TARGET) \
			--prefix=/usr \
			--datarootdir=/.remove \
			--enable-silent-rules \
			--disable-valgrind \
			$(GST_PLUGIN_CONFIG_DEBUG) \
			--disable-examples \
			--disable-gtk-doc-html \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	rm -r $(PKGPREFIX)/usr/include $(PKGPREFIX)/usr/lib/pkgconfig
	$(REMOVE)/gst-plugins-bad-$(GST_PLUGINS_BAD_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/gst_plugins_bad/control
	touch $(BUILD_TMP)/gst_plugins_bad/control/control
	echo Package: gst_plugins_bad > $(BUILD_TMP)/gst_plugins_bad/control/control
	echo Version: $(GST_PLUGINS_BAD_VER) >> $(BUILD_TMP)/gst_plugins_bad/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/gst_plugins_bad/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/gst_plugins_bad/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/gst_plugins_bad/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/gst_plugins_bad/control/control 
	echo Depends:  >> $(BUILD_TMP)/gst_plugins_bad/control/control
	pushd $(BUILD_TMP)/gst_plugins_bad/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/gst_plugins_bad-$(GST_PLUGINS_BAD_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/gst_plugins_bad
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# gst_plugins_ugly-ipk
#
gst_plugins_ugly-ipk: $(D)/bootstrap $(D)/gstreamer $(D)/gst_plugins_base $(ARCHIVE)/$(GST_PLUGINS_UGLY_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/gst-plugins-ugly-$(GST_PLUGINS_UGLY_VER)
	$(UNTAR)/$(GST_PLUGINS_UGLY_SOURCE)
	$(CHDIR)/gst-plugins-ugly-$(GST_PLUGINS_UGLY_VER); \
		./autogen.sh --noconfigure; \
		$(CONFIGURE) \
			--prefix=/usr \
			--datarootdir=/.remove \
			--enable-silent-rules \
			--disable-valgrind \
			$(GST_PLUGIN_CONFIG_DEBUG) \
			--disable-examples \
			--disable-gtk-doc-html \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/gst-plugins-ugly-$(GST_PLUGINS_UGLY_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/gst_plugins_ugly/control
	touch $(BUILD_TMP)/gst_plugins_ugly/control/control
	echo Package: gst_plugins_ugly > $(BUILD_TMP)/gst_plugins_ugly/control/control
	echo Version: $(GST_PLUGINS_UGLY_VER) >> $(BUILD_TMP)/gst_plugins_ugly/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/gst_plugins_ugly/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/gst_plugins_ugly/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/gst_plugins_ugly/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/gst_plugins_ugly/control/control 
	echo Depends:  >> $(BUILD_TMP)/gst_plugins_ugly/control/control
	pushd $(BUILD_TMP)/gst_plugins_ugly/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/gst_plugins_ugly-$(GST_PLUGINS_UGLY_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/gst_plugins_ugly
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# gst_plugins_subsink-ipk
#
gst_plugins_subsink-ipk: $(D)/bootstrap $(D)/gstreamer $(D)/gst_plugins_base $(D)/gst_plugins_good $(D)/gst_plugins_bad $(D)/gst_plugins_ugly
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/gstreamer-$(GST_PLUGINS_SUBSINK_VER)-plugin-subsink
	set -e; if [ -d $(ARCHIVE)/gstreamer$(GST_PLUGINS_SUBSINK_VER)-plugin-subsink.git ]; \
		then cd $(ARCHIVE)/gstreamer$(GST_PLUGINS_SUBSINK_VER)-plugin-subsink.git; git pull; \
		else cd $(ARCHIVE); git clone https://github.com/christophecvr/gstreamer$(GST_PLUGINS_SUBSINK_VER)-plugin-subsink.git gstreamer$(GST_PLUGINS_SUBSINK_VER)-plugin-subsink.git; \
		fi
	cp -ra $(ARCHIVE)/gstreamer$(GST_PLUGINS_SUBSINK_VER)-plugin-subsink.git $(BUILD_TMP)/gstreamer$(GST_PLUGINS_SUBSINK_VER)-plugin-subsink
	$(CHDIR)/gstreamer$(GST_PLUGINS_SUBSINK_VER)-plugin-subsink; \
		aclocal --force -I m4; \
		libtoolize --copy --ltdl --force; \
		autoconf --force; \
		autoheader --force; \
		automake --add-missing --copy --force-missing --foreign; \
		$(CONFIGURE) \
			--prefix=/usr \
			--enable-silent-rules \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/gstreamer$(GST_PLUGINS_SUBSINK_VER)-plugin-subsink
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/gst_plugins_subsink/control
	touch $(BUILD_TMP)/gst_plugins_subsink/control/control
	echo Package: gst_plugins_subsink > $(BUILD_TMP)/gst_plugins_subsink/control/control
	echo Version: $(GST_PLUGINS_SUBSINK_VER) >> $(BUILD_TMP)/gst_plugins_subsink/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/gst_plugins_subsink/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/gst_plugins_subsink/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/gst_plugins_subsink/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/gst_plugins_subsink/control/control 
	echo Depends:  >> $(BUILD_TMP)/gst_plugins_subsink/control/control
	pushd $(BUILD_TMP)/gst_plugins_subsink/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/gst_plugins_subsink-$(GST_PLUGINS_SUBSINK_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/gst_plugins_subsink
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# gst_plugins_dvbmediasink-ipk
#
gst_plugins_dvbmediasink-ipk: $(D)/bootstrap $(D)/gstreamer $(D)/gst_plugins_base $(D)/gst_plugins_good $(D)/gst_plugins_bad $(D)/gst_plugins_ugly $(D)/gst_plugins_subsink $(D)/libdca
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/gstreamer$(GST_PLUGINS_DVBMEDIASINK_VER)-plugin-dvbmediasink
	set -e; if [ -d $(ARCHIVE)/gstreamer$(GST_PLUGINS_DVBMEDIASINK_VER)-plugin-dvbmediasink.git ]; \
		then cd $(ARCHIVE)/gstreamer$(GST_PLUGINS_DVBMEDIASINK_VER)-plugin-dvbmediasink.git; git pull; \
		else cd $(ARCHIVE); git clone -b gst-1.0 https://github.com/OpenPLi/gst-plugin-dvbmediasink.git gstreamer$(GST_PLUGINS_DVBMEDIASINK_VER)-plugin-dvbmediasink.git; \
		fi
	cp -ra $(ARCHIVE)/gstreamer$(GST_PLUGINS_DVBMEDIASINK_VER)-plugin-dvbmediasink.git $(BUILD_TMP)/gstreamer$(GST_PLUGINS_DVBMEDIASINK_VER)-plugin-dvbmediasink
	$(CHDIR)/gstreamer$(GST_PLUGINS_DVBMEDIASINK_VER)-plugin-dvbmediasink; \
		aclocal --force -I m4; \
		libtoolize --copy --ltdl --force; \
		autoconf --force; \
		autoheader --force; \
		automake --add-missing --copy --force-missing --foreign; \
		$(CONFIGURE) \
			--prefix=/usr \
			--enable-silent-rules \
			--with-wma \
			--with-wmv \
			--with-pcm \
			--with-dts \
			--with-eac3 \
			--with-h265 \
			--with-vb6 \
			--with-vb8 \
			--with-vb9 \
			--with-spark \
			--with-gstversion=1.0 \
		; \
		$(MAKE) all; \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	$(REMOVE)/gstreamer$(GST_PLUGINS_DVBMEDIASINK_VER)-plugin-dvbmediasink
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/gst_plugins_dvbmediasink/control
	touch $(BUILD_TMP)/gst_plugins_dvbmediasink/control/control
	echo Package: gst_plugins_dvbmediasink > $(BUILD_TMP)/gst_plugins_dvbmediasink/control/control
	echo Version: $(GST_PLUGINS_DVBMEDIASINK_VER) >> $(BUILD_TMP)/gst_plugins_dvbmediasink/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/gst_plugins_dvbmediasink/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/gst_plugins_dvbmediasink/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/gst_plugins_dvbmediasink/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/gst_plugins_dvbmediasink/control/control 
	echo Depends:  >> $(BUILD_TMP)/gst_plugins_dvbmediasink/control/control
	pushd $(BUILD_TMP)/gst_plugins_dvbmediasink/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/gst_plugins_dvbmediasink-$(GST_PLUGINS_DVBMEDIASINK_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/gst_plugins_dvbmediasink
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# ffmpeg
#
ffmpeg-ipk: $(D)/bootstrap $(D)/openssl $(D)/bzip2 $(D)/freetype $(D)/libass $(D)/libxml2 $(D)/libroxml $(D)/librtmp $(ARCHIVE)/$(FFMPEG_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/ffmpeg-$(FFMPEG_VER)
	$(UNTAR)/$(FFMPEG_SOURCE)
	$(CHDIR)/ffmpeg-$(FFMPEG_VER); \
		$(call apply_patches, $(FFMPEG_PATCH)); \
		./configure \
			--disable-ffplay \
			--disable-ffprobe \
			\
			--disable-doc \
			--disable-htmlpages \
			--disable-manpages \
			--disable-podpages \
			--disable-txtpages \
			\
			--disable-altivec \
			--disable-amd3dnow \
			--disable-amd3dnowext \
			--disable-mmx \
			--disable-mmxext \
			--disable-sse \
			--disable-sse2 \
			--disable-sse3 \
			--disable-ssse3 \
			--disable-sse4 \
			--disable-sse42 \
			--disable-avx \
			--disable-xop \
			--disable-fma3 \
			--disable-fma4 \
			--disable-avx2 \
			--disable-armv5te \
			--disable-armv6 \
			--disable-armv6t2 \
			--disable-vfp \
			--disable-inline-asm \
			--disable-mips32r2 \
			--disable-mipsdsp \
			--disable-mipsdspr2 \
			--disable-fast-unaligned \
			\
			--disable-dxva2 \
			--disable-vaapi \
			--disable-vdpau \
			\
			--disable-muxers \
			--enable-muxer=apng \
			--enable-muxer=flac \
			--enable-muxer=mp3 \
			--enable-muxer=h261 \
			--enable-muxer=h263 \
			--enable-muxer=h264 \
			--enable-muxer=hevc \
			--enable-muxer=image2 \
			--enable-muxer=image2pipe \
			--enable-muxer=m4v \
			--enable-muxer=matroska \
			--enable-muxer=mjpeg \
			--enable-muxer=mp4 \
			--enable-muxer=mpeg1video \
			--enable-muxer=mpeg2video \
			--enable-muxer=mpegts \
			--enable-muxer=ogg \
			\
			--disable-parsers \
			--enable-parser=aac \
			--enable-parser=aac_latm \
			--enable-parser=ac3 \
			--enable-parser=dca \
			--enable-parser=dvbsub \
			--enable-parser=dvd_nav \
			--enable-parser=dvdsub \
			--enable-parser=flac \
			--enable-parser=h264 \
			--enable-parser=hevc \
			--enable-parser=mjpeg \
			--enable-parser=mpeg4video \
			--enable-parser=mpegvideo \
			--enable-parser=mpegaudio \
			--enable-parser=png \
			--enable-parser=vc1 \
			--enable-parser=vorbis \
			--enable-parser=vp8 \
			--enable-parser=vp9 \
			\
			--disable-encoders \
			--enable-encoder=aac \
			--enable-encoder=h261 \
			--enable-encoder=h263 \
			--enable-encoder=h263p \
			--enable-encoder=jpeg2000 \
			--enable-encoder=jpegls \
			--enable-encoder=ljpeg \
			--enable-encoder=mjpeg \
			--enable-encoder=mpeg1video \
			--enable-encoder=mpeg2video \
			--enable-encoder=mpeg4 \
			--enable-encoder=png \
			--enable-encoder=rawvideo \
			\
			--disable-decoders \
			--enable-decoder=aac \
			--enable-decoder=aac_latm \
			--enable-decoder=adpcm_ct \
			--enable-decoder=adpcm_g722 \
			--enable-decoder=adpcm_g726 \
			--enable-decoder=adpcm_g726le \
			--enable-decoder=adpcm_ima_amv \
			--enable-decoder=adpcm_ima_oki \
			--enable-decoder=adpcm_ima_qt \
			--enable-decoder=adpcm_ima_rad \
			--enable-decoder=adpcm_ima_wav \
			--enable-decoder=adpcm_ms \
			--enable-decoder=adpcm_sbpro_2 \
			--enable-decoder=adpcm_sbpro_3 \
			--enable-decoder=adpcm_sbpro_4 \
			--enable-decoder=adpcm_swf \
			--enable-decoder=adpcm_yamaha \
			--enable-decoder=alac \
			--enable-decoder=ape \
			--enable-decoder=atrac1 \
			--enable-decoder=atrac3 \
			--enable-decoder=atrac3p \
			--enable-decoder=ass \
			--enable-decoder=cook \
			--enable-decoder=dca \
			--enable-decoder=dsd_lsbf \
			--enable-decoder=dsd_lsbf_planar \
			--enable-decoder=dsd_msbf \
			--enable-decoder=dsd_msbf_planar \
			--enable-decoder=dvbsub \
			--enable-decoder=dvdsub \
			--enable-decoder=eac3 \
			--enable-decoder=evrc \
			--enable-decoder=flac \
			--enable-decoder=g723_1 \
			--enable-decoder=g729 \
			--enable-decoder=h261 \
			--enable-decoder=h263 \
			--enable-decoder=h263i \
			--enable-decoder=h264 \
			--enable-decoder=hevc \
			--enable-decoder=iac \
			--enable-decoder=imc \
			--enable-decoder=jpeg2000 \
			--enable-decoder=jpegls \
			--enable-decoder=mace3 \
			--enable-decoder=mace6 \
			--enable-decoder=metasound \
			--enable-decoder=mjpeg \
			--enable-decoder=mlp \
			--enable-decoder=movtext \
			--enable-decoder=mp1 \
			--enable-decoder=mp2 \
			--enable-decoder=mp3 \
			--enable-decoder=mp3adu \
			--enable-decoder=mp3on4 \
			--enable-decoder=mpeg1video \
			--enable-decoder=mpeg2video \
			--enable-decoder=mpeg4 \
			--enable-decoder=nellymoser \
			--enable-decoder=opus \
			--enable-decoder=pcm_alaw \
			--enable-decoder=pcm_bluray \
			--enable-decoder=pcm_dvd \
			--enable-decoder=pcm_f32be \
			--enable-decoder=pcm_f32le \
			--enable-decoder=pcm_f64be \
			--enable-decoder=pcm_f64le \
			--enable-decoder=pcm_lxf \
			--enable-decoder=pcm_mulaw \
			--enable-decoder=pcm_s16be \
			--enable-decoder=pcm_s16be_planar \
			--enable-decoder=pcm_s16le \
			--enable-decoder=pcm_s16le_planar \
			--enable-decoder=pcm_s24be \
			--enable-decoder=pcm_s24daud \
			--enable-decoder=pcm_s24le \
			--enable-decoder=pcm_s24le_planar \
			--enable-decoder=pcm_s32be \
			--enable-decoder=pcm_s32le \
			--enable-decoder=pcm_s32le_planar \
			--enable-decoder=pcm_s8 \
			--enable-decoder=pcm_s8_planar \
			--enable-decoder=pcm_u16be \
			--enable-decoder=pcm_u16le \
			--enable-decoder=pcm_u24be \
			--enable-decoder=pcm_u24le \
			--enable-decoder=pcm_u32be \
			--enable-decoder=pcm_u32le \
			--enable-decoder=pcm_u8 \
			--enable-decoder=pgssub \
			--enable-decoder=png \
			--enable-decoder=qcelp \
			--enable-decoder=qdm2 \
			--enable-decoder=ra_144 \
			--enable-decoder=ra_288 \
			--enable-decoder=ralf \
			--enable-decoder=s302m \
			--enable-decoder=sipr \
			--enable-decoder=shorten \
			--enable-decoder=sonic \
			--enable-decoder=srt \
			--enable-decoder=ssa \
			--enable-decoder=subrip \
			--enable-decoder=subviewer \
			--enable-decoder=subviewer1 \
			--enable-decoder=tak \
			--enable-decoder=text \
			--enable-decoder=truehd \
			--enable-decoder=truespeech \
			--enable-decoder=tta \
			--enable-decoder=vorbis \
			--enable-decoder=wmalossless \
			--enable-decoder=wmapro \
			--enable-decoder=wmav1 \
			--enable-decoder=wmav2 \
			--enable-decoder=wmavoice \
			--enable-decoder=wavpack \
			--enable-decoder=xsub \
			\
			--disable-demuxers \
			--enable-demuxer=aac \
			--enable-demuxer=ac3 \
			--enable-demuxer=apng \
			--enable-demuxer=ass \
			--enable-demuxer=avi \
			--enable-demuxer=dts \
			--enable-demuxer=dash \
			--enable-demuxer=ffmetadata \
			--enable-demuxer=flac \
			--enable-demuxer=flv \
			--enable-demuxer=h264 \
			--enable-demuxer=hls \
			--enable-demuxer=image2 \
			--enable-demuxer=image2pipe \
			--enable-demuxer=image_bmp_pipe \
			--enable-demuxer=image_jpeg_pipe \
			--enable-demuxer=image_jpegls_pipe \
			--enable-demuxer=image_png_pipe \
			--enable-demuxer=m4v \
			--enable-demuxer=matroska \
			--enable-demuxer=mjpeg \
			--enable-demuxer=mov \
			--enable-demuxer=mp3 \
			--enable-demuxer=mpegts \
			--enable-demuxer=mpegtsraw \
			--enable-demuxer=mpegps \
			--enable-demuxer=mpegvideo \
			--enable-demuxer=mpjpeg \
			--enable-demuxer=ogg \
			--enable-demuxer=pcm_s16be \
			--enable-demuxer=pcm_s16le \
			--enable-demuxer=realtext \
			--enable-demuxer=rawvideo \
			--enable-demuxer=rm \
			--enable-demuxer=rtp \
			--enable-demuxer=rtsp \
			--enable-demuxer=srt \
			--enable-demuxer=vc1 \
			--enable-demuxer=wav \
			--enable-demuxer=webm_dash_manifest \
			\
			--disable-filters \
			--enable-filter=scale \
			--enable-filter=drawtext \
			\
			--enable-zlib \
			--enable-bzlib \
			--enable-openssl \
			--enable-libass \
			--enable-bsfs \
			--disable-xlib \
			--disable-libxcb \
			--disable-libxcb-shm \
			--disable-libxcb-xfixes \
			--disable-libxcb-shape \
			\
			$(FFMPEG_CONF_OPTS) \
			\
			--enable-shared \
			--enable-network \
			--enable-nonfree \
			--disable-static \
			--disable-debug \
			--disable-runtime-cpudetect \
			--enable-pic \
			--enable-pthreads \
			--enable-hardcoded-tables \
			--disable-optimizations \
			\
			--pkg-config=pkg-config \
			--enable-cross-compile \
			--cross-prefix=$(TARGET)- \
			--extra-cflags="$(TARGET_CFLAGS) $(FFMPRG_EXTRA_CFLAGS)" \
			--extra-ldflags="$(TARGET_LDFLAGS) -lrt" \
			--arch=$(BOXARCH) \
			--target-os=linux \
			--prefix=/usr \
			--bindir=/sbin \
			--mandir=/.remove \
			--datadir=/.remove \
			--docdir=/.remove \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	rm -r $(PKGPREFIX)/usr/include $(PKGPREFIX)/usr/lib/pkgconfig
	$(REMOVE)/ffmpeg-$(FFMPEG_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/ffmpeg/control
	touch $(BUILD_TMP)/ffmpeg/control/control
	echo Package: ffmpeg > $(BUILD_TMP)/ffmpeg/control/control
	echo Version: $(FFMPEG_VER) >> $(BUILD_TMP)/ffmpeg/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/ffmpeg/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/ffmpeg/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/ffmpeg/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/ffmpeg/control/control 
	echo Depends:  >> $(BUILD_TMP)/ffmpeg/control/control
	pushd $(BUILD_TMP)/ffmpeg/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/ffmpeg-$(FFMPEG_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/ffmpeg
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)

#
# lua
#
lua-ipk: $(D)/bootstrap $(D)/ncurses $(ARCHIVE)/$(LUAPOSIX_SOURCE) $(ARCHIVE)/$(LUA_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/lua-$(LUA_VER)
	mkdir -p $(PKGPREFIX)/usr/share/lua/$(LUA_VER_SHORT)
	$(UNTAR)/$(LUA_SOURCE)
	$(CHDIR)/lua-$(LUA_VER); \
		$(call apply_patches, $(LUAPOSIX_PATCH)); \
		tar xf $(ARCHIVE)/$(LUAPOSIX_SOURCE); \
		cd luaposix-git-$(LUAPOSIX_VER)/ext; cp posix/posix.c include/lua52compat.h ../../src/; cd ../..; \
		cd luaposix-git-$(LUAPOSIX_VER)/lib; cp *.lua $(TARGET_DIR)/usr/share/lua/$(LUA_VER_SHORT); cd ../..; \
		sed -i 's/<config.h>/"config.h"/' src/posix.c; \
		sed -i '/^#define/d' src/lua52compat.h; \
		sed -i 's|man/man1|/.remove|' Makefile; \
		$(MAKE) linux CC=$(TARGET)-gcc CPPFLAGS="$(TARGET_CPPFLAGS) -fPIC" LDFLAGS="-L$(TARGET_DIR)/usr/lib" BUILDMODE=dynamic PKG_VERSION=$(LUA_VER); \
		$(MAKE) install INSTALL_TOP=$(PKGPREFIX)/usr INSTALL_MAN=$(PKGPREFIX)/.remove
	rm -r $(PKGPREFIX)/usr/include $(PKGPREFIX)/usr/bin/luac
	$(REMOVE)/lua-$(LUA_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/lua/control
	touch $(BUILD_TMP)/lua/control/control
	echo Package: lua > $(BUILD_TMP)/lua/control/control
	echo Version: $(LUA_VER) >> $(BUILD_TMP)/lua/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/lua/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/lua/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/lua/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/lua/control/control 
	echo Depends:  >> $(BUILD_TMP)/lua/control/control
	pushd $(BUILD_TMP)/lua/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/lua-$(LUA_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/lua
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# luacurl-ipk
#
luacurl-ipk: $(D)/bootstrap $(D)/libcurl $(D)/lua $(ARCHIVE)/$(LUACURL_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/luacurl-git-$(LUACURL_VER)
	$(UNTAR)/$(LUACURL_SOURCE)
	$(CHDIR)/luacurl-git-$(LUACURL_VER); \
		$(MAKE) CC=$(TARGET)-gcc LDFLAGS="-L$(TARGET_DIR)/usr/lib" \
			LIBDIR=$(TARGET_DIR)/usr/lib \
			LUA_INC=$(TARGET_DIR)/usr/include; \
		$(MAKE) install DESTDIR=$(PKGPREFIX) LUA_CMOD=/usr/lib/lua/$(LUA_VER_SHORT) LUA_LMOD=/usr/share/lua/$(LUA_VER_SHORT)
	$(REMOVE)/luacurl-git-$(LUACURL_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/luacurl/control
	touch $(BUILD_TMP)/luacurl/control/control
	echo Package: luacurl > $(BUILD_TMP)/luacurl/control/control
	echo Version: $(LUACURL_VER) >> $(BUILD_TMP)/luacurl/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/luacurl/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/luacurl/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/luacurl/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/luacurl/control/control 
	echo Depends:  >> $(BUILD_TMP)/luacurl/control/control
	pushd $(BUILD_TMP)/luacurl/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/luacurl-$(LUACURL_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/luacurl
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# luaexpat-ipk
#
luaexpat-ipk: $(D)/bootstrap $(D)/lua $(D)/expat $(ARCHIVE)/$(LUAEXPAT_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/luaexpat-$(LUAEXPAT_VER)
	$(UNTAR)/$(LUAEXPAT_SOURCE)
	$(CHDIR)/luaexpat-$(LUAEXPAT_VER); \
		$(call apply_patches, $(LUAEXPAT_PATCH)); \
		$(MAKE) CC=$(TARGET)-gcc LDFLAGS="-L$(TARGET_DIR)/usr/lib" PREFIX=$(TARGET_DIR)/usr; \
		$(MAKE) install DESTDIR=$(PKGPREFIX)/usr
	$(REMOVE)/luaexpat-$(LUAEXPAT_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/luaexpat/control
	touch $(BUILD_TMP)/luaexpat/control/control
	echo Package: luaexpat > $(BUILD_TMP)/luaexpat/control/control
	echo Version: $(LUAEXPAT_VER) >> $(BUILD_TMP)/luaexpat/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/luaexpat/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/luaexpat/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/luaexpat/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/luaexpat/control/control 
	echo Depends:  >> $(BUILD_TMP)/luaexpat/control/control
	pushd $(BUILD_TMP)/luaexpat/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/luaexpat-$(LUAEXPAT_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/luaexpat
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)

#
# luasocket-ipk
#	
luasocket-ipk: $(D)/bootstrap $(D)/lua $(ARCHIVE)/$(LUASOCKET_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/luasocket-git-$(LUASOCKET_VER)
	$(UNTAR)/$(LUASOCKET_SOURCE)
	$(CHDIR)/luasocket-git-$(LUASOCKET_VER); \
		sed -i -e "s@LD_linux=gcc@LD_LINUX=$(TARGET)-gcc@" -e "s@CC_linux=gcc@CC_LINUX=$(TARGET)-gcc -L$(TARGET_DIR)/usr/lib@" -e "s@DESTDIR?=@DESTDIR?=$(PKGPREFIX)/usr@" src/makefile; \
		$(MAKE) CC=$(TARGET)-gcc LD=$(TARGET)-gcc LUAV=$(LUA_VER_SHORT) PLAT=linux COMPAT=COMPAT LUAINC_linux=$(TARGET_DIR)/usr/include LUAPREFIX_linux=; \
		$(MAKE) install LUAPREFIX_linux= LUAV=$(LUA_VER_SHORT)
	$(REMOVE)/luasocket-git-$(LUASOCKET_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/luasocket/control
	touch $(BUILD_TMP)/luasocket/control/control
	echo Package: luasocket > $(BUILD_TMP)/luasocket/control/control
	echo Version: $(LUASOCKET_VER) >> $(BUILD_TMP)/luasocket/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/luasocket/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/luasocket/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/luasocket/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/luasocket/control/control 
	echo Depends:  >> $(BUILD_TMP)/luasocket/control/control
	pushd $(BUILD_TMP)/luasocket/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/luasocket-$(LUASOCKET_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/luasocket
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# luafeedparser-ipk
#
luafeedparser-ipk: $(D)/bootstrap $(D)/lua $(ARCHIVE)/$(LUAFEEDPARSER_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	install -d $(PKGPREFIX)/usr/share/lua/$(LUA_VER_SHORT)
	$(REMOVE)/luafeedparser-git-$(LUAFEEDPARSER_VER)
	$(UNTAR)/$(LUAFEEDPARSER_SOURCE)
	$(CHDIR)/luafeedparser-git-$(LUAFEEDPARSER_VER); \
		sed -i -e "s/^PREFIX.*//" -e "s/^LUA_DIR.*//" Makefile ; \
		$(BUILDENV) $(MAKE) install  LUA_DIR=$(PKGPREFIX)/usr/share/lua/$(LUA_VER_SHORT)
	$(REMOVE)/luafeedparser-git-$(LUAFEEDPARSER_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/luafeedparser/control
	touch $(BUILD_TMP)/luafeedparser/control/control
	echo Package: luafeedparser > $(BUILD_TMP)/luafeedparser/control/control
	echo Version: $(LUAFEEDPARSER_VER) >> $(BUILD_TMP)/luafeedparser/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/luafeedparser/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/luafeedparser/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/luafeedparser/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/luafeedparser/control/control 
	echo Depends:  >> $(BUILD_TMP)/luafeedparser/control/control
	pushd $(BUILD_TMP)/luafeedparser/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/luafeedparser-$(LUAFEEDPARSER_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/luafeedparser
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# luasoap-ipk
#
luasoap-ipk: $(D)/bootstrap $(D)/lua $(ARCHIVE)/$(LUASOAP_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	install -d $(PKGPREFIX)/usr/share/lua/$(LUA_VER_SHORT)
	$(REMOVE)/luasoap-$(LUASOAP_VER)
	$(UNTAR)/$(LUASOAP_SOURCE)
	$(CHDIR)/luasoap-$(LUASOAP_VER); \
		$(call apply_patches, $(LUASOAP_PATCH)); \
		$(MAKE) install LUA_DIR=$(PKGPREFIX)/usr/share/lua/$(LUA_VER_SHORT)
	$(REMOVE)/luasoap-$(LUASOAP_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/luasoap/control
	touch $(BUILD_TMP)/luasoap/control/control
	echo Package: luasoap > $(BUILD_TMP)/luasoap/control/control
	echo Version: $(LUA_VER_SHORT) >> $(BUILD_TMP)/luasoap/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/luasoap/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/luasoap/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/luasoap/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/luasoap/control/control 
	echo Depends:  >> $(BUILD_TMP)/luasoap/control/control
	pushd $(BUILD_TMP)/luasoap/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/luasoap-$(LUA_VER_SHORT)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/luasoap
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# luajson-ipk
#
luajson-ipk: $(D)/bootstrap $(D)/lua $(ARCHIVE)/json.lua
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	install -d $(PKGPREFIX)/usr/share/lua/$(LUA_VER_SHORT)
	cp $(ARCHIVE)/json.lua $(PKGPREFIX)/usr/share/lua/$(LUA_VER_SHORT)/json.lua
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/luajson/control
	touch $(BUILD_TMP)/luajson/control/control
	echo Package: luajson > $(BUILD_TMP)/luajson/control/control
	echo Version: $(LUA_VER_SHORT) >> $(BUILD_TMP)/luajson/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/luajson/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/luajson/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/luajson/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/luajson/control/control 
	echo Depends:  >> $(BUILD_TMP)/luajson/control/control
	pushd $(BUILD_TMP)/luajson/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/luajson-$(LUA_VER_SHORT)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/luajson
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# python
#
python-ipk: $(D)/bootstrap $(D)/host_python $(D)/ncurses $(D)/zlib $(D)/openssl $(D)/libffi $(D)/bzip2 $(D)/readline $(D)/sqlite $(ARCHIVE)/$(PYTHON_SOURCE)
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	$(REMOVE)/Python-$(PYTHON_VER)
	$(UNTAR)/$(PYTHON_SOURCE)
	$(CHDIR)/Python-$(PYTHON_VER); \
		$(call apply_patches, $(PYTHON_PATCH)); \
		CONFIG_SITE= \
		$(BUILDENV) \
		autoreconf -fiv Modules/_ctypes/libffi; \
		autoconf; \
		./configure \
			--build=$(BUILD) \
			--host=$(TARGET) \
			--target=$(TARGET) \
			--prefix=/usr \
			--mandir=/.remove \
			--sysconfdir=/etc \
			--enable-shared \
			--with-lto \
			--enable-ipv6 \
			--with-threads \
			--with-pymalloc \
			--with-signal-module \
			--with-wctype-functions \
			ac_sys_system=Linux \
			ac_sys_release=2 \
			ac_cv_file__dev_ptmx=no \
			ac_cv_file__dev_ptc=no \
			ac_cv_have_long_long_format=yes \
			ac_cv_no_strict_aliasing_ok=yes \
			ac_cv_pthread=yes \
			ac_cv_cxx_thread=yes \
			ac_cv_sizeof_off_t=8 \
			ac_cv_have_chflags=no \
			ac_cv_have_lchflags=no \
			ac_cv_py_format_size_t=yes \
			ac_cv_broken_sem_getvalue=no \
			HOSTPYTHON=$(HOST_DIR)/bin/python$(PYTHON_VER_MAJOR) \
		; \
		$(MAKE) $(MAKE_OPTS) \
			PYTHON_MODULES_INCLUDE="$(PKGPREFIX)/usr/include" \
			PYTHON_MODULES_LIB="$(PKGPREFIX)/usr/lib" \
			PYTHON_XCOMPILE_DEPENDENCIES_PREFIX="$(PKGPREFIX)" \
			CROSS_COMPILE_TARGET=yes \
			CROSS_COMPILE=$(TARGET) \
			MACHDEP=linux2 \
			HOSTARCH=$(TARGET) \
			CFLAGS="$(TARGET_CFLAGS)" \
			LDFLAGS="$(TARGET_LDFLAGS)" \
			LD="$(TARGET)-gcc" \
			HOSTPYTHON=$(HOST_DIR)/bin/python$(PYTHON_VER_MAJOR) \
			HOSTPGEN=$(HOST_DIR)/bin/pgen \
			all DESTDIR=$(PKGPREFIX) \
		; \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
	ln -sf ../../libpython$(PYTHON_VER_MAJOR).so.1.0 $(PKGPREFIX)/$(PYTHON_DIR)/config/libpython$(PYTHON_VER_MAJOR).so; \
	ln -sf $(PKGPREFIX)/$(PYTHON_INCLUDE_DIR) $(TARGET_DIR)/usr/include/python
	rm -r $(PKGPREFIX)/usr/include $(PKGPREFIX)/usr/lib/pkgconfig
	$(REMOVE)/Python-$(PYTHON_VER)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/python/control
	touch $(BUILD_TMP)/python/control/control
	echo Package: python > $(BUILD_TMP)/python/control/control
	echo Version: $(PYTHON_VER) >> $(BUILD_TMP)/python/control/control
	echo Section: base/libraries >> $(BUILD_TMP)/python/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/python/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/python/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/python/control/control 
	echo Depends:  >> $(BUILD_TMP)/python/control/control
	pushd $(BUILD_TMP)/python/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/python-$(PYTHON_VER)_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/python
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)

#
# aio-grab-ipk
#
aio-grab-ipk: $(D)/bootstrap $(D)/libpng $(D)/libjpeg
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	set -e; cd $(TOOLS_DIR)/aio-grab-$(BOXARCH); \
		$(CONFIGURE_TOOLS) CPPFLAGS="$(CPPFLAGS) -I$(DRIVER_DIR)/bpamem" \
			--prefix= \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/aio-grab/control
	touch $(BUILD_TMP)/aio-grab/control/control
	echo Package: aio-grab > $(BUILD_TMP)/aio-grab/control/control
	echo Section: applications >> $(BUILD_TMP)/aio-grab/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/aio-grab/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/aio-grab/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/aio-grab/control/control 
	echo Depends:  >> $(BUILD_TMP)/aio-grab/control/control
	pushd $(BUILD_TMP)/aio-grab/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/aio-grab_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/aio-grab
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# showiframe-ipk
#
showiframe-ipk: $(D)/bootstrap
	$(START_BUILD)
	rm -rf $(PKGPREFIX)
	install -d $(PKGPREFIX)
	install -d $(PKGS_DIR)
	install -d $(PKGS_DIR)/$@
	set -e; cd $(TOOLS_DIR)/showiframe-$(BOXARCH); \
		$(CONFIGURE_TOOLS) \
			--prefix= \
		; \
		$(MAKE); \
		$(MAKE) install DESTDIR=$(PKGPREFIX)
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(PKGPREFIX)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	pushd $(PKGPREFIX) && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/data.tar.gz ./* && popd
	install -d $(BUILD_TMP)/showiframe/control
	touch $(BUILD_TMP)/showiframe/control/control
	echo Package: showiframe > $(BUILD_TMP)/showiframe/control/control
#	echo Version: $(SHOWIFRAME_VER) >> $(BUILD_TMP)/showiframe/control/control
	echo Section: applications >> $(BUILD_TMP)/showiframe/control/control
ifeq ($(BOXARCH), mips)
	echo Architecture: $(BOXARCH)el >> $(BUILD_TMP)/showiframe/control/control 
else
	echo Architecture: $(BOXARCH) >> $(BUILD_TMP)/showiframe/control/control 
endif
	echo Maintainer: $(MAINTAINER)  >> $(BUILD_TMP)/showiframe/control/control 
	echo Depends:  >> $(BUILD_TMP)/showiframe/control/control
	pushd $(BUILD_TMP)/showiframe/control && chmod +x * && tar --numeric-owner --group=0 --owner=0 -czf $(PKGS_DIR)/$@/control.tar.gz ./* && popd
	pushd $(PKGS_DIR)/$@ && echo 2.0 > debian-binary && ar rv $(PKGS_DIR)/showiframe_$(BOXARCH)_all.ipk ./data.tar.gz ./control.tar.gz ./debian-binary && popd && rm -rf data.tar.gz control.tar.gz debian-binary
	rm -rf $(BUILD_TMP)/showiframe
	rm -rf $(PKGPREFIX)
	rm -rf $(PKGS_DIR)/$@
	$(END_BUILD)
	
#
# all packages
#
packages: \
	libupnp-ipk \
	minidlna-ipk \
	fbshot-ipk \
	ofgwrite-ipk \
	xupnpd-ipk \
	graphlcd-ipk \
	lcd4linux-ipk \
	gstreamer-ipk \
	gst_plugins_good-ipk \
	gst_plugins_bad-ipk \
	gst_plugins_ugly-ipk \
	gst_plugins_subsink-ipk \
	gst_plugins_dvbmediasink-ipk \
	ffmpeg-ipk lua-ipk \
	luacurl-ipk \
	luaexpat-ipk \
	luasocket-ipk \
	luafeedparser-ipk \
	luajson-ipk \
	python-ipk \
	aio-grab-ipk \
	showiframe-ipk
		
#
# pkg-clean
#
packages-clean:
	cd $(PKGS_DIR) && rm -rf *
		
