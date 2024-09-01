CCFLAGSY += -I$(TREE_ROOT)/linux/include/linux/stm/
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/media/video/stm/
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/sound/pseudocard
CCFLAGSY += -I$(CONFIG_STGFB_PATH)/include
CCFLAGSY += -DDEBUG_DVB -D__TDT__

# workaround for stlinux24
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
endif

stmsysfs-objs:= player_interface.o \
		sysfs_module.o \
		player_sysfs.o

obj-$(CONFIG_STM_UNIFY_STMSYSFS) += stmsysfs.o
