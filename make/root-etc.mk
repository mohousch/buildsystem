#
# DIVERSE STUFF / TOOLS
#
$(D)/diverse-tools:
	$(START_BUILD)
	( cd root/etc && for i in $(INITSCRIPTS_ADAPTED_ETC_FILES); do \
		[ -f $$i ] && install -m 644 $$i $(TARGET_DIR)/etc/$$i || true; \
		[ "$${i%%/*}" = "init.d" ] && chmod 755 $(TARGET_DIR)/etc/$$i || true; done ) || true ; \
	( cd root/etc && for i in $(BASE_FILES_ADAPTED_ETC_FILES); do \
		[ -f $$i ] && install -m 644 $$i $(TARGET_DIR)/etc/$$i || true; \
		[ "$${i%%/*}" = "init.d" ] && chmod 755 $(TARGET_DIR)/etc/$$i || true; done ) ; \
	( cd root/etc && for i in $(BASE_PASSWD_ADAPTED_ETC_FILES); do \
		[ -f $$i ] && install -m 644 $$i $(TARGET_DIR)/etc/$$i || true; \
		[ "$${i%%/*}" = "init.d" ] && chmod 755 $(TARGET_DIR)/etc/$$i || true; done ) ; \
	( cd root/etc && for i in $(NETBASE_ADAPTED_ETC_FILES); do \
		[ -f $$i ] && install -m 644 $$i $(TARGET_DIR)/etc/$$i || true; \
		[ "$${i%%/*}" = "init.d" ] && chmod 755 $(TARGET_DIR)/etc/$$i || true; done ) ; \
	( cd root/etc && for i in $(WLAN_ADAPTED_ETC_FILES); do \
		[ -f $$i ] && install -m 755 $$i $(TARGET_DIR)/etc/$$i || true; \
		[ "$${i%%/*}" = "network" ] && chmod 755 $(TARGET_DIR)/etc/$$i || true; done ) ; \
	( cd root/etc && for i in $(DEFAULT_ADAPTED_ETC_FILES); do \
		[ -f $$i ] && install -m 755 $$i $(TARGET_DIR)/etc/$$i || true; \
		[ "$${i%%/*}" = "default" ] && chmod 755 $(TARGET_DIR)/etc/$$i || true; done ) ; \
	( cd root/etc && for i in $(SAMBA_ADAPTED_ETC_FILES); do \
		[ -f $$i ] && install -m 755 $$i $(TARGET_DIR)/etc/$$i || true; \
		[ "$${i%%/*}" = "samba" ] && chmod 755 $(TARGET_DIR)/etc/$$i || true; done ) ; \
	ln -sf /usr/share/zoneinfo/CET $(TARGET_DIR)/etc/localtime
	$(TOUCH)

#
# Adapted etc files and etc read-write files
#
FUSE_ADAPTED_ETC_FILES = \
	init.d/fuse

BASE_FILES_ADAPTED_ETC_FILES = \
	timezone.xml \
	hosts \
	fstab \
	profile \
	resolv.conf \
	shells \
	shells.conf \
	host.conf \
	nsswitch.conf \
	inetd.conf \
	irexec.keys \
	issue.net

BASE_PASSWD_ADAPTED_ETC_FILES = \
	passwd \
	group

NETBASE_ADAPTED_ETC_FILES = \
	protocols \
	services \
	network/interfaces \
	network/options
	
WLAN_ADAPTED_ETC_FILES = \
	network/post-wlan0.sh \
	network/pre-wlan0.sh

INITSCRIPTS_ADAPTED_ETC_FILES = \
	init.d/bootclean.sh \
	init.d/hostname \
	init.d/mountall \
	init.d/networking \
	init.d/rc \
	init.d/reboot \
	init.d/sendsigs \
	init.d/udhcpc \
	init.d/umountfs
	
DEFAULT_ADAPTED_ETC_FILES = \
	default/*
	
SAMBA_ADAPTED_ETC_FILES = \
	samba/*
	
ifeq ($(BOXARCH), sh4)
INITSCRIPTS_ADAPTED_ETC_FILES += \
	init.d/getfb.awk \
	init.d/makedev \
	init.d/mountvirtfs
endif

