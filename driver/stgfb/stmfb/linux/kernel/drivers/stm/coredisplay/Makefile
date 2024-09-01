ifeq ($(CONFIG_STM_FMDSW),y)
KBUILD_CPPFLAGS += -DUSE_FMD -DBUILTIN_FMD
else
ifeq ($(CONFIG_ST_BIOS),y)
KBUILD_CPPFLAGS += -DUSE_FMD
endif
endif

obj-m += stx7100_7109_5202/
obj-m += sti7200/
obj-m += sti7111_7141/
obj-m += sti7105_7106/
obj-m += sti5206/
obj-m += sti7108/
