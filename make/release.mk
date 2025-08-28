#
# release-common
#
RELEASE_DEPS = $(D)/bootstrap
RELEASE_DEPS += $(D)/kernel
RELEASE_DEPS += $(D)/driver
RELEASE_DEPS += $(D)/busybox
RELEASE_DEPS += $(D)/sysvinit
RELEASE_DEPS += $(D)/module_init_tools
RELEASE_DEPS += $(D)/vsftpd
#
# root-etc
#
RELEASE_DEPS += $(D)/diverse-tools
#
# misc 
#
RELEASE_DEPS += $(D)/util_linux
RELEASE_DEPS += $(D)/e2fsprogs
RELEASE_DEPS += $(D)/hdidle
RELEASE_DEPS += $(D)/portmap
RELEASE_DEPS += $(D)/jfsutils
RELEASE_DEPS += $(D)/nfs_utils
RELEASE_DEPS += $(D)/udpxy
RELEASE_DEPS += $(D)/opkg
RELEASE_DEPS += $(D)/alsa_utils
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))
RELEASE_DEPS += $(D)/ofgwrite
RELEASE_DEPS += $(D)/parted
RELEASE_DEPS += $(D)/ntfs_3g
RELEASE_DEPS += $(D)/mtd_utils 
RELEASE_DEPS += $(D)/gptfdisk
RELEASE_DEPS += $(D)/dvb-apps
RELEASE_DEPS += $(D)/dvbsnoop
endif
#
# tools
#
RELEASE_DEPS += $(D)/tools-aio-grab
RELEASE_DEPS += $(D)/tools-showiframe
ifeq ($(BOXARCH), sh4)
RELEASE_DEPS += $(LIRC)
RELEASE_DEPS += $(D)/tools-devinit
RELEASE_DEPS += $(D)/tools-evremote2
RELEASE_DEPS += $(D)/tools-fp_control
RELEASE_DEPS += $(D)/tools-flashtool-fup
RELEASE_DEPS += $(D)/tools-flashtool-mup
RELEASE_DEPS += $(D)/tools-flashtool-pad
RELEASE_DEPS += $(D)/tools-stfbcontrol
RELEASE_DEPS += $(D)/tools-ustslave
RELEASE_DEPS += $(D)/tools-vfdctl
RELEASE_DEPS += $(D)/tools-wait4button
RELEASE_DEPS += $(D)/tools-ipbox_eeprom
endif
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))
RELEASE_DEPS += $(D)/tools-turnoff_power
endif
#
# wlan
#
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))	
RELEASE_DEPS += $(D)/wpa_supplicant 
RELEASE_DEPS += $(D)/wireless_tools
endif
#
# python
#
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))
RELEASE_DEPS += $(D)/python
endif
#
# lua
#
RELEASE_DEPS += $(D)/lua 
RELEASE_DEPS += $(D)/luaexpat 
RELEASE_DEPS += $(D)/luacurl 
RELEASE_DEPS += $(D)/luasocket 
RELEASE_DEPS += $(D)/luafeedparser 
#RELEASE_DEPS += $(D)/luasoap 
RELEASE_DEPS += $(D)/luajson
#
# gstreamer
#
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))
RELEASE_DEPS  += $(D)/gstreamer 
RELEASE_DEPS  += $(D)/gst_plugins_base 
RELEASE_DEPS  += $(D)/gst_plugins_good 
RELEASE_DEPS  += $(D)/gst_plugins_bad 
RELEASE_DEPS  += $(D)/gst_plugins_ugly 
RELEASE_DEPS  += $(D)/gst_plugins_subsink
RELEASE_DEPS  += $(D)/gst_plugins_dvbmediasink
endif
#
# optional
#
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))
RELEASE_DEPS += $(D)/mc
RELEASE_DEPS += $(D)/nano
endif
#
# graphlcd
#
GRAPHLCD ?= graphlcd
ifeq ($(GRAPHLCD), graphlcd)
RELEASE_DEPS += $(D)/graphlcd
endif
#
# lcd4linux
#
LCD4LINUX ?= lcd4linux
ifeq ($(LCD4LINUX), lcd4linux)
RELEASE_DEPS += $(D)/lcd4linux
endif
#
# neutrino / neutrinoNG deps (optional)
#
RELEASE_DEPS += $(D)/ncurses  
RELEASE_DEPS += $(D)/libcurl
RELEASE_DEPS += $(D)/libpng 
RELEASE_DEPS += $(D)/libjpeg 
RELEASE_DEPS += $(D)/giflib 
RELEASE_DEPS += $(D)/freetype
RELEASE_DEPS += $(D)/alsa_utils 
RELEASE_DEPS += $(D)/ffmpeg
RELEASE_DEPS += $(D)/libfribidi 
RELEASE_DEPS += $(D)/libsigc 
RELEASE_DEPS += $(D)/libdvbsi 
RELEASE_DEPS += $(D)/pugixml 
RELEASE_DEPS += $(D)/libopenthreads
RELEASE_DEPS += $(D)/libid3tag
RELEASE_DEPS += $(D)/libmad
RELEASE_DEPS += $(D)/flac
RELEASE_DEPS += $(D)/openssl
RELEASE_DEPS += $(D)/libass
RELEASE_DEPS += $(D)/libdvbcsa

