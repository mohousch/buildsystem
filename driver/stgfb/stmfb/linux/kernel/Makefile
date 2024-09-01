KBUILD_CPPFLAGS += -I$(STG_TOPDIR) -I$(STG_TOPDIR)/include -I$(STG_TOPDIR)/linux/kernel/include

ifeq ($(CONFIG_STG_DEBUG),y)
KBUILD_CPPFLAGS += -DDEBUG
endif

ifeq ($(CONFIG_ST_BIOS),y)
KBUILD_CPPFLAGS += -DHAVE_BIOS
endif

obj-m := drivers/