release-common: $(RELEASE_DEPS)
	rm -rf $(RELEASE_DIR) || true
	install -d $(RELEASE_DIR)
	install -d $(RELEASE_DIR)/{bin,boot,dev,dev.static,etc,lib,media,mnt,proc,ram,root,sbin,sys,tmp,usr,var}
	install -d $(RELEASE_DIR)/etc/{init.d,network,mdev,ssl,default,samba}
	install -d $(RELEASE_DIR)/etc/network/if-{post-{up,down},pre-{up,down},up,down}.d
	install -d $(RELEASE_DIR)/lib/{modules,firmware}
ifeq ($(BOXARCH), sh4)
	install -d $(RELEASE_DIR)/lib/udev
endif
	install -d $(RELEASE_DIR)/media/{hdd,dvd,nfs,usb,sda1,sdb1}
	install -d $(RELEASE_DIR)/mnt/{hdd,nfs,usb}
	install -d $(RELEASE_DIR)/mnt/mnt{0..7}
	install -d $(RELEASE_DIR)/usr/{bin,lib,sbin,share}
	install -d $(RELEASE_DIR)/usr/lib/locale
	cp -aR $(SKEL_ROOT)/usr/lib/locale/* $(RELEASE_DIR)/usr/lib/locale
	install -d $(RELEASE_DIR)/usr/share/{udhcpc,zoneinfo,fonts}
	install -d $(RELEASE_DIR)/var/{bin,etc,lib,net,tuxbox}
	install -d $(RELEASE_DIR)/var/tuxbox/config
	install -d $(RELEASE_DIR)/var/lib/{nfs,modules,opkg}
	install -d $(RELEASE_DIR)/var/spool/cron/crontabs
	install -d $(RELEASE_DIR)/usr/share/lua/5.2	
	mkdir -p $(RELEASE_DIR)/etc/rc.d/rc0.d
	ln -s ../init.d/sendsigs $(RELEASE_DIR)/etc/rc.d/rc0.d/S20sendsigs
	ln -s ../init.d/halt $(RELEASE_DIR)/etc/rc.d/rc0.d/S90halt
	mkdir -p $(RELEASE_DIR)/etc/rc.d/rc6.d
	ln -s ../init.d/sendsigs $(RELEASE_DIR)/etc/rc.d/rc6.d/S20sendsigs
	ln -s ../init.d/reboot $(RELEASE_DIR)/etc/rc.d/rc6.d/S90reboot
	touch $(RELEASE_DIR)/var/etc/.firstboot
	ln -sf /proc/mounts $(RELEASE_DIR)/etc/mtab
#
# imageversion
#
	echo "distro=$(BS_NAME)" > $(RELEASE_DIR)/.version
	echo "imagename=NeutrinoNG" >> $(RELEASE_DIR)/.version
	echo "imageversion=1$(BS_CYCLE)`date +%Y%m%d%H%M`" >> $(RELEASE_DIR)/.version
	echo "homepage=https://github.com/mohousch" >> $(RELEASE_DIR)/.version
	echo "creator=$(MAINTAINER)" >> $(RELEASE_DIR)/.version
	echo "docs=https://github.com/mohousch" >> $(RELEASE_DIR)/.version
	echo "forum=https://forum.mbremer.de" >> $(RELEASE_DIR)/.version
	echo "version=1$(BS_CYCLE)`date +%Y%m%d%H%M`" >> $(RELEASE_DIR)/.version
	echo "git=`git log | grep "^commit" | wc -l`" >> $(RELEASE_DIR)/.version
#
# bin/sbin/usr/bin/usr/sbin
#
	cp -a $(TARGET_DIR)/bin/* $(RELEASE_DIR)/bin/
	cp -a $(TARGET_DIR)/usr/bin/* $(RELEASE_DIR)/usr/bin/
	cp -a $(TARGET_DIR)/sbin/* $(RELEASE_DIR)/sbin/
	cp -a $(TARGET_DIR)/usr/sbin/* $(RELEASE_DIR)/usr/sbin/
ifeq ($(BOXARCH), sh4)
	cp -dp $(SKEL_ROOT)/sbin/MAKEDEV $(RELEASE_DIR)/sbin/
	ln -sf ../sbin/MAKEDEV $(RELEASE_DIR)/dev/MAKEDEV
	ln -sf ../../sbin/MAKEDEV $(RELEASE_DIR)/lib/udev/MAKEDEV
	cp $(SKEL_ROOT)/bin/vdstandby $(RELEASE_DIR)/bin/
	cp $(SKEL_ROOT)/etc/vdstandby.cfg $(RELEASE_DIR)/etc/
	cp $(SKEL_ROOT)/usr/sbin/fw_printenv $(RELEASE_DIR)/usr/sbin/
	ln -sf ../../usr/sbin/fw_printenv $(RELEASE_DIR)/usr/sbin/fw_setenv
endif
	cp $(SKEL_ROOT)/bin/autologin $(RELEASE_DIR)/bin/
	cp -dp $(SKEL_ROOT)/sbin/hotplug $(RELEASE_DIR)/sbin/
	cp -aR $(SKEL_ROOT)/etc/mdev/* $(RELEASE_DIR)/etc/mdev/
	cp -aR $(SKEL_ROOT)/etc/mdev_$(BOXARCH).conf $(RELEASE_DIR)/etc/mdev.conf
	cp -aR $(SKEL_ROOT)/usr/share/udhcpc/* $(RELEASE_DIR)/usr/share/udhcpc/
	cp -aR $(SKEL_ROOT)/usr/share/zoneinfo/* $(RELEASE_DIR)/usr/share/zoneinfo/
	cp -aR $(SKEL_ROOT)/usr/share/fonts $(RELEASE_DIR)/usr/share/
	cp -aR $(TARGET_DIR)/etc/init.d/* $(RELEASE_DIR)/etc/init.d/
	install -m 0755 $(BASE_DIR)/machine/$(BOXTYPE)/files/rcS $(RELEASE_DIR)/etc/init.d/rcS
	install -m 0755 $(SKEL_ROOT)/etc/init.d/rcS.local $(RELEASE_DIR)/etc/init.d/rcS.local
	cp -aR $(TARGET_DIR)/etc/* $(RELEASE_DIR)/etc/
	echo "$(BOXTYPE)" > $(RELEASE_DIR)/etc/hostname
	ln -sf ../../bin/busybox $(RELEASE_DIR)/usr/bin/ether-wake
#
# wlan firmware
#
	install -d $(RELEASE_DIR)/etc/Wireless
	cp -aR $(SKEL_ROOT)/lib/firmware/Wireless/* $(RELEASE_DIR)/etc/Wireless/
	cp -aR $(SKEL_ROOT)/lib/firmware/rtlwifi $(RELEASE_DIR)/lib/firmware/
	cp -aR $(SKEL_ROOT)/lib/firmware/*.bin $(RELEASE_DIR)/lib/firmware/
#
# lib
#
	cp -R $(TARGET_DIR)/lib/* $(RELEASE_DIR)/lib/
	rm -f $(RELEASE_DIR)/lib/*.{a,o,la}
	chmod 755 $(RELEASE_DIR)/lib/*
#
# usr/lib
#
	cp -R $(TARGET_DIR)/usr/lib/* $(RELEASE_DIR)/usr/lib/
	rm -rf $(RELEASE_DIR)/usr/lib/{engines,gconv,libxslt-plugins,pkgconfig,lua,python$(PYTHON_VER_MAJOR),enigma2,gstreamer-1.0,gio,dbus-1.0}
	rm -f $(RELEASE_DIR)/usr/lib/*.{a,o,la}
	chmod 755 $(RELEASE_DIR)/usr/lib/*
#
# gstreamer
#
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))
	cp -aR $(TARGET_DIR)/usr/lib/gstreamer-1.0 $(RELEASE_DIR)/usr/lib
	cp -aR $(TARGET_DIR)/usr/lib/gio $(RELEASE_DIR)/usr/lib
endif
#
# lua
#
	cp -R $(TARGET_DIR)/usr/lib/lua $(RELEASE_DIR)/usr/lib/
	if [ -d $(TARGET_DIR)/usr/share/lua ]; then \
		cp -aR $(TARGET_DIR)/usr/share/lua/* $(RELEASE_DIR)/usr/share/lua; \
	fi
#
# python
#
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))
	install -d $(RELEASE_DIR)/$(PYTHON_DIR)
	cp -R $(TARGET_DIR)/$(PYTHON_DIR)/* $(RELEASE_DIR)/$(PYTHON_DIR)/
	install -d $(RELEASE_DIR)/$(PYTHON_INCLUDE_DIR)
	cp $(TARGET_DIR)/$(PYTHON_INCLUDE_DIR)/pyconfig.h $(RELEASE_DIR)/$(PYTHON_INCLUDE_DIR)
endif
#
# mc
#
	if [ -e $(TARGET_DIR)/usr/bin/mc ]; then \
		cp -aR $(TARGET_DIR)/usr/share/mc $(RELEASE_DIR)/usr/share/; \
		cp -af $(TARGET_DIR)/usr/libexec $(RELEASE_DIR)/usr/; \
	fi
#
# shairport
#
	if [ -e $(TARGET_DIR)/usr/bin/shairport ]; then \
		cp -f $(SKEL_ROOT)/etc/init.d/shairport $(RELEASE_DIR)/etc/init.d/shairport; \
		chmod 755 $(RELEASE_DIR)/etc/init.d/shairport; \
		cp -f $(TARGET_DIR)/usr/lib/libhowl.so* $(RELEASE_DIR)/usr/lib; \
		cp -f $(TARGET_DIR)/usr/lib/libmDNSResponder.so* $(RELEASE_DIR)/usr/lib; \
	fi	
#
# alsa
#
	if [ -e $(TARGET_DIR)/usr/share/alsa ]; then \
		mkdir -p $(RELEASE_DIR)/usr/share/alsa/; \
		mkdir $(RELEASE_DIR)/usr/share/alsa/cards/; \
		mkdir $(RELEASE_DIR)/usr/share/alsa/pcm/; \
		cp -dp $(TARGET_DIR)/usr/share/alsa/alsa.conf $(RELEASE_DIR)/usr/share/alsa/alsa.conf; \
		cp $(TARGET_DIR)/usr/share/alsa/cards/aliases.conf $(RELEASE_DIR)/usr/share/alsa/cards/; \
		cp $(TARGET_DIR)/usr/share/alsa/pcm/default.conf $(RELEASE_DIR)/usr/share/alsa/pcm/; \
		cp $(TARGET_DIR)/usr/share/alsa/pcm/dmix.conf $(RELEASE_DIR)/usr/share/alsa/pcm/; \
	fi
#
# nfs-utils
#
	if [ -e $(TARGET_DIR)/usr/sbin/rpc.nfsd ]; then \
		cp -f $(TARGET_DIR)/usr/sbin/exportfs $(RELEASE_DIR)/usr/sbin/; \
		cp -f $(TARGET_DIR)/usr/sbin/rpc.nfsd $(RELEASE_DIR)/usr/sbin/; \
		cp -f $(TARGET_DIR)/usr/sbin/rpc.mountd $(RELEASE_DIR)/usr/sbin/; \
		cp -f $(TARGET_DIR)/usr/sbin/rpc.statd $(RELEASE_DIR)/usr/sbin/; \
	fi
#
# graphlcd
#
	if [ -e $(RELEASE_DIR)/usr/lib/libglcddrivers.so ]; then \
		cp -f $(TARGET_DIR)/etc/graphlcd.conf $(RELEASE_DIR)/etc/; \
		rm -f $(RELEASE_DIR)/usr/lib/libglcdskin.so*; \
	fi
#
# lcd4linux
#
	if [ -e $(TARGET_DIR)/usr/bin/lcd4linux ]; then \
		cp -f $(TARGET_DIR)/usr/bin/lcd4linux $(RELEASE_DIR)/usr/bin/; \
		cp -f $(TARGET_DIR)/etc/init.d/lcd4linux $(RELEASE_DIR)/etc/init.d/; \
		cp -a $(TARGET_DIR)/etc/lcd4linux.conf $(RELEASE_DIR)/etc/; \
	fi
#
# openvpn
#
	if [ -e $(TARGET_DIR)/usr/sbin/openvpn ]; then \
		install -d $(RELEASE_DIR)/etc/openvpn; \
	fi
#
# udpxy
#
	if [ -e $(TARGET_DIR)/usr/bin/udpxy ]; then \
		cp -a $(TARGET_DIR)/usr/bin/udpxrec $(RELEASE_DIR)/usr/bin; \
	fi
#
# xupnpd
#
	if [ -e $(TARGET_DIR)/usr/bin/xupnpd ]; then \
		cp -aR $(TARGET_DIR)/usr/share/xupnpd $(RELEASE_DIR)/usr/share; \
		mkdir -p $(RELEASE_DIR)/usr/share/xupnpd/playlists; \
	fi
#
# minisatip
#
	if [ -e $(TARGET_DIR)/usr/bin/minisatip ]; then \
		cp -aR $(TARGET_DIR)/usr/share/minisatip $(RELEASE_DIR)/usr/share; \
	fi
#
# dropbear
#
	if [ -d $(RELEASE_DIR)/etc/dropbear ]; then \
		mkdir -p $(RELEASE_DIR)/.ssh; \
		chmod 700 $(RELEASE_DIR)/.ssh; \
		ln -s /etc/dropbear/authorized_keys $(RELEASE_DIR)/.ssh/authorized_keys; \
	fi
#
# e2-multiboot
#
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))
	if [ -e $(TARGET_DIR)/var/lib/opkg/status ]; then \
		cp -af $(TARGET_DIR)/etc/image-version $(RELEASE_DIR)/etc; \
		cp -af $(TARGET_DIR)/etc/issue $(RELEASE_DIR)/etc; \
		cp -af $(TARGET_DIR)/usr/bin/enigma2 $(RELEASE_DIR)/usr/bin; \
		cp -af $(TARGET_DIR)/var/lib/opkg/status $(RELEASE_DIR)/var/lib/opkg; \
	fi
endif
#
# delete unnecessary python files
#
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/{bsddb,compiler,curses,lib-old,lib-tk,plat-linux3,test,sqlite3,pydoc_data,multiprocessing,hotshot,distutils,email,unitest,ensurepip,wsgiref,lib2to3,logging,idlelib}
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/pdb.doc
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/ctypes/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/email/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/json/tests
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/idlelib/idle_test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/idlelib/icons
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/lib2to3/tests
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/sqlite3/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/unittest/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/{test,conch,mail,names,news,words,flow,lore,pair,runner}
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/Cheetah/Tests
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/livestreamer_cli
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/lxml
	rm -f $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/libxml2mod.so
	rm -f $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/libxsltmod.so
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/OpenSSL/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/setuptools
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/zope/interface/tests
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/application/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/conch/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/internet/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/lore/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/mail/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/manhole/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/names/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/news/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/pair/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/persisted/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/protocols/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/python/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/runner/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/scripts/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/trial/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/web/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/twisted/words/test
	rm -rf $(RELEASE_DIR)/$(PYTHON_DIR)/site-packages/*-py$(PYTHON_VER_MAJOR).egg-info
	find $(RELEASE_DIR)/$(PYTHON_DIR)/ -name '*.a' -exec rm -f {} \;
	find $(RELEASE_DIR)/$(PYTHON_DIR)/ -name '*.c' -exec rm -f {} \;
	find $(RELEASE_DIR)/$(PYTHON_DIR)/ -name '*.pyx' -exec rm -f {} \;
	find $(RELEASE_DIR)/$(PYTHON_DIR)/ -name '*.py' -exec rm -f {} \;
	find $(RELEASE_DIR)/$(PYTHON_DIR)/ -name '*.o' -exec rm -f {} \;
	find $(RELEASE_DIR)/$(PYTHON_DIR)/ -name '*.la' -exec rm -f {} \;
endif
ifeq ($(BOXARCH), sh4)
	rm -f $(RELEASE_DIR)/sbin/jfs_fsck
	rm -f $(RELEASE_DIR)/sbin/fsck.jfs
	rm -f $(RELEASE_DIR)/sbin/jfs_mkfs
	rm -f $(RELEASE_DIR)/sbin/mkfs.jfs
	rm -f $(RELEASE_DIR)/sbin/jfs_tune
	rm -f $(RELEASE_DIR)/sbin/ffmpeg
	rm -f $(RELEASE_DIR)/etc/ssl/certs/ca-certificates.crt
	rm -f $(RELEASE_DIR)/usr/bin/gst*
	rm -f $(RELEASE_DIR)/usr/bin/gapplication
	rm -f $(RELEASE_DIR)/usr/bin/gio
	rm -f $(RELEASE_DIR)/usr/bin/gresource
	rm -f $(RELEASE_DIR)/usr/bin/gsettings
	rm -f $(RELEASE_DIR)/usr/bin/gdbus
	rm -f $(RELEASE_DIR)/usr/bin/dbus*
	rm -f $(RELEASE_DIR)/usr/bin/python*
	rm -f $(RELEASE_DIR)/usr/bin/avahi-*
	rm -f $(RELEASE_DIR)/usr/bin/easy_install*
	rm -f $(RELEASE_DIR)/usr/bin/glib-*
	rm -f $(addprefix $(RELEASE_DIR)/usr/bin/,dvdnav-config gio-querymodules gobject-query gtester gtester-report)
	rm -f $(addprefix $(RELEASE_DIR)/usr/bin/,livestreamer mailmail manhole)
endif
ifeq ($(BOXARCH), $(filter $(BOXARCH), arm mips))
	rm -rf $(RELEASE_DIR)/dev.static
	rm -rf $(RELEASE_DIR)/ram
	rm -rf $(RELEASE_DIR)/root
endif
	rm -rf $(RELEASE_DIR)/usr/lib/m4-nofpu/
	rm -rf $(RELEASE_DIR)/usr/lib/gcc
	rm -f $(RELEASE_DIR)/usr/lib/libc.so
	rm -f $(RELEASE_DIR)/usr/share/meta/*
	rm -f $(RELEASE_DIR)/lib/libSegFault*
	rm -f $(RELEASE_DIR)/lib/libstdc++.*-gdb.py
	rm -f $(RELEASE_DIR)/lib/libthread_db*
	rm -f $(RELEASE_DIR)/lib/libanl*
	rm -rf $(RELEASE_DIR)/usr/lib/alsa
	rm -rf $(RELEASE_DIR)/usr/lib/glib-2.0
	rm -rf $(RELEASE_DIR)/usr/lib/cmake
	rm -f $(RELEASE_DIR)/usr/lib/*.py
	rm -f $(RELEASE_DIR)/usr/lib/libc.so
	rm -f $(RELEASE_DIR)/usr/lib/xml2Conf.sh
	rm -f $(RELEASE_DIR)/usr/lib/libfontconfig*
	rm -f $(RELEASE_DIR)/usr/lib/libthread_db*
	rm -f $(RELEASE_DIR)/usr/lib/libanl*
	rm -f $(RELEASE_DIR)/sbin/ldconfig
	rm -f $(RELEASE_DIR)/usr/bin/{gdbus-codegen,glib-*,gtester-report}
#
#
#
	ln -s /tmp $(RELEASE_DIR)/var/lock
	ln -s /tmp $(RELEASE_DIR)/var/log
	ln -s /tmp $(RELEASE_DIR)/var/run
	ln -s /tmp $(RELEASE_DIR)/var/tmp
	ln -s /tmp $(RELEASE_DIR)/var/empty
	
#
# release
#
release: release-common release-$(BOXTYPE)
	$(START_BUILD)
#
# delete unnecessary files
#
	rm -rf $(RELEASE_DIR)/var/tuxbox/config/zapit/services.xml
	rm -rf $(RELEASE_DIR)/var/tuxbox/config/zapit/bouquets.xml
	rm -rf $(RELEASE_DIR)/var/tuxbox/config/zapit/ubouquets.xml
ifeq ($(BOXARCH), sh4)
	[ -e $(RELEASE_DIR)/usr/bin/neutrino2 ] && rm -rf $(RELEASE_DIR)/usr/bin/neutrino2 || true
	[ -d $(RELEASE_DIR)/var/tuxbox/locale/de/LC_MESSAGES/neutrino2.mo ] && rm -rf $(RELEASE_DIR)/var/tuxbox/locale || true
	[ -e $(RELEASE_DIR)/usr/bin/neutrino ] && rm -rf $(RELEASE_DIR)/usr/bin/neutrino || true
	[ -e $(RELEASE_DIR)/usr/bin/enigma2 ] && rm -rf $(RELEASE_DIR)/usr/bin/enigma2 || true
	[ -e $(RELEASE_DIR)/usr/bin/titan ] && rm -rf $(RELEASE_DIR)/usr/bin/titan || true
	[ -e $(RELEASE_DIR)/usr/bin/amixer ] && rm -rf $(RELEASE_DIR)/usr/bin/amixer || true
	[ -e $(RELEASE_DIR)/usr/bin/eplayer3 ] && rm -rf $(RELEASE_DIR)/usr/bin/eplayer3 || true
	[ -e $(RELEASE_DIR)/usr/bin/exteplayer3 ] && rm -rf $(RELEASE_DIR)/usr/bin/exteplayer3 || true
	[ -e $(RELEASE_DIR)/usr/bin/satip_client ] && rm -rf $(RELEASE_DIR)/usr/bin/satip* || true
	[ -e $(RELEASE_DIR)/sbin/sfdisk ] && rm -rf $(RELEASE_DIR)/sbin/sfdisk || true
	[ -e $(RELEASE_DIR)/usr/bin/lircd ] && rm -rf $(RELEASE_DIR)/usr/bin/lircd || true
	rm -rf $(RELEASE_DIR)/usr/lib/locale
	rm -rf $(RELEASE_DIR)/usr/share/zoneinfo
	[ -e $(RELEASE_DIR)/usr/lib/libipkg.so ] && rm -rf $(RELEASE_DIR)/usr/lib/libipkg* || true
	[ -e $(RELEASE_DIR)/usr/bin/ipkg-cl ] && rm -rf $(RELEASE_DIR)/usr/bin/ipkg* || true
	[ -e $(RELEASE_DIR)/usr/lib/libarchive.so ] && rm -rf $(RELEASE_DIR)/usr/lib/libarchive* || true
	[ -e $(RELEASE_DIR)/usr/lib/libdbus-1.so ] && rm -rf $(RELEASE_DIR)/usr/lib/libdbus-1* || true
	[ -e $(RELEASE_DIR)/usr/lib/libeplayer3.so ] && rm -rf $(RELEASE_DIR)/usr/lib/libeplayer3* || true
	[ -e $(RELEASE_DIR)/usr/lib/libexteplayer3.so ] && rm -rf $(RELEASE_DIR)/usr/lib/libexteplayer3* || true
	[ -e $(RELEASE_DIR)/usr/lib/libglib-2.0.so ] && rm -rf $(RELEASE_DIR)/usr/lib/libglib-2.0* || true
	[ -e $(RELEASE_DIR)/usr/lib/libgmodule-2.0.so ] && rm -rf $(RELEASE_DIR)/usr/lib/libgmodule-2.0* || true
	[ -e $(RELEASE_DIR)/usr/lib/libgobject-2.0.so ] && rm -rf $(RELEASE_DIR)/usr/lib/libgobject-2.0* || true
	[ -e $(RELEASE_DIR)/usr/lib/libgthread-2.0.so ] && rm -rf $(RELEASE_DIR)/usr/lib/libgthread-2.0* || true
	[ -e $(RELEASE_DIR)/usr/lib/libpython2.7.so ] && rm -rf $(RELEASE_DIR)/usr/lib/libpython* || true
endif
#
# imigrate /etc to /var/etc
#
	cp -dpfr $(RELEASE_DIR)/etc $(RELEASE_DIR)/var
	rm -fr $(RELEASE_DIR)/etc
	ln -sf /var/etc $(RELEASE_DIR)
#
# strip
#	
ifneq ($(OPTIMIZATIONS), $(filter $(OPTIMIZATIONS), kerneldebug debug normal))
	find $(RELEASE_DIR)/ -name '*' -exec $(TARGET)-strip --strip-unneeded {} &>/dev/null \;
endif
	$(END_BUILD)
	
#
# release-clean
#
release-clean:
	rm -rf $(RELEASE_DIR)

